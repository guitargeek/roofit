/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitModels                                                     *
 *    File: $Id: RooGaussian.h,v 1.16 2007/07/12 20:30:49 wouter Exp $
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef ROO_GAUSSIAN
#define ROO_GAUSSIAN

#include "RooAbsPdf.h"
#include "RooRealProxy.h"

class RooAbsReal;

class RooGaussian : public RooAbsPdf {
public:
  RooGaussian() { };
  // Original constructor without RooAbsReal::Ref for backwards compatibility.
  inline RooGaussian(const char *name, const char *title,
         RooAbsReal& _x, RooAbsReal& _mean, RooAbsReal& _sigma)
      : RooGaussian{name, title, RooAbsReal::Ref{_x}, RooAbsReal::Ref{_mean}, RooAbsReal::Ref{_sigma}} {}
  RooGaussian(const char *name, const char *title,
         RooAbsReal::Ref _x, RooAbsReal::Ref _mean, RooAbsReal::Ref _sigma);
  RooGaussian(const RooGaussian& other, const char* name=nullptr);
  TObject* clone(const char* newname=nullptr) const override {
    return new RooGaussian(*this,newname);
  }

  Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars, const char* rangeName=nullptr) const override;
  double analyticalIntegral(Int_t code, const char* rangeName=nullptr) const override;

  Int_t getGenerator(const RooArgSet& directVars, RooArgSet &generateVars, bool staticInitOK=true) const override;
  void generateEvent(Int_t code) override;

  /// Get the x variable.
  RooAbsReal const& getX() const { return x.arg(); }

  /// Get the mean parameter.
  RooAbsReal const& getMean() const { return mean.arg(); }

  /// Get the sigma parameter.
  RooAbsReal const& getSigma() const { return sigma.arg(); }

protected:

  RooRealProxy x ;
  RooRealProxy mean ;
  RooRealProxy sigma ;

  double evaluate() const override;
  void doEval(RooFit::EvalContext &) const override;
  inline bool canComputeBatchWithCuda() const override { return true; }

private:

  ClassDefOverride(RooGaussian,1) // Gaussian PDF
};

#endif
