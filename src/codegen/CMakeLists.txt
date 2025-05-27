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

target_link_libraries(RooFitCodegen PRIVATE
    RooFit
    RooFitCore
    HistFactory
)

target_link_libraries(RooFitCodegen PUBLIC
    RooFitCore
)

ROOT_GENERATE_DICTIONARY(G__RooFitCodegen ${sources_headers}
    LINKDEF LinkDef.h
    MODULE RooFitCodegen
    OPTIONS --deep -writeEmptyRootPCM
)

# Install the dictionaries.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRooFitCodegen_rdict.pcm
              ${CMAKE_CURRENT_BINARY_DIR}/libRooFitCodegen.rootmap
        DESTINATION lib)

# Install the libraries and header files.
install(TARGETS RooFitCodegen
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")
