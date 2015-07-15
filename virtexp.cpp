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

VariablesHolder::VariablesHolder() 
{
}

VariablesHolder::VariablesHolder(const VariablesHolder& other) 
{ 
	m_Vars.assign(other.m_Vars.begin(),other.m_Vars.end()); 
}

VariablesHolder::~VariablesHolder()
{
}

VariablesHolder& VariablesHolder::operator=(const VariablesHolder& other)
{
	if(&other!=this)
		m_Vars.assign(other.m_Vars.begin(),other.m_Vars.end());
	return *this;
}

double VariablesHolder::operator()(const std::wstring& name)
{
	ALLELE::iterator it=find_if(m_Vars.begin(),m_Vars.end(),
								bind1st(pair_equal_to<std::wstring,double>(),name));	//find iterator to the pair in m_Vars for which the "first" member equals name (end if no such pair)
	return (it==m_Vars.end()?double(0.0):it->second);
}

double VariablesHolder::operator()(const std::wstring& name,double val)
{
	// find if matching allele already exists in VarHold
	ALLELE::iterator it=find_if(m_Vars.begin(),m_Vars.end(),
								bind1st(pair_equal_to<std::wstring,double>(),name));

	if(it!=m_Vars.end())
		it->second=val;	// update the existing allele
	else
		m_Vars.push_back(std::make_pair<std::wstring,double>(std::wstring(name),double(val)));	// add the allele

	return val;
}

std::wstring VariablesHolder::name(int index)
{
	return (index>=0 && index<m_Vars.size())?m_Vars[index].first:std::wstring();
}

bool VariablesHolder::exists(const std::wstring& name)
{
	ALLELE::iterator it=find_if(m_Vars.begin(),m_Vars.end(),
								bind1st(pair_equal_to<std::wstring,double>(),name));
	return (it!=m_Vars.end());
}

size_t VariablesHolder::size()
{
	// is the size of m_Vars vector i.e. number of alleles in stored in the m_Vars vector
	return m_Vars.size();
}

void VariablesHolder::collate(std::vector<double>& v)
{
	for(ALLELE::iterator it=m_Vars.begin();it!=m_Vars.end();++it)
	{
		v.push_back(it->second);
	}
}

void VariablesHolder::print(FILE *pfout)
{
	for(ALLELE::iterator it=m_Vars.begin();it!=m_Vars.end();++it)
	{
		fprintf(pfout,"%s=%.5e  ",convert(it->first).c_str(),it->second);
	}
	fprintf(pfout,"\n");
}

bool VariablesHolder::fillup(std::vector<double>& v)
{
	// check if the sizes are equal
	if(v.size()!=m_Vars.size())
		return false;

	// assign allele values
	for(int i=0;i<v.size();i++)
	{
		m_Vars[i].second=v[i];
	}
	return true;
}


VirtualExperiment::VirtualExperiment():m_nResultColumn(-1),m_ReportStep(0.0),m_MaxTime(0),m_Accuracy(EPSILON),b_Error(false)
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
		// Set model variable of interest (target)
		if(!elem.GetAttribute("Variable").GetValue().size())
		{
			vx->b_Error=true;
			std::cerr << "Error: VirtualExperiment::LoadExperiment: Variable is unspecified: raising error flag: " << currentDateTime() << std::endl;
		}
		else
		{
			vx->m_Variable=elem.GetAttribute("Variable").GetValue().c_str();
		}

		// Set accuracy
        if(elem.GetAttribute("Accuracy").GetValue().size())
              vx->m_Accuracy=atof(elem.GetAttribute("Accuracy").GetValue().c_str());
       
		// Set maximum time limit on simulator (no time limit on default)
        vx->m_MaxTime=atoi(elem.GetAttribute("MaxSecondsForSimulation").GetValue().c_str());

		// ReportStep needs to be supplied by user for each VirtualExperiment
			// For convenience, 0.0 should be the default ReportStep for which ODE solver output will not be optimised
        if(!elem.GetAttribute("ReportStep").GetValue().size())
		{
			vx->b_Error=true;	// raise the error flag for unspecified attribute
			std::cerr << "Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - set to 0.0 if unknown: raising error flag: " << currentDateTime() << std::endl;
		}
		else
		{
			// Get user-specified ReportStep
			vx->m_ReportStep=atof(elem.GetAttribute("ReportStep").GetValue().c_str());
		}

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
    double SSR=0.0;		// sum of squared residuals

	// Check if estimation and experimental data are of same size
	if(d.size()!=m_Timepoints.size())
	{
		std::cerr << "Error: VirtualExperiment::getSSRD: estimation and data need to have the same size but are different: " << currentDateTime() << std::endl;
		return INFINITY;
	}

    for(int i=0;i<d.size();i++)
    {
		double sim_data = d[i].second;		// model estimate for the ith data point
		double exp_data = m_Timepoints[d[i].first].second;	// reference experimental data

		// normalisation of residual is safe due to pre-solver check for zero targets
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
		std::cerr << "Error: VirtualExperiment::LoadModel: error loading model " << model_name << ": " << currentDateTime() << std::endl;
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
	// Check the error flag
	if(b_Error)
	{
		std::cerr << "Error: VirtualExperiment::isValid: error flag is raised: " << currentDateTime() << std::endl;
		return false;
	}

	// Check for zero target value
	for(int i=0;i<m_Timepoints.size();i++)
	{
		if(m_Timepoints[i].second==0.0)
		{
			std::cerr << "Error: VirtualExperiment::isValid: invalid target value: zero target: " << currentDateTime() << std::endl;
			return false;
		}
	}

	// Check for chronological ordering
	for(int i=0;i<m_Timepoints.size();i++)
	{
		if (i && (m_Timepoints[i].first-m_Timepoints[i-1].first<0.0))
		{
			std::cerr << "Error: VirtualExperiment::isValid: assessment points are not in chronological order: " << currentDateTime() << std::endl;
			return false;
		}
	}

	// Check consistency of ReportStep with t-data
	if(m_ReportStep)
	{
		for(int i=0;i<m_Timepoints.size();i++)
		{
			double n=m_Timepoints[i].first/m_ReportStep;
			if(fabs(n-round(n))>0.1)
			{
				std::cerr << "Error: VirtualExperiment::isValid: inconsistent ReportStep " << m_ReportStep << " with time " << m_Timepoints[i].first << ": " << currentDateTime() << std::endl;
				return false;
			}
		}
	}
	else
	{
		std::cerr << "Warning: VirtualExperiment::isValid: user has supplied 0.0 ReportStep: ODE solver will automatically determine steps and cannot guarantee accurate regression analysis: " << currentDateTime() << std::endl;;
	}

	return true;
}


int VirtualExperiment::datasize() const
{
	return m_Timepoints.size();		// number of data points
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
       
	   // Set ODE solver's reporting timestep to match that of the experiment
	   if(m_ReportStep!=0.0)
		   osr->setTabulationStepControl(m_ReportStep,true);
	   else
		   osr->setTabulationStepControl(1.0,true);		// default to reporting result every 1 second

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
           int recsize=po->GetResults(vd);	// get ODE simulation result as 1D-vector and record size
 
		   // Construct model estimation vector by collating points from simulation closest to the VE data with respect to time
		   for(int i=0;i<m_Timepoints.size();i++)
		   {
			   bool b_match=false;
			   double diff;
			   double t_target=m_Timepoints[i].first;
			   int best_est;

			   // Get an estimation from simulation result
			   for(int j=0;j<vd.size();j+=recsize)
			   {
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

			   // Get m_nResultColumn from m_Variable just once per VE
			   if(m_nResultColumn<0)
			   {
				   m_nResultColumn=po->GetVariableIndex(m_Variable);
				   if(m_nResultColumn==-1)
				   {
					   std::cerr << "Error: VirtualExperiment::Evaluate: Variable " << m_Variable << " does not exist in model: " << currentDateTime() << std::endl;
					   return INFINITY;
				   }
			   }
			   results.push_back(make_pair(i,vd[best_est+m_nResultColumn]));	// add the var of interest
		   }

		   // Evaluate the squared-sum-residual of the model
		   res=((results.size()==m_Timepoints.size())?getSSRD(results):INFINITY);
	   }
       else
           res=INFINITY;	// return INF if simulation failed
    }
    catch(CellMLException e)
    {
		std::cerr << "Error: VirtualExperiment::Evaluate: error evaluating model: " << currentDateTime() << std::endl;
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
 *	Evaluate the fitness for the set of model parameters based on the average fitness from supplied VEs
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
		else
		{
			std::cerr << "Error: VEGroup::Evaluate: error in evaluating Experiment[" << i << "] with parameters: " << currentDateTime() << std::endl;
			v.print(stderr);	// print model parameters
			std::cerr << ": " << currentDateTime() << std::endl;
			return INFINITY;
		}
    }

    // Return the avg SSR obtained across all v-experiments
	return (count==experiments.size()?res/(double)count:INFINITY);
}

void VEGroup::add(VirtualExperiment *p)
{
    experiments.push_back(p);
}

void VEGroup::print_summary()
{
	printf("Experiments:\n");

	// Virtual experiment list
	for(int i=0;i<experiments.size();i++)
	{
		// Model, Variable, # assess points
		printf("Model=%s  Variable=%d  nPoints=%d\n",experiments[i]->model().c_str(),experiments[i]->resultcol(),experiments[i]->datasize());
	}
}