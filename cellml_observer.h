//Class LocalProgressObserver implements
//observer role for CellML API purposes
#ifndef CELLML_OBSERVER_H
#define CELLML_OBSERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cellml-api-cxx-support.hpp"
#include "IfaceCellML_APISPEC.hxx"
#include "CellMLBootstrap.hpp"
#include <string>
#include "utils.h"


class LocalProgressObserver:public iface::cellml_services::IntegrationProgressObserver
{
public:
  LocalProgressObserver(iface::cellml_services::CellMLCompiledModel* aCCM);

  ~LocalProgressObserver();

  void add_ref() throw(std::exception&);

  void release_ref() throw(std::exception&);

  std::string objid() throw (std::exception&);

  void* query_interface(const std::string& iface) throw (std::exception&);

  virtual std::vector<std::string> supported_interfaces()
	  throw (std::exception&)
  {
	  std::vector<std::string> v;

	  v.push_back("xpcom::IObject");
	  v.push_back("cellml_services::IntegrationProgressObserver");
	  return v;
  }

  void computedConstants(const std::vector<double>& results) throw (std::exception&);

  void results(const std::vector<double>& results) throw (std::exception&);

  // Get index of the variable in the sequence of result entries, if it exists. Return -1 for errors
  int GetVariableIndex(std::string& variable);

  //Public interface to the observer data
  //
  //results of the computations are returned in res vector
  int GetResults(std::vector<double>& res);

  //Marks computation process as finished
  void done() throw (std::exception&);

  //Marks computation process as finished
  //and prints out error message
  void failed(const std::string& errmsg) throw (std::exception&);

  //true if compute is done
  bool finished() const { return bFinished; }
  bool failed() const { return bFailed; }

private:
  iface::cellml_services::CellMLCompiledModel* mCCM;
  iface::cellml_services::CodeInformation* mCI;
  uint32_t mRefcount;
  bool bFinished;
  bool bFailed;
  std::vector<double> m_Results;
};

#endif

