// Registers RooFit's builtin Minuit2 as the "RooFitMinuit2" plugin for
// ROOT::Math::Minimizer, so it can be created with the ROOT::Math::Factory.
//
// In ROOT, the plugin handlers are defined by the macros in $ROOTSYS/etc/plugins,
// which is not an option for a library that is built outside of ROOT. The
// handler is therefore added when this library is loaded. The plugin manager
// only replaces handlers with the same name when it reads the plugin
// directories later, so the handler stays.
//
// See math/CMakeLists.txt for how the builtin Minuit2 is built.

#include <TPluginManager.h>
#include <TROOT.h>

namespace {

struct RegisterRooFitMinuit2Plugin {
   RegisterRooFitMinuit2Plugin()
   {
      gROOT->GetPluginManager()->AddHandler("ROOT::Math::Minimizer", "RooFitMinuit2",
                                            "ROOT::RooFitMinuit2::Minuit2Minimizer", "RooFitMinuit2",
                                            "Minuit2Minimizer(const char *)");
   }
} registerRooFitMinuit2Plugin;

} // namespace
