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

set(ROOFITMORE_HEADERS
    RooFitMoreLib.h
    RooLegendre.h
    RooMathMoreReg.h
    RooSpHarmonic.h
    RooNonCentralChiSquare.h
    RooHypatia2.h
)

set(RELATIVE_INC_HEADERS ${ROOFITMORE_HEADERS})
list(TRANSFORM RELATIVE_INC_HEADERS PREPEND inc/)

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

target_sources(RooFitMore PRIVATE ${RELATIVE_INC_HEADERS} ${sources_cxx})

target_include_directories(RooFitMore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

find_package(GSL 1.10 REQUIRED)

target_link_libraries(RooFitMore PRIVATE
    RooBatchCompute
    GSL::gsl
    GSL::gslcblas
)

target_link_libraries(RooFitMore PUBLIC
    Core
    RooFitCore
    RooFit
    Hist
    Tree
    RIO
    ${ROOT_MATHMORE_LIBRARY}
    MathCore
)

ROOT_GENERATE_DICTIONARY(G__RooFitMore ${RELATIVE_INC_HEADERS}
    LINKDEF inc/LinkDef.h
    MODULE RooFitMore
    OPTIONS --deep
)

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

if(NOT CMAKE_VERSION VERSION_LESS "3.23.0")
  target_sources(
      RooFitMore
      PRIVATE
      FILE_SET private_header_files
      TYPE HEADERS
      BASE_DIRS inc/ src/
      FILES
          ${RELATIVE_INC_HEADERS}
          inc/LinkDef.h
          src/BracketAdapters.h
          src/RooAdaptiveGaussKronrodIntegrator1D.h
          src/RooGaussKronrodIntegrator1D.h
  )
endif()
