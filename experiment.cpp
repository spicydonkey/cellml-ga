// Sequential version 
/*
	Test GA against various test functions in GATESTER.h
	No parallel processing
	No call to CellML
*/

#include <map>
#include <string>
#include <string.h>
#include <cfloat>
#include "processor.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "AdvXMLParser.h"
#include "GAEngine.h"
#include "virtexp.h"


using namespace std;
using namespace AdvXMLParser;


//If SUPPORT_BLOCK_SAMPLING is defined
//the selection algorithms may use either
//probabilistic search or blocking search
//#define SUPPORT_BLOCK_SAMPLING


VariablesHolder var_template; //template for the variables, just holds names of the variables

int verbosity=0;	// verbosity initialised to 0

void usage(const char *name)
{
    printf("Usage: %s <experiment definition xml> [-v [-v]]\n",name);
    printf("Where -v increases the verbosity of the output\n");
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
	//fread(pBuffer,1,nSize,f);
    
	pBuffer[nSize]=0;	// null terminate the char array buffer
    fclose(f);

    return pBuffer;
}

//Initialise GA engine
	//return number of generations to run GA
int SetAndInitEngine(GAEngine<COMP_FUNC >& ga, const Element& elem)
{
	//Get GA parameters from XML file
    int initPopulation=atoi(elem.GetAttribute("InitialPopulation").GetValue().c_str());
    double mutation=atof(elem.GetAttribute("Mutation_proportion").GetValue().c_str());
    double cross=atof(elem.GetAttribute("Crossover_proportion").GetValue().c_str());
    int generations=atoi(elem.GetAttribute("Generations").GetValue().c_str());
    int block_sample=atoi(elem.GetAttribute("Sampling").GetValue().c_str());
    

    //Set the parameters for the GA engine accordingly
    if(!initPopulation)
        initPopulation=100;		// default population size if in XML it is undeclared or 0
    if(cross>1.0)
        cross=1.0;
    if(mutation>1.0)
        mutation=1.0;
    ga.prob_cross()=cross;
    ga.prob_mutate()=mutation;
    ga.part_cross()=(int)((double)initPopulation*cross);
    ga.part_mutate()=(int)((double)initPopulation*mutation);
    
    //Read alleles information
    for(int i=0;;i++)
    {
        const Element& al=elem("Alleles",0)("Allele",i);		//AdvXMLParser// assign to "al" the ith Allele sub-element of the (first) Alleles element in "elem"
        std::wstring name;
        if(al.IsNull())
           break;	// exhaustively iterate through the "Allele" sub-elements of Alleles
        name=convert(al.GetAttribute("Name").GetValue());	// get the String value for Name attribute in "al" allele sub-element; convert it to wstring and store in name
        // Add allele
		ga.AddAllele(name);	
        // Set allele limits
        ga.AddLimit(name,atof(al.GetAttribute("LowerBound").GetValue().c_str()),atof(al.GetAttribute("UpperBound").GetValue().c_str()));
        // Initialise variable template
        var_template(name,0.0);
    }
    ga.set_borders(initPopulation);		// set max population of GA

	// return the num of generations to run GA
    return (generations?generations:1);	// default number of generations to run is 1
}


//Initialise var_template with Alleles in XML; all param names are updated into var_template
void initialize_template_var(const Element& elem)	//AdvXMLParser
{
    for(int i=0;;i++)
    {
        const Element& al=elem("Alleles",0)("Allele",i);	// assign to "al" the ith Allele sub-element of the (first) Alleles element in "elem"
        std::wstring name; 
        if(al.IsNull())
           break;	// exhaustively iterate through the "Allele" sub-elements of Alleles
        name=convert(al.GetAttribute("Name").GetValue());	// get the String value for Name attribute in "al" allele sub-element; convert it to wstring and store in name
        // Add this parameter as an allele in var_template
		var_template(name,0.0);
    }
}


//Observer callback
bool observer(WorkItem *w,double answer,void *g)
{
    GAEngine<COMP_FUNC> *ga=(GAEngine<COMP_FUNC> *)g;	// type casting void pointer appropriately
   
    ga->process_workitem(w,answer);
    return true;
}


// do_compute	[processor.cpp]
// Call a virtual simulation (i.e. test functions) to assign genome's fitness
double do_compute(std::vector<double>& val)
{
	// fill-up the tmp's allele values with supplied data
	var_template.fillup(val);
	// Virtual simulation: evaluate this chromosome's fitness
	return VEGroup::instance().Evaluate(var_template);
}


int main(int argc,char *argv[])
{
    char *pBuffer=NULL;
    long nSize=0;
    GAEngine<COMP_FUNC > ga;	// initialise GA engine for the program
    int generations=1;
    const char *filename=NULL;

    srand(time(NULL));	// seed the RNG

    if(argc<2)
    {
		// warn user for incorrect usage of command
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

	// Read input file and store contents in buffer
    if((pBuffer=OpenXmlFile(filename,nSize)) == NULL)
    {
		fprintf(stderr,"Error opening input file %s\n",argv[1]);
		return -1;
    }
	// Read success: pBuffer is a C-string containing file and nSize is size of file

    //Load the experiments package: Virtual Experiment data and GA parameters
    try
    {
        Parser parser;

		// Parse the XML contents in buffer
        auto_ptr<Document> pDoc(parser.Parse(pBuffer,nSize));	// can throw an exception
		// Get the root of the XML structure
        const Element& root=pDoc->GetRoot();

		// Load virtual experiments
		// List of test functions and their call number at VEGroup::Evaluate
		std::map<std::string,int> testFunctions;
		testFunctions["schwefel"]=1;
		testFunctions["shifted"]=2;
		testFunctions["inf_schwefel"]=3;
		testFunctions["neg_schwefel"]=4;
		testFunctions["lge_schwefel"]=5;

		// get test function specified in XML file
		string testName=(root("VirtualExperiments",0)("VirtualExperiment",0)).GetAttribute("TestFunction").GetValue();
		var_template.test() = testFunctions[testName];	// assign corresponding test function to the variable template
		
		// load the GA parameters from file and initialise the engine
		generations=SetAndInitEngine(ga,root("GA",0));		// load the GA params and initialise the engine
		initialize_template_var(root("GA",0));		// initialise var_template with alleles
    }
    catch(ParsingException e)
    {
        printf("Parsing error at line %d\n",e.GetLine());
    }
    delete [] pBuffer;	// free memory used to store file

    VariablesHolder v;	// storage for the best chromosome

	//Initialise the population in GA engine
    ga.Initialise();
	//Run GA
    ga.RunGenerations(generations);
        
	// Best genome from the run
	double bf=ga.GetBest(v);
	printf("Best fitness: %lf\n",bf);
    for(int i=0;;i++)
    {
        wstring name=v.name(i);
        if(!name.size())
            break;
        printf("Best[%s]=%lf\n",convert(name).c_str(),v(name));
    }

    return 0;
}