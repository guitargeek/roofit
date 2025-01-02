# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT roofitmore package
# @author Lorenzo Moneta, CERN
############################################################################

if(mathmore)
  set(ROOT_MATHMORE_LIBRARY MathMore)
endif()

add_library(RooFitMore SHARED)

set(sources_headers
    RooFitMoreLib.h
    RooLegendre.h
    RooMathMoreReg.h
    RooSpHarmonic.h
    RooNonCentralChiSquare.h
    RooHypatia2.h
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/RooFitMoreLib.cxx
    src/RooAdaptiveGaussKronrodIntegrator1D.cxx
    src/RooGaussKronrodIntegrator1D.cxx
    src/RooLegendre.cxx
    src/RooMathMoreReg.cxx
    src/RooSpHarmonic.cxx
    src/RooNonCentralChiSquare.cxx
    src/RooHypatia2.cxx
)

target_sources(RooFitMore PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(RooFitMore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(RooFitMore PRIVATE
    RooBatchCompute
    GSL::gsl
)

find_package(GSL 1.10 REQUIRED)

target_link_libraries(RooFitMore PUBLIC
    MathMore
    Core
    RooFitCore
    RooFit
    Hist
    Matrix
    Tree
    Minuit
    RIO
    MathCore
    Foam
)

ROOT_GENERATE_DICTIONARY(G__RooFitMore ${sources_headers}
    LINKDEF inc/LinkDef.h
    MODULE RooFitMore
    OPTIONS --deep
)

# Install the dictionaries.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRooFitMore_rdict.pcm
              ${CMAKE_CURRENT_BINARY_DIR}/libRooFitMore.rootmap
        DESTINATION lib)

# Install the libraries and header files.
install(TARGETS RooFitMore
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

# For recent clang, this can facilitate auto-vectorisation.
# In RooFit, the errno side effect is not needed, anyway:
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  target_compile_options(RooFitMore PUBLIC -fno-math-errno)
endif()
