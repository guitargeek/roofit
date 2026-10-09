/*
 * Project: RooFit
 *
 * Copyright (c) 2026, CERN
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted according to the terms
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)
 */

// Selects the Minuit2 that RooFit uses. Specific to the standalone RooFit
// repository: with the builtin_minuit2 option (ROOFIT_BUILTIN_MINUIT2), RooFit
// uses the Minuit2 that is built from math/minuit2 in this repository, where
// the namespace ROOT::Minuit2 is renamed to ROOT::RooFitMinuit2 and the header
// directory Minuit2/ to RooFitMinuit2/. Otherwise, it uses the Minuit2 of the
// ROOT installation. See math/CMakeLists.txt for details.

#ifndef RooFit_RooMinuit2Helpers_h
#define RooFit_RooMinuit2Helpers_h

#include <Fit/FitConfig.h>
#include <Math/Factory.h>
#include <Math/Minimizer.h>

#ifdef ROOFIT_BUILTIN_MINUIT2
#include <RooFitMinuit2/FCNBase.h>
#include <RooFitMinuit2/Minuit2Minimizer.h>
#include <RooFitMinuit2/MnMatrix.h>
#include <RooFitMinuit2/MnStrategy.h>
#include <RooFitMinuit2/NumericalDerivator.h>
#else
#include <Minuit2/FCNBase.h>
#include <Minuit2/Minuit2Minimizer.h>
#include <Minuit2/MnMatrix.h>
#include <Minuit2/MnStrategy.h>
#include <Minuit2/NumericalDerivator.h>
#endif

#include <stdexcept>
#include <string>

namespace RooFit::Detail {

/// The namespace of the Minuit2 that RooFit uses. RooFit code that uses Minuit2
/// classes directly has to go through this alias.
#ifdef ROOFIT_BUILTIN_MINUIT2
namespace Minuit2 = ROOT::RooFitMinuit2;
#else
namespace Minuit2 = ROOT::Minuit2;
#endif

#ifdef ROOFIT_BUILTIN_MINUIT2
/// Create a Minuit2Minimizer of the builtin Minuit2, via the "RooFitMinuit2"
/// plugin that is registered by math/RooFitMinuit2Plugin.cxx. There is no
/// fallback to other minimizers, to not silently use the Minuit2 of ROOT.
inline ROOT::Math::Minimizer *createBuiltinMinuit2(std::string const &algo)
{
   ROOT::Math::Minimizer *minimizer = ROOT::Math::Factory::CreateMinimizer("RooFitMinuit2", algo);
   if (!minimizer) {
      throw std::runtime_error("RooFit::Detail::createBuiltinMinuit2(): could not create the builtin Minuit2 minimizer");
   }
   return minimizer;
}
#endif

// The createMinimizer() functions redirect the "Minuit2" minimizer type to the
// builtin Minuit2 if it is used. Like this, the minimizer type that users and
// RooFit see stays "Minuit2", and only the created minimizer is different.
// RooFit code should therefore never create minimizers via the
// ROOT::Math::Factory or ROOT::Fit::FitConfig directly.

/// Create a minimizer like ROOT::Math::Factory::CreateMinimizer().
inline ROOT::Math::Minimizer *createMinimizer(std::string const &type, std::string const &algo)
{
#ifdef ROOFIT_BUILTIN_MINUIT2
   if (type == "Minuit2") {
      return createBuiltinMinuit2(algo);
   }
#endif
   return ROOT::Math::Factory::CreateMinimizer(type, algo);
}

/// Create a minimizer like ROOT::Fit::FitConfig::CreateMinimizer().
inline ROOT::Math::Minimizer *createMinimizer(ROOT::Fit::FitConfig &config)
{
#ifdef ROOFIT_BUILTIN_MINUIT2
   if (config.MinimizerType() == "Minuit2") {
      // The FitConfig doesn't only create the minimizer, it also passes all the
      // minimizer options to it, like the error level. Therefore, it is asked
      // to create the builtin Minuit2, and the minimizer type is set back
      // right away.
      config.SetMinimizer("RooFitMinuit2");
      ROOT::Math::Minimizer *minimizer = config.CreateMinimizer();
      // If the minimizer could not be created, the FitConfig falls back to
      // another minimizer type. There is no fallback here, to not silently use
      // something else than the builtin Minuit2.
      const bool isBuiltin = config.MinimizerType() == "RooFitMinuit2";
      config.SetMinimizer("Minuit2");
      if (!minimizer || !isBuiltin) {
         delete minimizer;
         throw std::runtime_error("RooFit::Detail::createMinimizer(): could not create the builtin Minuit2 minimizer");
      }
      return minimizer;
   }
#endif
   return config.CreateMinimizer();
}

} // namespace RooFit::Detail

#endif
