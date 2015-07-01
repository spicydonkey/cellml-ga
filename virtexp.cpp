#include <unistd.h>
#include <stdlib.h>
#include "virtexp.h"
#include "AdvXMLParser.h"
#include "utils.h"
#include "cellml_observer.h"
#include <math.h>

using namespace std;
using namespace AdvXMLParser;
using namespace iface::cellml_api;
using namespace iface::cellml_services;


#define EPSILON 0.01

extern ObjRef<iface::cellml_api::CellMLBootstrap> bootstrap; //CellML api bootstrap
extern ObjRef<iface::cellml_services::CellMLIntegrationService> cis;

VirtualExperiment::VirtualExperiment():m_nResultColumn(-1),m_ReportStep(0.0),m_MaxTime(0),m_Accuracy(EPSILON)
{
}

VirtualExperiment::~VirtualExperiment()
{
}

VirtualExperiment *VirtualExperiment::LoadExperiment(const AdvXMLParser::Element& elem)
{
    VirtualExperiment *vx=NULL;

    string strName=elem.GetAttribute("ModelFilePath").GetValue();	// Get model name

    if(!strName.size())
        return NULL;	// error: no model to optimize

    vx=new VirtualExperiment;	// allocate memory for a new VE object

	// Load the CellML model
    if(!vx->LoadModel(strName))
    {	// Error in loading model
        delete vx;
        vx=NULL;
    }
    else
    {	// Model loaded correctly - read the VE data
		// Set model variable of interest (variable corresponding to the 'target' variable in data)
        vx->m_nResultColumn=atoi(elem.GetAttribute("ResultColumn").GetValue().c_str());

		// Set accuracy (TODO unused)
        if(elem.GetAttribute("Accuracy").GetValue().size())
              vx->m_Accuracy=atof(elem.GetAttribute("Accuracy").GetValue().c_str());
       
		// Set maximum time limit on simulator (no time limit on default)
        vx->m_MaxTime=atoi(elem.GetAttribute("MaxSecondsForSimulation").GetValue().c_str());

        vx->m_ReportStep=atof(elem.GetAttribute("ReportStep").GetValue().c_str());
        
		// Acquire the v-experiment data points
		for(int i=0;;i++)
        {
            const AdvXMLParser::Element& al=elem("AssessmentPoints",0)("AssessmentPoint",i);	// get a data point
            POINT pt;	// initialise a POINT object to store data

            if(al.IsNull())
                break;

			// Update point and store in m_Timepoints
            pt.first=atof(al.GetAttribute("time").GetValue().c_str());
            pt.second=atof(al.GetAttribute("target").GetValue().c_str());
            vx->m_Timepoints.push_back(pt);
        }

        // Read experimental configuration as fixed model parameters
        for(int i=0;;i++)
        {
            const AdvXMLParser::Element& al=elem("Parameters",0)("Parameter",i);

            if(al.IsNull())
                break;

            wstring name=convert(al.GetAttribute("ToSet").GetValue());		// parameter to set as simulation constant
            double val=atof(al.GetAttribute("Value").GetValue().c_str());	// value
            if(name.size())
                vx->m_Parameters[name]=val;		// update simulation config
        }
    }
    
    return vx;
}

double VirtualExperiment::getSSRD(std::vector<std::pair<int,double> >& d)
{
    double SSR=0.0;		// init sum of squared residuals

#ifdef DEBUG_BUILD
	// Check if simulation and experimental data are same size
	if(d.size()!=m_Timepoints.size())
	{
		fprintf(stderr,"getSSRD: estimation and data vector need to have the same size but are different.\n");
		return INFINITY;
	}
#endif

    for(int i=0;i<d.size();i++)
    {
		// TODO d[i].first may not be i while it is quite important in the regression analysis
		double sim_data = d[i].second;		// predicted data value from simulation 'near' the {d[i].first}^th data point
		double exp_data = m_Timepoints[d[i].first].second;	// virtual experiment measurement

		// residual is normalised by dividing by the measured value before squaring! (TODO hence measured value of 0 yields INF normed res)
        //r+=pow((d[i].second-m_Timepoints[d[i].first].second)/m_Timepoints[d[i].first].second,2);
		SSR+=pow(sim_data/exp_data-1.0,2);
    }
    return SSR;
}

bool VirtualExperiment::LoadModel(const std::string& model_name)
{
    bool res=false;

    std::wstring modelURL=convert(model_name);
    m_strModelName=model_name;
 
    try
    {
        m_Model=bootstrap->modelLoader()->loadFromURL(modelURL); 
        res=true;
    }
    catch(CellMLException e)
    {
        printf("Error loading model %s\n",model_name.c_str());
        res=false;
    }
    return res;
}

void VirtualExperiment::SetParameters(VariablesHolder& v)
{
	// iterate through the stored alleles
    for(int i=0;;i++)
    {
        wstring n=v.name(i);	// get the next allele name
        if(n.empty())
           break;
        double val=v(n);
        m_Parameters[n]=val;	// add the allele as a constant for the model
    }
}

void VirtualExperiment::SetVariables(VariablesHolder& v)
{
    ObjRef<iface::cellml_api::CellMLComponentSet> comps=m_Model->modelComponents();
    ObjRef<iface::cellml_api::CellMLComponentIterator> comps_it=comps->iterateComponents();
    ObjRef<iface::cellml_api::CellMLComponent> firstComp=comps_it->nextComponent();

	// Iterate thorough model components
    while(firstComp)
    {
        ObjRef<iface::cellml_api::CellMLVariableSet> vars=firstComp->variables();
        ObjRef<iface::cellml_api::CellMLVariableIterator> vars_it=vars->iterateVariables();
        ObjRef<iface::cellml_api::CellMLVariable> var=vars_it->nextVariable();

        string compname=convert(firstComp->name());

		// Iterate component variables
        while(var)
        {
            wstring name=var->name();	// get this variable's name

            // Find the full-name for the variable
			wstring fullname=name;
            if(compname!="all" && compname!="")
            {
                fullname=convert(compname)+convert(".");
                fullname+=name;
            }

			// Get model optimization parameter from v or experimental constants from m_Parameters
            if(v.exists(fullname))
            {	// model optimisation paramter
                char sss[120];	// buffer for parameter value 
                gcvt(v(fullname),25,sss);	// convert the param value to char string
                std::wstring wv=convert(sss);
                var->initialValue(wv);		// set this variable
            }
            else if(m_Parameters.find(fullname)!=m_Parameters.end())
            {	// experimental constant
                char sss[120];
                gcvt(m_Parameters[fullname],25,sss);
                std::wstring wv=convert(sss);
                var->initialValue(wv);
            }
            var=vars_it->nextVariable();
        }
        firstComp=comps_it->nextComponent();
    }
}


bool VirtualExperiment::isValid()
{
	for (int i = 0; i < m_Timepoints.size(); i++)
	{
		// Check for zero target value
		if (m_Timepoints[i].second == 0.0)
		{
			fprintf(stderr, "Error: VirtualExperiment: invalid target value: zero target\n");
			return false;
		}

		// Check for chronological ordering
	}
	return true;
}


// Compile and run an ODE solver to simulate the configured CellML model
// Select local estimation points from the result 
// Return the model's deviation to virt-experiment as Residual Sum of Squares
double VirtualExperiment::Evaluate()
{
    double res=0.0;

    ObjRef<iface::cellml_services::ODESolverCompiledModel> compiledModel;
    ObjRef<iface::cellml_services::ODESolverRun> osr;
    time_t calc_started;

    try
    {
		// Set up the ODE solver
       compiledModel=cis->compileModelODE(m_Model);
       osr=cis->createODEIntegrationRun(compiledModel);
       LocalProgressObserver *po=new LocalProgressObserver(compiledModel);

       osr->setProgressObserver(po);
       po->release_ref();
       osr->stepType(iface::cellml_services::BDF_IMPLICIT_1_5_SOLVE);
       osr->setStepSizeControl(1e-6,1e-6,1.0,0.0,1.0);
       osr->setResultRange(0.0,m_Timepoints[m_Timepoints.size()-1].first,m_Timepoints[m_Timepoints.size()-1].first);	// TODO Maximum point density?
       if(m_ReportStep)
            osr->setTabulationStepControl(m_ReportStep,true);

		// Run the solver
       calc_started=time(NULL);		// solver start time
       osr->start();
	   
	   // Wait until the solver has completed
       while(!po->finished())
       {
           usleep(1000);	// wait 1ms for the ODE solver
           if(m_MaxTime)
           {
               time_t t=time(NULL);		// current time
			   // Check if the solver is running over maximum time
               if((unsigned long)(t-calc_started)>m_MaxTime)
               {
                   po->failed("Took too long to integrate");
                   throw CellMLException();
               }
           }
       }      

	   // Regression analysis
       if(!po->failed())
       {
           std::vector<double> vd;
           std::vector<std::pair<int,double> > results;
           int recsize=po->GetResults(vd);	// get ODE simulation result as 1D-vector and per-time record size
 
		   // Collate the set of estimation model points corresponding to VE data points
		   //
#ifndef DEBUG_BUILD
		   // TODO Shouldn't we search for the t-point in the simulation result that is in range to a t-point specified in VE?
				// in original code, multiple points from the simulation can be mapped in-range of a point in VE
				// should we look for the closest simulation point wrt time?
		   for(int i=0;i<vd.size();i+=recsize)	// iterate through the time-points in simulation
           {
			   // record the result if the simulation point is in a close range to a VE point
               for(int j=0;j<m_Timepoints.size();j++)
                   
					//if(in_range(vd[i],m_Timepoints[j].first,EPSILON))	// TODO should in_range gap be EPSILON or 'accuracy'?
					if (in_range(vd[i], m_Timepoints[j].first, m_Accuracy))
					{
						results.push_back(make_pair(j,vd[i+m_nResultColumn]));	// add the variable of interest
						break;
					}
           }
#else
		   // FIX Replace above nested loop for building estimation vector with code below:
		   // Guaranteed to match 1-1 OR output error message
		   // May not find the closest estimation, but finds an appropriate one
		   //
		   // iterate through the data points
		   for(int i=0;i<m_Timepoints.size();i++)
		   {
			   bool b_match=false;	// flag to indicate if a data point has been matched with an appropriate estimation
			   double diff;
			   double t_target=m_Timepoints[i].first;	// target assessment time
			   int best_est;

			   // get an estimation from simulation result
			   for(int j=0;j<vd.size();j+=recsize)
			   {
				   /*	Match to the first simulation result in-range
				   // check if sim-point is in range
				   if (in_range(vd[j],m_Timepoints[i].first,m_Accuracy))
				   {
					   results.push_back(make_pair(i,vd[j+m_nResultColumn]));	// add the var of interest
					   b_match=true;
					   break;	// done with this data-point
				   }
				   */

				   // Get the closest estimate wrt time
				   if (!b_match)
				   {
						diff=fabs(vd[j]-t_target);
						b_match=true;
						best_est=j;
				   }
				   else
				   {
					   // Update best estimate
					   if(diff>fabs(vd[j]-t_target))
					   {
						   diff=fabs(vd[j]-t_target);
						   best_est=j;
					   }
				   }
			   }

			   results.push_back(make_pair(i,vd[best_est+m_nResultColumn]));	// add the var of interest

			   if(!b_match)
			   {
				   std::cerr << "Error: Simulation cannot estimate VE data-point" << std::endl;

				   //// print the data-point and simulation result as a check
				   //std::cerr << "DATA POINT: " << m_Timepoints[i].first << "\n";
				   //std::cerr << "Simulation:\n";
				   //for(int j=0;j<vd.size();j+=recsize)
				   //{
					  // std::cerr << vd[j] << " ";
				   //}
				   //std::cerr << std::endl;
			   }
		   }
#endif

#ifdef DEBUG_BUILD
		   // Check for multiple estimation of data points
		   for(int data_index=0;data_index<m_Timepoints.size();data_index++)
		   {
			   int count=0;
			   // iterate through results vector and count the number of estimations
			   for(int i=0;i<results.size();i++)
			   {
				   if(results[i].first==data_index)
					   count++;
			   }
			   if(count!=1)
					std::cerr << data_index << ":" << count << std::endl;
		   }
#endif

		   // TODO more importantly, shouldn't results.size()==m_Timepoints.size() so that all data points have estimations?
				// No duplicate estimation: our estimation vector selector above will eliminate duplicates
				// No data missing estimation: error message will be printed. SSRD function should print error message
           if(!results.size())
           {
               fprintf(stderr,"Results vector is empty, Observer returned %d bytes (%d records)\n",vd.size(),vd.size()/recsize);
           }

		   // Calculate the squared-sum-residual
#ifndef DEBUG_BUILD
		   // TODO getSSRD doesn't check if results is the same size as m_Timepoints; can lead to multiple estimation?
		   res=(results.size()?getSSRD(results):INFINITY);	// return INF if result vector is empty
#else
		   // FIX
		   // Check completeness of estimation points selected from simulation
		   res=((results.size()==m_Timepoints.size())?getSSRD(results):INFINITY);
#endif
	   }
       else
           res=INFINITY;	// return INF if simulation failed
    }
    catch(CellMLException e)
    {
        fprintf(stderr,"Error evaluating model\n");
        res=INFINITY;
    }
    return res;
}

double VirtualExperiment::Runner::operator()(VariablesHolder& v)
{
    pOwner->SetVariables(v);
    return pOwner->Evaluate();
}


VEGroup::VEGroup()
{
}

VEGroup::~VEGroup()
{
}


VEGroup& VEGroup::instance()
{
    static VEGroup *pInstance=new VEGroup;

    return *pInstance;
}


/**
 *	Evaluate model parameters based on the average SSR across the whole VEs
 *	
 *	v contains a list of parameters to characterise a CellML model
 *	
 *	Returns INFINITY if any virtual experiment causes error (by returning inf)	
 *	0.0 returned when the VEGroup object contains no virtual experiments
 **/
double VEGroup::Evaluate(VariablesHolder& v)
{
    double res=0.0;		// initialise the total residual from all models and VE data points
    int count=0;	// counter for the number of experiments that yield a finite residual

    if(!experiments.size())
        return 0.0;		// no virtual experiments to reference

	// iterate through each VE in group
    for(int i=0;i<experiments.size();i++)
    {
		// set model variables to compare against experiment
        experiments[i]->SetVariables(v);

		// evaluate squared-sum-residual for this test (model config + VE data)
        double d=experiments[i]->Evaluate();

		// update the total residual
        if(d!=INFINITY)
        {	
            res+=d;
            count++;
        }
#ifdef DEBUG_BUILD
		// If any experiment evaluated to INFINITY, this loop should immediately return INFINITY, since all experiments need to be processed properly
		else
		{
			fprintf(stderr,"Error in evaluating Experiment[%d] with parameters: ",i);
			v.print(stderr);	// print model parameters
			return INFINITY;
		}
#endif
    }

    // Return the avg SSR obtained across all v-experiments
	return (count==experiments.size()?res/(double)count:INFINITY);
}

void VEGroup::add(VirtualExperiment *p)
{
    experiments.push_back(p);
}

