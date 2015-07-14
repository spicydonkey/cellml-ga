#ifndef VIRTUAL_EXPERIMENT_H
#define VIRTUAL_EXPERIMENT_H
#include <stddef.h>
#include "cellml-api-cxx-support.hpp"
#include "IfaceCellML_APISPEC.hxx"
#include "CellMLBootstrap.hpp"
#include "CISBootstrap.hpp"
#include "AdvXMLParser.h"
#include "utils.h"
#include <string>
#include <functional>
#include <algorithm>

// DEBUG macro
//#define DEBUG_BUILD

// COMP_FUNC is a function object class for <= comparisons on doubles
#define COMP_FUNC std::less_equal<double>

// define ALLELE as vector of <wstring, double> pairs ( i.e. ALLELE is a vector of 'allele's (pair<wstr,doub>) )
typedef std::vector<std::pair<std::wstring,double> > ALLELE;


/**
 *	VariablesHolder
 *		m_Vars : a storage for ALLELE variable
 **/
class VariablesHolder
{
	private:
		ALLELE m_Vars;	//the member that stores alleles in a chromosome form ( i.e. vector <allele=pair<wstring allele_name,double allele_value> > )

	public:
		VariablesHolder();
		VariablesHolder(const VariablesHolder& other);	//copy other VarHolder's variables into this one
		~VariablesHolder();

		VariablesHolder& operator=(const VariablesHolder& other);

		// indexing an allele's value stored by its name
			// 0.0 returned if an allele by the supplied name does not exist
		double operator()(const std::wstring& name);

		// Update an allele in m_Var member and return updated allele value
		double operator()(const std::wstring& name,double val);

		// Search allele by index and return its name, if any
		std::wstring name(int index);

		// Existence of allele of given name
		bool exists(const std::wstring& name);

		// Size of a VariablesHolder object (number of alleles stored)
		size_t size();

		// Collate the genome sequence into a vector
		void collate(std::vector<double>& v);

		// print all alleles held in m_Vars to file
		void print(FILE *pfout);

		// Fill-up the chromosome with a supplied vector of allele values. Return true iff executed correctly
		bool fillup(std::vector<double>& v);
};


/**
 *	VirtualExperiment
 *	
 *	
 *	
 **/
class VirtualExperiment
{
    public:
        VirtualExperiment();
        ~VirtualExperiment();

		// Load the CellML model and return if the load was successful
        bool LoadModel(const std::string& model_name);

		// Load the virtual experiment data from XML file and return a pointer to the constructed VE object (NULL ptr if error)
        static VirtualExperiment *LoadExperiment(const AdvXMLParser::Element& elem);

		// Load the CellML model parameters onto m_Model
		void SetVariables(VariablesHolder& v);

		// Set model constants with parameters stored in a VariablesHolder object
        void SetParameters(VariablesHolder& v);

		// Evaluate the parameter-configured CellML model fitness to the experimental data
        double Evaluate();

		// Check for invalid VE settings
		bool isValid();

		// Get CellML model name
		std::string model() const { return m_strModelName; }

		// Get data size
		int datasize() const;

        int resultcol() const { return m_nResultColumn; }
        void resultcol(int r) { m_nResultColumn=r; }

        unsigned long maxtime() const { return m_MaxTime; }
        void maxtime(unsigned long m) { m_MaxTime=m; }

        double accuracy() const { return m_Accuracy; }
        void accuracy(double a) { m_Accuracy=a; }

        void Run();

	private:
        struct Runner
        {
           Runner(VirtualExperiment *p):pOwner(p) {}
           double operator()(VariablesHolder& v);

           VirtualExperiment *pOwner;
        };
		//friend class VEGroup;
        friend class Runner;

		// Calculate the normalised Sum of Square Residuals (SSR) of the simulation against v-experimental data
		double getSSRD(std::vector<std::pair<int,double> >& d);
        
		std::string m_strModelName;

        ObjRef<iface::cellml_api::Model> m_Model;	// CellML model corresponding to this experiment
	
		int m_nResultColumn;			// indicator for the variable of interest 
        
		typedef std::map<std::wstring,double>	PARAMS;
        typedef std::pair<double,double>		POINT;
        typedef std::vector<POINT>				TIMEPOINTS;

        PARAMS m_Parameters;			// parameters to be held constant in simulations of mathematical models
        TIMEPOINTS m_Timepoints;		// object for storing empirical data for model optimisation
        double m_ReportStep;
        unsigned long m_MaxTime;		// maximum time limit (sec) given to the solver
        double m_Accuracy;

		bool b_Error;		// error indicator
};


//Virtual experiment group handler
class VEGroup
{
    private:
        VEGroup();
        ~VEGroup();

    public:
		// get the singleton VE group object
        static VEGroup& instance();

		// Evaluate the set of parameters in characterising models as approximations of data in VEs
        double Evaluate(VariablesHolder& v);

		// Add a VE object onto experiments
		void add(VirtualExperiment *p);

		// Print summary
		void print_summary();

    protected:
        typedef std::vector<VirtualExperiment *> VE;
        
		// a vector of pointers to virtual experiments
		VE experiments;
};

#endif

