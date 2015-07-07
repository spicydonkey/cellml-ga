#include <mpi.h>
#include "cellml-api-cxx-support.hpp"
#include "IfaceCellML_APISPEC.hxx"
#include "IfaceCIS.hxx"
#include "CellMLBootstrap.hpp"
#include "CISBootstrap.hpp"
#include "distributor.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "AdvXMLParser.h"
#include "GAEngine.h"
#include "virtexp.h"
#include "utils.h"


using namespace std;
using namespace AdvXMLParser;

using namespace iface::cellml_api;
using namespace iface::cellml_services;


ObjRef<iface::cellml_api::CellMLBootstrap> bootstrap;	//CellML api bootstrap
ObjRef<iface::cellml_services::CellMLIntegrationService> cis;


VariablesHolder var_template;	//template for the variables, just holds names of the variables

int verbosity=0;	// verbosity initialised to 0

void usage(const char *name)
{
	std::cerr << "Error: usage: " << name << " <experiment definition xml> [-v [-v [...]]]\n";
	std::cerr << "Where -v increases the verbosity of the output" << std::endl;
}

//Open and read XML configuration file
//nSize is assigned the size of file
//return the contents of the file as a null-terminated char array
char *OpenXmlFile(const char *name,long& nSize)
{
    FILE *f=fopen(name,"rb");	// open file "name" for reading
    char *pBuffer=NULL;	// initialise a buffer for storing C-string to a nullptr

	//check for file open error
    if(!f)
        return NULL;
	
	//obtain file size
    fseek(f,0,SEEK_END);
    nSize=ftell(f);
    fseek(f,0,SEEK_SET);

	//allocate memory to contain the whole file
    pBuffer=new char[nSize+1];

    fread(pBuffer,nSize,1,f);	//copy the file into buffer (usage of fread)
    
	pBuffer[nSize]=0;	// null terminate the char array buffer
    fclose(f);

    return pBuffer;
}

// Initialise GA engine
// returns number of generations to run GA
int SetAndInitEngine(GAEngine<COMP_FUNC >& ga, const Element& elem)
{
	//Get GA parameters from XML file
    int initPopulation=atoi(elem.GetAttribute("InitialPopulation").GetValue().c_str());
    double mutation=atof(elem.GetAttribute("Mutation_proportion").GetValue().c_str());
    double cross=atof(elem.GetAttribute("Crossover_proportion").GetValue().c_str());
    int generations=atoi(elem.GetAttribute("Generations").GetValue().c_str());
    
    // Check for default and limit for params
    if(!initPopulation)
        initPopulation=100;
    if(cross>1.0)
        cross=1.0;
    if(mutation>1.0)
        mutation=1.0;
    ga.prob_cross()=cross;
    ga.prob_mutate()=mutation;
    ga.part_cross()=(int)((double)initPopulation*cross);
    ga.part_mutate()=(int)((double)initPopulation*mutation);
    
    // Read alleles information
    for(int i=0;;i++)
    {
        const Element& al=elem("Alleles",0)("Allele",i);	// get sub-Allele element in XML
        std::wstring name;
        if(al.IsNull())
           break;	// no more alleles specified
        name=convert(al.GetAttribute("Name").GetValue());
		ga.AddAllele(name);
        ga.AddLimit(name,atof(al.GetAttribute("LowerBound").GetValue().c_str()),atof(al.GetAttribute("UpperBound").GetValue().c_str()));
        var_template(name,0.0);		// update allele list in var_template
    }
    ga.set_borders(initPopulation);		// set max population of GA and initialise the population with default genomes

	// return the num of generations to run GA
    return (generations?generations:1);	// default number of generations to run is 1
}


//Initialise var_template with Alleles in XML; all param (allele) names are updated into var_template
void initialize_template_var(const Element& elem)
{
	// Read alleles information in XML
    for(int i=0;;i++)
    {
        const Element& al=elem("Alleles",0)("Allele",i);
        std::wstring name; 
        if(al.IsNull())
           break;	// no more alleles specified
        name=convert(al.GetAttribute("Name").GetValue());
        // Add this parameter as an allele in var_template
		var_template(name,0.0);
    }
}


// Observer callback to process workitem in the GA context
bool observer(WorkItem *w,double answer,void *g)
{
    GAEngine<COMP_FUNC> *ga=(GAEngine<COMP_FUNC> *)g;	// GAEngine typecasting
   
    ga->process_workitem(w,answer);		// assign 'answer' as fitness to the genome corresponding to the workitem w
    return true;
}


double do_compute(std::vector<double>& val)
{
	// fill-up the tmp's allele values with supplied data
    var_template.fillup(val);

	// Evaluate the chromosome fitness
    return VEGroup::instance().Evaluate(var_template);
}


// Slave process
// Returns only when appropriate quit command is received from the master
void run_slave(int proc)
{
    double req;
    MPI_Status stat;
    std::vector<double> data;

    var_template.collate(data);
    while(1)
    {
        //check if data is received
        MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);
        if(stat.MPI_TAG==TAG_QUIT)
        {
            //Quit signal received
            break;
        }
        //Receive compute request and process it
        MPI_Recv(&data[0],data.size(),MPI_DOUBLE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);
        req=do_compute(data);
        //returns the result of the computations
        MPI_Send(&req,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
    }
}


int main(int argc,char *argv[])
{
    char *pBuffer=NULL;
    long nSize=0;
    GAEngine<COMP_FUNC > ga;	// initialise GA engine for the program
	int proc,nproc;
    int generations=1;
    const char *filename=NULL;

    MPI_Init(&argc,&argv);

    if(argc<2)
    {
		// warn user for incorrect usage at command line
        usage(argv[0]);
        return -1;
    }

    for(int i=1;i<argc;i++)
    {
        if(!strcmp(argv[i],"-v"))
			// if an arg string is "-v" increment verbosity
            verbosity++;
        else
			// other arg string becomes the filename
            filename=argv[i];
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &proc);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

	srand(time(NULL)*proc);		// unique seed for RNG

    //Load and initialise CellML API
    bootstrap=CreateCellMLBootstrap();
    cis=CreateIntegrationService();

	// Read input file and store contents in buffer
    if((pBuffer=OpenXmlFile(filename,nSize)) == NULL)
    {
		std::cerr << "Error: main: error opening input file " << argv[1] << ": " << currentDateTime() << std::endl;
		return -1;
    }
	// Read success: pBuffer is a C-string containing file and nSize is size of file

    //Load the experiments package: Virtual Experiment data and GA parameters
    try
    {
        Parser parser;

        ObjRef<CellMLComponentSet> comps;	// TODO comps not referenced elsewhere in project

		// Parse the XML contents in buffer
        auto_ptr<Document> pDoc(parser.Parse(pBuffer,nSize));	// can throw an exception
		// Get the root of the XML structure
        const Element& root=pDoc->GetRoot();

		// Load virtual experiments
		// load all virtual experiments in the XML file
		for(int i=0;;i++)
        {
			// load the VE data in file
            VirtualExperiment *vx=VirtualExperiment::LoadExperiment(root("VirtualExperiments",0)("VirtualExperiment",i));
			if(!vx)
               break;	// loaded all the VE in file
			
			// Quit program with err msg if any VE is invalid
			if (!vx->isValid())
				return -1;

			// add each VE into the group singleton
            VEGroup::instance().add(vx);
        }
		
		// load the GA parameters from file and initialise the engine
        if(!proc)
        {
			// Master processor initialises the GA engine
			// get max generations and GA parameters
            generations=SetAndInitEngine(ga,root("GA",0));
        }
        else
        {
			// The slaves initialise the template variable holder to create gene-pool
            initialize_template_var(root("GA",0));
        }
    }
    catch(ParsingException e)
    {
        //printf("Parsing error at line %d\n",e.GetLine());
		std::cerr << "Error: main: parsing error at line " << e.GetLine() << std::endl;
    }
    delete [] pBuffer;	// free memory used to store file

	// Wait until all the clients are ready
    MPI_Barrier(MPI_COMM_WORLD);

    // Only master task needs GA engine to be initialised and used   
    if(!proc)
    {
        VariablesHolder v;	// storage for the best chromosome

		// Initialise the population in GA engine
        ga.Initialise();
		// Run GA
        ga.RunGenerations(generations);
        
		// Print best genome from the run
		double bf=ga.GetBest(v);
		std::cout << "==========================================\n";
		fprintf(stdout,"BEST GENOME (%lf):\n",bf);
		v.print(stdout);
		std::cout << "==========================================\n";
		
        Distributor::instance().finish();
    }
    else
    {
        run_slave(proc);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
