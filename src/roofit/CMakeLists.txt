# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT rootfit package
# @author Pere Mato, CERN
############################################################################

add_library(RooFit SHARED)

set(sources_headers
    Roo2DKeysPdf.h
    RooArgusBG.h
    RooBCPEffDecay.h
    RooBCPGenDecay.h
    RooBDecay.h
    RooBernstein.h
    RooBifurGauss.h
    RooBlindTools.h
    RooBMixDecay.h
    RooBreitWigner.h
    RooBukinPdf.h
    RooCBShape.h
    RooCrystalBall.h
    RooCFunction1Binding.h
    RooCFunction2Binding.h
    RooCFunction3Binding.h
    RooCFunction4Binding.h
    RooChebychev.h
    RooChi2MCSModule.h
    RooChiSquarePdf.h
    RooDecay.h
    RooDstD0BG.h
    RooExponential.h
    RooLegacyExpPoly.h
    RooPowerSum.h
    RooFunctor1DBinding.h
    RooFunctorBinding.h
    RooGamma.h
    RooGaussian.h
    RooGaussModel.h
    RooGExpModel.h
    RooHistConstraint.h
    RooIntegralMorph.h
    RooJeffreysPrior.h
    RooKeysPdf.h
    RooLagrangianMorphFunc.h
    RooLandau.h
    RooLognormal.h
    RooMathCoreReg.h
    RooMomentMorph.h
    RooMomentMorphFunc.h
    RooMomentMorphFuncND.h
    RooMultiBinomial.h
    RooMultiPdf.h
    RooNDKeysPdf.h
    RooNonCPEigenDecay.h
    RooNovosibirsk.h
    RooParametricStepFunction.h
    RooParamHistFunc.h
    RooPoisson.h
    RooPolynomial.h
    RooPyBind.h
    RooSpline.h
    RooStepFunction.h
    RooTFnBinding.h
    RooTFnPdfBinding.h
    RooTMathReg.h
    RooUnblindCPAsymVar.h
    RooUnblindOffset.h
    RooUnblindPrecision.h
    RooUnblindUniform.h
    RooUniform.h
    RooVoigtian.h
    RooJohnson.h
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/Roo2DKeysPdf.cxx
    src/RooArgusBG.cxx
    src/RooBCPEffDecay.cxx
    src/RooBCPGenDecay.cxx
    src/RooBDecay.cxx
    src/RooBernstein.cxx
    src/RooBifurGauss.cxx
    src/RooBlindTools.cxx
    src/RooBMixDecay.cxx
    src/RooBreitWigner.cxx
    src/RooBukinPdf.cxx
    src/RooCBShape.cxx
    src/RooCrystalBall.cxx
    src/RooCFunction1Binding.cxx
    src/RooCFunction2Binding.cxx
    src/RooCFunction3Binding.cxx
    src/RooCFunction4Binding.cxx
    src/RooChebychev.cxx
    src/RooChi2MCSModule.cxx
    src/RooChiSquarePdf.cxx
    src/RooDecay.cxx
    src/RooDstD0BG.cxx
    src/RooExponential.cxx
    src/RooLegacyExpPoly.cxx
        src/RooPowerSum.cxx
    src/RooFunctor1DBinding.cxx
    src/RooFunctorBinding.cxx
    src/RooGamma.cxx
    src/RooGaussian.cxx
    src/RooGaussModel.cxx
    src/RooGExpModel.cxx
    src/RooHistConstraint.cxx
    src/RooIntegralMorph.cxx
    src/RooJeffreysPrior.cxx
    src/RooKeysPdf.cxx
    src/RooLagrangianMorphFunc.cxx
    src/RooLandau.cxx
    src/RooLognormal.cxx
    src/RooMathCoreReg.cxx
    src/RooMomentMorph.cxx
    src/RooMomentMorphFunc.cxx
    src/RooMomentMorphFuncND.cxx
    src/RooMultiBinomial.cxx
    src/RooMultiPdf.cxx
    src/RooNDKeysPdf.cxx
    src/RooNonCPEigenDecay.cxx
    src/RooNovosibirsk.cxx
    src/RooParametricStepFunction.cxx
    src/RooParamHistFunc.cxx
    src/RooPoisson.cxx
    src/RooPolynomial.cxx
    src/RooSpline.cxx
    src/RooStepFunction.cxx
    src/RooTFnBinding.cxx
    src/RooTFnPdfBinding.cxx
    src/RooTMathReg.cxx
    src/RooUnblindCPAsymVar.cxx
    src/RooUnblindOffset.cxx
    src/RooUnblindPrecision.cxx
    src/RooUnblindUniform.cxx
    src/RooUniform.cxx
    src/RooVoigtian.cxx
    src/RooJohnson.cxx
)

target_sources(RooFit PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(RooFit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(RooFit PRIVATE
    RooBatchCompute
)

target_link_libraries(RooFit PUBLIC
    Core
    RooFitCore
    Tree
    RIO
    Matrix
    MathCore
)

ROOT_GENERATE_DICTIONARY(G__RooFit ${sources_headers}
    LINKDEF inc/LinkDef1.h
    MODULE RooFit
    OPTIONS --deep
)

# Install the dictionaries.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRooFit_rdict.pcm
              ${CMAKE_CURRENT_BINARY_DIR}/libRooFit.rootmap
        DESTINATION lib)

# Install the libraries and header files.
install(TARGETS RooFit
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

# For recent clang, this can facilitate auto-vectorisation.
# In RooFit, the errno side effect is not needed, anyway:
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  target_compile_options(RooFit PUBLIC -fno-math-errno)
endif()

if(testing)
  add_subdirectory(test)
endif()
