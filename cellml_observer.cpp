#include "cellml_observer.h"


LocalProgressObserver::LocalProgressObserver(iface::cellml_services::CellMLCompiledModel* aCCM):
	mRefcount(1),bFinished(false),bFailed(false)
{
	mCCM = aCCM;
	mCCM->add_ref();
	mCI = mCCM->codeInformation();

	iface::cellml_services::ComputationTargetIterator* cti = mCI->iterateTargets();

	while(true)
	{
		iface::cellml_services::ComputationTarget* ct = cti->nextComputationTarget();
		if(ct == NULL)
			break;
		if((ct->type() == iface::cellml_services::STATE_VARIABLE ||
			ct->type() == iface::cellml_services::ALGEBRAIC ||
			ct->type() == iface::cellml_services::VARIABLE_OF_INTEGRATION) &&
			ct->degree() == 0)
		{
			iface::cellml_api::CellMLVariable* source = ct->variable();
			std::string n = convert(source->name());

			source->release_ref();
		}
		ct->release_ref();
	}
	cti->release_ref();
}

LocalProgressObserver::~LocalProgressObserver()
{
	mCCM->release_ref();
	mCI->release_ref();
}

void LocalProgressObserver::add_ref()
throw(std::exception&)
{
	mRefcount++;
}

void LocalProgressObserver::release_ref()
throw(std::exception&)
{
	mRefcount--;
	if(mRefcount == 0)
		delete this;
}

std::string LocalProgressObserver::objid()
throw (std::exception&)
{
	return std::string("singletonLocalProgressObserver");
}

void* LocalProgressObserver::query_interface(const std::string& iface)
throw (std::exception&)
{
	if(iface=="xpcom::IObject")
		return static_cast<::iface::XPCOM::IObject*>(this);
	else if(iface=="cellml_services::IntegrationProgressObserver")
		return static_cast<::iface::cellml_services::IntegrationProgressObserver*>(this);
	return NULL;
}

void LocalProgressObserver::computedConstants(const std::vector<double>& results)
throw (std::exception&)
{
#if 0
	iface::cellml_services::ComputationTargetIterator* cti =
		mCI->iterateTargets();
	while (true)
	{
		iface::cellml_services::ComputationTarget* ct = cti->nextComputationTarget();
		if (ct == NULL)
			break;
		if (ct->type() == iface::cellml_services::CONSTANT &&
			ct->degree() == 0)
		{
			iface::cellml_api::CellMLVariable* source = ct->variable();
			std::string n = convert(source->name());
			source->release_ref();
			printf("# Computed constant: %S = %e\n", n.c_str(), values[ct->assignedIndex()]);
		}
		ct->release_ref();
	}
	cti->release_ref();
#endif
}

void LocalProgressObserver::results(const std::vector<double>& results)
throw (std::exception&)
{
	m_Results.insert(m_Results.end(),results.begin(),results.end());
}

int LocalProgressObserver::GetVariableIndex(std::string& variable)
{
	int index;
	uint32_t aic = mCI->algebraicIndexCount();
	uint32_t ric = mCI->rateIndexCount();

	iface::cellml_services::ComputationTargetIterator* cti = mCI->iterateTargets();

	while(true)
	{
		iface::cellml_services::ComputationTarget* ct = cti->nextComputationTarget();

		// Target out of range
		if(ct == NULL)
		{
			std::cerr << "Error: LocalProgressObserver::GetVariableIndex: Could not find the variable " << variable << " in the model: " << currentDateTime() << std::endl;
			index=-1;
			break;
		}

		// Found a match with target variable name
		if(variable==convert(ct->variable()->name()))
		{
			index=ct->assignedIndex();	// get the variable index (type specific)

			// Evaluate the type-dependent shift
			int type_shift;
			if(ct->type() == iface::cellml_services::STATE_VARIABLE)
			{
				// get degree of the state variable
				if(ct->degree() == 0)
					type_shift=1;
				else if(ct->degree() == 1)
					type_shift=1+ric;
				else
				{
					std::cerr << "Error: LocalProgressObserver::GetVariableIndex: state variable " << variable << " has >1 degree: " << currentDateTime() << std::endl;
					index=-1;
					break;
				}
			}
			else if(ct->type() == iface::cellml_services::ALGEBRAIC)
				type_shift=1+2*ric;
			else if(ct->type() == iface::cellml_services::VARIABLE_OF_INTEGRATION)
			{
				std::cerr << "Error: LocalProgressObserver::GetVariableIndex: VOI " << variable << " detected as target: " << currentDateTime() << std::endl;
				index=0;
				break;
			}
			else
			{
				std::cerr << "Error: LocalProgressObserver::GetVariableIndex: Variable " << variable << " must be either VOI, State/rate or algebraic: " << currentDateTime() << std::endl;
				index=-1;
				break;
			}

			index+=type_shift;	// apply the shift
			ct->release_ref();
			break;
		}
		ct->release_ref();
	}
	cti->release_ref();

	return index;
}

int LocalProgressObserver::GetResults(std::vector<double>& res)
{
	uint32_t aic = mCI->algebraicIndexCount();
	uint32_t ric = mCI->rateIndexCount();
	uint32_t recsize = 2 * ric + aic + 1;

	if(recsize == 0)
		return 0;
	res.assign(m_Results.begin(),m_Results.end());
	return recsize;
}

void LocalProgressObserver::done()
throw (std::exception&)
{
	bFinished = true;
}

void LocalProgressObserver::failed(const std::string& errmsg)
throw (std::exception&)
{
	fprintf(stderr,"# Integration failed (%s)\n",errmsg.c_str());
	bFinished = true;
	bFailed=true;
}

