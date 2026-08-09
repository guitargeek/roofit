/// \file compat/Math/CladDerivator.h
///
/// \brief Shim that repairs ROOT's <Math/CladDerivator.h> for a ROOT build
///        without TMVA.
///
/// RooFit asks the interpreter to parse <Math/CladDerivator.h> before every
/// clad::gradient() / clad::hessian() request (see RooFuncWrapper). Some ROOT
/// versions -- 6.40 among them -- end that header with reverse-mode pullbacks
/// for the TMVA SOFIE inference helpers, and the Gemm_Call one starts with
///
///     using ::TMVA::Experimental::SOFIE::Gemm_Call;
///
/// That name only exists if TMVA is part of the build. With a RooFit-less ROOT
/// that was also configured with -Dtmva=OFF -- the usual case for this
/// repository -- parsing the header fails, clad is never declared, and every
/// differentiation request dies with "Function could not be differentiated".
///
/// The declaration is all that is missing: the pullback is a plain inline
/// function that nothing in RooFit calls, so it is never emitted and no
/// definition of Gemm_Call is ever needed. This file therefore declares the
/// function and then hands over to ROOT's real header via #include_next.
///
/// The directory holding this file is put in front of $ROOTSYS/include on the
/// interpreter's search path (see the top-level CMakeLists.txt), so an
/// #include <Math/CladDerivator.h> from anywhere lands here first. When ROOT
/// does ship TMVA, __has_include() below finds the SOFIE header, the shim adds
/// nothing, and the include chain behaves exactly as without this file.

#ifndef ROOFIT_COMPAT_CLAD_DERIVATOR
#define ROOFIT_COMPAT_CLAD_DERIVATOR

#if defined(__CLING__) && !__has_include(<TMVA/SOFIE_common.hxx>)

namespace TMVA {
namespace Experimental {
namespace SOFIE {

void Gemm_Call(float *output, bool transa, bool transb, int m, int n, int k, float alpha, const float *A,
               const float *B, float beta, const float *C);

} // namespace SOFIE
} // namespace Experimental
} // namespace TMVA

#endif

#include_next <Math/CladDerivator.h>

#endif // ROOFIT_COMPAT_CLAD_DERIVATOR
