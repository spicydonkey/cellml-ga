// Sequential version

#include <unistd.h>
#include <stdlib.h>
#include "virtexp.h"
#include "AdvXMLParser.h"
#include "utils.h"
#include <math.h>

#include "GATESTER.h" // test functions 


using namespace std;
using namespace AdvXMLParser;


#define EPSILON 0.01


VirtualExperiment::VirtualExperiment():m_nResultColumn(-1),m_ReportStep(0.0),m_MaxTime(0),m_Accuracy(EPSILON)
{
}

VirtualExperiment::~VirtualExperiment()
{
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

double VEGroup::Evaluate(VariablesHolder& v)
{
	// Get back the vector<double> of params
	std::vector<double> params;
	v.collate(params);

	int test = v.test();
	// Evaluate and return the result of a test function
	switch (test) {
		case 1:
			// multi-dim Schwefel function
			return schwefel(params);
		case 2:
			// shifted multi-dim Schwefel function
			return sh_schwefel(params);
		case 3:
			// multi-dim Schwefel with infinite region
			return inf_schwefel(params);
		case 4:
			// negative valued Schwefel function
			return neg_schwefel(params);
		case 5:
			// multi-dim Schwefel with v large region (not inf)
			return lge_schwefel(params);
	}
}

void VEGroup::add(VirtualExperiment *p)
{
    experiments.push_back(p);
}

