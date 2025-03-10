// @(#)root/roostats:$Id:  cranmer $
// Author: Kyle Cranmer, George Lewis
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

////////////////////////////////////////////////////////////////////////////////

/** \class ParamHistFunc
 * \ingroup HistFactory
 *   A class which maps the current values of a RooRealVar
 *  (or a set of RooRealVars) to one of a number of RooAbsReal
 *  (nominally RooRealVar):
 *
 *  `ParamHistFunc: {val1, val2, ...} -> {gamma (RooRealVar)}`
 *
 *  The intended interpretation is that each parameter in the
 *  range represent the height of a bin over the domain
 *  space.
 *
 *  The 'createParamSet' is an easy way to create these
 *  parameters from a set of observables. They are
 *  stored using the "TH1" ordering convention (as compared
 *  to the RooDataHist convention, which is used internally
 *  and one must map between the two).
 *
 *  All indices include '0':<br>
 *  \f$ \gamma_{i,j} \f$ = `paramSet[ size(i)*j + i ]`
 *
 *  ie assuming the dimensions are 5*5:<br>
 *  \f$ \gamma_{2,1} \f$ = `paramSet[ 5*1 + 2 ] = paramSet[7]`
 */


#include "RooStats/HistFactory/ParamHistFunc.h"

#include "RooConstVar.h"
#include "RooBinning.h"
#include "RooErrorHandler.h"
#include "RooArgSet.h"
#include "RooMsgService.h"
#include "RooRealVar.h"
#include "RooArgList.h"
#include "RooWorkspace.h"

#include "TH1.h"

#include <array>
#include <sstream>
#include <stdexcept>
#include <iostream>



////////////////////////////////////////////////////////////////////////////////

ParamHistFunc::ParamHistFunc()
  : _normIntMgr(this)
{
  _dataSet.removeSelfFromDir(); // files must not delete _dataSet.
}


////////////////////////////////////////////////////////////////////////////////
/// Create a function which returns binewise-values
/// This class contains N RooAbsReals's, one for each
/// bin from the given RooRealVar.
///
/// The value of the function in the ith bin is
/// given by:
///
/// F(i) = gamma_i * nominal(i)
///
/// Where the nominal values are simply fixed
/// numbers (default = 1.0 for all i)
ParamHistFunc::ParamHistFunc(const char* name, const char* title,
              const RooArgList& vars, const RooArgList& paramSet) :
  RooAbsReal(name, title),
  _normIntMgr(this),
  _dataVars("!dataVars","data Vars",       this),
  _paramSet("!paramSet","bin parameters",  this),
  _dataSet( (std::string(name)+"_dataSet").c_str(), "", vars)
{

  // Create the dataset that stores the binning info:

  //  _dataSet = RooDataSet("

  _dataSet.removeSelfFromDir(); // files must not delete _dataSet.

  // Set the binning
  // //_binning = var.getBinning().clone() ;

  // Create the set of parameters
  // controlling the height of each bin

  // Get the number of bins
  _numBins = GetNumBins( vars );

  // Add the parameters (with checking)
  _dataVars.addTyped<RooRealVar>(vars);
  addParamSet( paramSet );
}


////////////////////////////////////////////////////////////////////////////////
/// Create a function which returns bin-wise values.
/// This class allows to multiply bin contents of histograms
/// with the values of a set of RooAbsReal.
///
/// The value of the function in the ith bin is
/// given by:
/// \f[
///   F(i) = \gamma_{i} * \mathrm{nominal}(i)
/// \f]
///
/// Where the nominal values are taken from the histogram,
/// and the \f$ \gamma_{i} \f$ can be set from the outside.
ParamHistFunc::ParamHistFunc(const char* name, const char* title,
              const RooArgList& vars, const RooArgList& paramSet,
              const TH1* Hist ) :
  RooAbsReal(name, title),
  _normIntMgr(this),
  //  _dataVar("!dataVar","data Var", this, (RooRealVar&) var),
  _dataVars("!dataVars","data Vars",       this),
  _paramSet("!paramSet","bin parameters",  this),
  _dataSet( (std::string(name)+"_dataSet").c_str(), "", vars, Hist)
{

  _dataSet.removeSelfFromDir(); // files must not delete _dataSet.

  // Get the number of bins
  _numBins = GetNumBins( vars );

  // Add the parameters (with checking)
  _dataVars.addTyped<RooRealVar>(vars);
  addParamSet( paramSet );
}


Int_t ParamHistFunc::GetNumBins( const RooArgSet& vars ) {

  // A helper method to get the number of bins

  if( vars.empty() ) return 0;

  Int_t numBins = 1;

  for (auto comp : vars) {
    if (!dynamic_cast<RooRealVar*>(comp)) {
      auto errorMsg = std::string("ParamHistFunc::GetNumBins") + vars.GetName() + ") ERROR: component "
                      + comp->GetName() + " in vars list is not of type RooRealVar";
      oocoutE(nullptr, InputArguments) <<  errorMsg << std::endl;
      throw std::runtime_error(errorMsg);
    }
    auto var = static_cast<RooRealVar*>(comp);

    Int_t varNumBins = var->numBins();
    numBins *= varNumBins;
  }

  return numBins;
}


////////////////////////////////////////////////////////////////////////////////

ParamHistFunc::ParamHistFunc(const ParamHistFunc& other, const char* name) :
  RooAbsReal(other, name),
  _normIntMgr(other._normIntMgr, this),
  _dataVars("!dataVars", this, other._dataVars ),
  _paramSet("!paramSet", this, other._paramSet),
  _numBins( other._numBins ),
  _dataSet( other._dataSet )
{
  _dataSet.removeSelfFromDir(); // files must not delete _dataSet.

  // Copy constructor
  // Member _ownedList is intentionally not copy-constructed -- ownership is not transferred
}


////////////////////////////////////////////////////////////////////////////////
/// Get the parameter associated with the index.
/// The index follows RooDataHist indexing conventions.
/// It uses the binMap to convert the RooDataSet style index
/// into the TH1 style index (which is how they are stored
/// internally in the '_paramSet' vector).
RooAbsReal& ParamHistFunc::getParameter( Int_t index ) const {

  auto const& n = _numBinsPerDim;

  // check if _numBins needs to be filled
  if(n.x == 0) {
    _numBinsPerDim = getNumBinsPerDim(_dataVars);
  }

  // Unravel the index to 3D coordinates. We can't use the index directly,
  // because in the parameter set the dimensions are ordered in reverse order
  // compared to the RooDataHist (for historical reasons).
  const int i = index / n.yz;
  const int tmp = index % n.yz;
  const int j = tmp / n.z;
  const int k = tmp % n.z;

  const int idx = i + j * n.x + k * n.xy;
  if (idx >= _numBins) {
    throw std::runtime_error("invalid index");
  }
  return static_cast<RooAbsReal &>(_paramSet[idx]);
}


////////////////////////////////////////////////////////////////////////////////

RooAbsReal& ParamHistFunc::getParameter() const {
  Int_t index = getCurrentBin();
  return getParameter( index );
}


void ParamHistFunc::setParamConst( Int_t index, bool varConst ) {
  RooAbsReal& var = getParameter( index );
  var.setAttribute( "Constant", varConst );
}


void ParamHistFunc::setConstant( bool constant ) {
  for( int i=0; i < numBins(); ++i) {
    setParamConst(i, constant);
  }
}


////////////////////////////////////////////////////////////////////////////////

void ParamHistFunc::setShape( TH1* shape ) {
  int num_hist_bins = shape->GetNbinsX()*shape->GetNbinsY()*shape->GetNbinsZ();

  if( num_hist_bins != numBins() ) {
    std::cout << "Error - ParamHistFunc: cannot set Shape of ParamHistFunc: " << GetName()
         << " using histogram: " << shape->GetName()
         << ". Bins don't match" << std::endl;
    throw std::runtime_error("setShape");
  }


  Int_t TH1BinNumber = 0;
  for( Int_t i = 0; i < numBins(); ++i) {

    TH1BinNumber++;

    while( shape->IsBinUnderflow(TH1BinNumber) || shape->IsBinOverflow(TH1BinNumber) ){
      TH1BinNumber++;
    }

    RooRealVar* param = dynamic_cast<RooRealVar*>(&_paramSet[i]);
    if(!param) {
       std::cout << "Error - ParamHisFunc: cannot set Shape of ParamHistFunc: " << GetName()
                 << " - param is not RooRealVar" << std::endl;
       throw std::runtime_error("setShape");
    }
    param->setVal( shape->GetBinContent(TH1BinNumber) );
  }

}


////////////////////////////////////////////////////////////////////////////////
/// Create the list of RooRealVar
/// parameters which represent the
/// height of the histogram bins.
/// The list 'vars' represents the
/// observables (corresponding to histogram bins)
/// that these newly created parameters will
/// be mapped to. (ie, we create one parameter
/// per observable in vars and per bin in each observable)

/// Store them in a list using:
/// _paramSet.add( createParamSet() );
/// This list is stored in the "TH1" index order
RooArgList ParamHistFunc::createParamSet(RooWorkspace& w, const std::string& Prefix,
                const RooArgList& vars) {


  // Get the number of bins
  // in the nominal histogram

  RooArgList paramSet;

  Int_t numVars = vars.size();
  Int_t numBins = GetNumBins( vars );

  if( numVars == 0 ) {
    std::cout << "Warning - ParamHistFunc::createParamSet() :"
    << " No Variables provided.  Not making constraint terms."
    << std::endl;
    return paramSet;
  }

  else if( numVars == 1 ) {

    // For each bin, create a RooRealVar
    for( Int_t i = 0; i < numBins; ++i) {

      std::stringstream VarNameStream;
      VarNameStream << Prefix << "_bin_" << i;
      std::string VarName = VarNameStream.str();

      RooRealVar gamma( VarName.c_str(), VarName.c_str(), 1.0 );
      // "Hard-Code" a minimum of 0.0
      gamma.setMin( 0.0 );
      gamma.setConstant( false );

      w.import( gamma, RooFit::RecycleConflictNodes() );

      paramSet.add(*w.arg(VarName));
    }
  }

  else if( numVars == 2 ) {

    // Create a vector of indices
    // all starting at 0
    std::vector< Int_t > Indices(numVars, 0);

    RooRealVar* varx = static_cast<RooRealVar*>(vars.at(0));
    RooRealVar* vary = static_cast<RooRealVar*>(vars.at(1));

    // For each bin, create a RooRealVar
    for( Int_t j = 0; j < vary->numBins(); ++j) {
      for( Int_t i = 0; i < varx->numBins(); ++i) {

        // Ordering is important:
        // To match TH1, list goes over x bins
        // first, then y

        std::stringstream VarNameStream;
        VarNameStream << Prefix << "_bin_" << i << "_" << j;
        std::string VarName = VarNameStream.str();

        RooRealVar gamma( VarName.c_str(), VarName.c_str(), 1.0 );
        // "Hard-Code" a minimum of 0.0
        gamma.setMin( 0.0 );
        gamma.setConstant( false );

        w.import( gamma, RooFit::RecycleConflictNodes() );

        paramSet.add(*w.arg(VarName));
      }
    }
  }

  else if( numVars == 3 ) {

    // Create a vector of indices
    // all starting at 0
    std::vector< Int_t > Indices(numVars, 0);

    RooRealVar* varx = static_cast<RooRealVar*>(vars.at(0));
    RooRealVar* vary = static_cast<RooRealVar*>(vars.at(1));
    RooRealVar* varz = static_cast<RooRealVar*>(vars.at(2));

    // For each bin, create a RooRealVar
    for( Int_t k = 0; k < varz->numBins(); ++k) {
      for( Int_t j = 0; j < vary->numBins(); ++j) {
        for( Int_t i = 0; i < varx->numBins(); ++i) {

          // Ordering is important:
          // To match TH1, list goes over x bins
          // first, then y, then z

          std::stringstream VarNameStream;
          VarNameStream << Prefix << "_bin_" << i << "_" << j << "_" << k;
          std::string VarName = VarNameStream.str();

          RooRealVar gamma( VarName.c_str(), VarName.c_str(), 1.0 );
          // "Hard-Code" a minimum of 0.0
          gamma.setMin( 0.0 );
          gamma.setConstant( false );

          w.import( gamma, RooFit::RecycleConflictNodes() );

          paramSet.add(*w.arg(VarName));
        }
      }
    }
  }

  else {
    std::cout << " Error: ParamHistFunc doesn't support dimensions > 3D " <<  std::endl;
  }

  return paramSet;

}


////////////////////////////////////////////////////////////////////////////////
/// Create the list of RooRealVar parameters which scale the
/// height of histogram bins.
/// The list `vars` represents the observables (corresponding to histogram bins)
/// that these newly created parameters will
/// be mapped to. *I.e.*, we create one parameter
/// per observable in `vars` and per bin in each observable.
///
/// The new parameters are initialised to 1 with an uncertainty of +/- 1.,
/// their range is set to the function arguments.
///
/// Store the parameters in a list using:
/// ```
/// _paramSet.add( createParamSet() );
/// ```
/// This list is stored in the "TH1" index order.
RooArgList ParamHistFunc::createParamSet(RooWorkspace& w, const std::string& Prefix,
                const RooArgList& vars,
                double gamma_min, double gamma_max) {



  RooArgList params = ParamHistFunc::createParamSet( w, Prefix, vars );

  for (auto comp : params) {
    // If the gamma is subject to a preprocess function, it is a RooAbsReal and
    // we don't need to set the range.
    if(auto var = dynamic_cast<RooRealVar*>(comp)) {
      var->setMin( gamma_min );
      var->setMax( gamma_max );
    }
  }

  return params;

}


////////////////////////////////////////////////////////////////////////////////
/// Create the list of RooRealVar
/// parameters which represent the
/// height of the histogram bins.
/// Store them in a list
RooArgList ParamHistFunc::createParamSet(const std::string& Prefix, Int_t numBins,
                double gamma_min, double gamma_max) {

  // Get the number of bins
  // in the nominal histogram

  RooArgList paramSet;

  if( gamma_max <= gamma_min ) {

    std::cout << "Warning: gamma_min <= gamma_max: Using default values (0, 10)" << std::endl;

    gamma_min = 0.0;
    gamma_max = 10.0;

  }

  double gamma_nominal = 1.0;

  if( gamma_nominal < gamma_min ) {
    gamma_nominal = gamma_min;
  }

  if( gamma_nominal > gamma_max ) {
    gamma_nominal = gamma_max;
  }

  // For each bin, create a RooRealVar
  for( Int_t i = 0; i < numBins; ++i) {

    std::stringstream VarNameStream;
    VarNameStream << Prefix << "_bin_" << i;
    std::string VarName = VarNameStream.str();

    auto gamma = std::make_unique<RooRealVar>(VarName.c_str(), VarName.c_str(),
               gamma_nominal, gamma_min, gamma_max);
    gamma->setConstant( false );
    paramSet.addOwned(std::move(gamma));

  }

  return paramSet;

}


ParamHistFunc::NumBins ParamHistFunc::getNumBinsPerDim(RooArgSet const& vars) {
  int numVars = vars.size();

  if (numVars > 3 || numVars < 1) {
    std::cout << "ParamHistFunc() - Only works for 1-3 variables (1d-3d)" << std::endl;
    throw -1;
  }

  int numBinsX = numVars >= 1 ? static_cast<RooRealVar const&>(*vars[0]).numBins() : 1;
  int numBinsY = numVars >= 2 ? static_cast<RooRealVar const&>(*vars[1]).numBins() : 1;
  int numBinsZ = numVars >= 3 ? static_cast<RooRealVar const&>(*vars[2]).numBins() : 1;

  return {numBinsX, numBinsY, numBinsZ};
}


////////////////////////////////////////////////////////////////////////////////
/// Get the index of the gamma parameter associated with the current bin.
/// e.g. `RooRealVar& currentParam = getParameter( getCurrentBin() );`
Int_t ParamHistFunc::getCurrentBin() const {
  // We promise that our coordinates and the data hist coordinates have same layout.
  return _dataSet.getIndex(_dataVars, /*fast=*/true);
}


////////////////////////////////////////////////////////////////////////////////

Int_t ParamHistFunc::addParamSet( const RooArgList& params ) {
  // return 0 for success
  // return 1 for failure

  // Check that the supplied list has
  // the right number of arguments:

  Int_t numVarBins  = GetNumBins(_dataVars);
  Int_t numElements = params.size();

  if( numVarBins != numElements ) {
    std::cout << "ParamHistFunc::addParamSet - ERROR - "
         << "Supplied list of parameters " << params.GetName()
         << " has " << numElements << " elements but the ParamHistFunc"
         << GetName() << " has " << numVarBins << " bins."
         << std::endl;
    return 1;

  }

  // Check that the elements
  // are actually RooAbsreal's
  // If so, add them to the
  // list of params

  _paramSet.addTyped<RooAbsReal>(params);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
/// Find the bin corresponding to the current value of the observable, and evaluate
/// the associated parameter.
double ParamHistFunc::evaluate() const
{
  return getParameter().getVal();
}

////////////////////////////////////////////////////////////////////////////////
/// Find all bins corresponding to the values of the observables in `ctx`,
// and evaluate the associated parameters.
/// \param[in,out] ctx Input/output data for evaluating the ParamHistFunc.
void ParamHistFunc::doEval(RooFit::EvalContext & ctx) const
{
  std::span<double> output = ctx.output();
  std::size_t size = output.size();

  auto const& n = _numBinsPerDim;
  // check if _numBins needs to be filled
  if(n.x == 0) {
    _numBinsPerDim = getNumBinsPerDim(_dataVars);
  }

  // Different from the evaluate() funnction that first retrieves the indices
  // corresponding to the RooDataHist and then transforms them, we can use the
  // right bin multiplicators to begin with.
  std::array<int, 3> idxMult{{1, n.x, n.xy}};

  // As a working buffer for the bin indices, we use the tail of the output
  // buffer. We can't use the same starting pointer, otherwise we would
  // overwrite the later bin indices as we fill the output.
  auto indexBuffer = reinterpret_cast<int*>(output.data() + size) - size;
  std::fill(indexBuffer, indexBuffer + size, 0); // output buffer for bin indices needs to be zero-initialized

  // Use the vectorized RooAbsBinning::binNumbers() to update the total bin
  // index for each dimension, using the `coef` parameter to multiply with the
  // right index multiplication factor for each dimension.
  for (std::size_t iVar = 0; iVar < _dataVars.size(); ++iVar) {
    _dataSet.getBinnings()[iVar]->binNumbers(ctx.at(&_dataVars[iVar]).data(), indexBuffer, size, idxMult[iVar]);
  }

  // Finally, look up the parameters and get their values to fill the output buffer
  for (std::size_t i = 0; i < size; ++i) {
    output[i] = static_cast<RooAbsReal const&>(_paramSet[indexBuffer[i]]).getVal();
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Advertise that all integrals can be handled internally.

Int_t ParamHistFunc::getAnalyticalIntegralWN(RooArgSet& allVars, RooArgSet& analVars,
                    const RooArgSet* normSet,
                    const char* /*rangeName*/) const
{
  // Handle trivial no-integration scenario
  if (allVars.empty()) return 0 ;
  if (_forceNumInt) return 0 ;


  // Select subset of allVars that are actual dependents
  analVars.add(allVars) ;

  // Check if this configuration was created before
  Int_t sterileIdx(-1) ;
  CacheElem* cache = static_cast<CacheElem*>(_normIntMgr.getObj(normSet,&analVars,&sterileIdx,(const char*)nullptr)) ;
  if (cache) {
    return _normIntMgr.lastIndex()+1 ;
  }

  // Create new cache element
  cache = new CacheElem ;

  // Store cache element
  Int_t code = _normIntMgr.setObj(normSet,&analVars,cache,nullptr) ;

  return code+1 ;

}


////////////////////////////////////////////////////////////////////////////////
/// Implement analytical integrations by doing appropriate weighting from  component integrals
/// functions to integrators of components

double ParamHistFunc::analyticalIntegralWN(Int_t /*code*/, const RooArgSet* /*normSet2*/,
                    const char* /*rangeName*/) const
{
  double value(0) ;

  // Simply loop over bins,
  // get the height, and
  // multiply by the bind width
  auto binVolumes = _dataSet.binVolumes(0, _dataSet.numEntries());

  for (unsigned int i=0; i < _paramSet.size(); ++i) {
    const auto& param = static_cast<const RooAbsReal&>(_paramSet[i]);

    // Get the gamma's value
    const double paramVal = param.getVal();

    // Finally, get the subtotal
    value += paramVal * binVolumes[i];
  }

  return value;

}



////////////////////////////////////////////////////////////////////////////////
/// Return sampling hint for making curves of (projections) of this function
/// as the recursive division strategy of RooCurve cannot deal efficiently
/// with the vertical lines that occur in a non-interpolated histogram

std::list<double>* ParamHistFunc::plotSamplingHint(RooAbsRealLValue& obs, double xlo,
                  double xhi) const
{
  // copied and edited from RooHistFunc
  RooAbsLValue* lvarg = &obs;

  // Retrieve position of all bin boundaries
  const RooAbsBinning* binning = lvarg->getBinningPtr(nullptr);
  double* boundaries = binning->array() ;

  std::list<double>* hint = new std::list<double> ;

  // Widen range slightly
  xlo = xlo - 0.01*(xhi-xlo) ;
  xhi = xhi + 0.01*(xhi-xlo) ;

  double delta = (xhi-xlo)*1e-8 ;

  // Construct array with pairs of points positioned epsilon to the left and
  // right of the bin boundaries
  for (Int_t i=0 ; i<binning->numBoundaries() ; i++) {
    if (boundaries[i]>=xlo && boundaries[i]<=xhi) {
      hint->push_back(boundaries[i]-delta) ;
      hint->push_back(boundaries[i]+delta) ;
    }
  }
  return hint ;
}


////////////////////////////////////////////////////////////////////////////////
/// Return sampling hint for making curves of (projections) of this function
/// as the recursive division strategy of RooCurve cannot deal efficiently
/// with the vertical lines that occur in a non-interpolated histogram

std::list<double> *ParamHistFunc::binBoundaries(RooAbsRealLValue &obs, double xlo, double xhi) const
{
   // copied and edited from RooHistFunc
   RooAbsLValue *lvarg = &obs;

   // look for variable in the DataHist, and if found, return the binning
   std::string varName = dynamic_cast<TObject *>(lvarg)->GetName();
   RooArgSet const &vars = *_dataSet.get(); // guaranteed to be in the same order as the binnings vector
   auto &binnings = _dataSet.getBinnings();
   for (size_t i = 0; i < vars.size(); i++) {
      if (varName == vars[i]->GetName()) {
         // found the variable, return its binning
         double *boundaries = binnings.at(i)->array();
         std::list<double> *hint = new std::list<double>;
         for (int j = 0; j < binnings.at(i)->numBoundaries(); j++) {
            if (boundaries[j] >= xlo && boundaries[j] <= xhi) {
               hint->push_back(boundaries[j]);
            }
         }
         return hint;
      }
   }
   // variable not found, return null
   return nullptr;
}
