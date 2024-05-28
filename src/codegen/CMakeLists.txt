# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT rootfit/histfactory package
# @author Pere Mato, CERN
############################################################################

add_library(RooFitCodegen SHARED)

set(sources_headers
    RooFit/CodegenImpl.h
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/CodegenImpl.cxx
)

target_sources(RooFitCodegen PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(RooFitCodegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# These are the DEPENDENCIES in the ROOT build, which are linked publicly there.
target_link_libraries(RooFitCodegen PUBLIC
    RooFit
    RooFitCore
    HistFactory
)

# No dictionary is generated for this library. Its LinkDef.h doesn't select
# anything anymore, and rootcling refuses to run without selection rules unless
# it is asked to create a C++ module, which we don't do here. In the ROOT build,
# the dictionary only serves to create the C++ module for RooFit/CodegenImpl.h.
# The codegen functions are found by the interpreter via symbol lookup in the
# libraries on the library search path, which doesn't need a rootmap.

# Install the libraries and header files.
install(TARGETS RooFitCodegen
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

if(NOT CMAKE_VERSION VERSION_LESS "3.23.0")
  target_sources(
      RooFitCodegen
      PRIVATE
      FILE_SET private_header_files
      TYPE HEADERS
      BASE_DIRS inc/
      FILES
          # LinkDef.h # empty, not being used by ROOT_STANDARD_LIBRARY_PACKAGE call
          inc/RooFit/CodegenImpl.h
  )
endif()
