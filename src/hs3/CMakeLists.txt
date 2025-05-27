# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT rootfit/histfactory package
# @author Carsten Burgard, CERN
############################################################################

add_library(RooFitHS3 SHARED)

set(sources_headers
    RooFitHS3/JSONIO.h
    RooFitHS3/RooJSONFactoryWSTool.h
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/Domains.cxx
    src/JSONIO.cxx
    src/RooJSONFactoryWSTool.cxx
    src/JSONFactories_RooFitCore.cxx
    src/JSONFactories_HistFactory.cxx
    src/JSONIOUtils.cxx
)

target_sources(RooFitHS3 PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(RooFitHS3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(RooFitHS3 PRIVATE
    RooFit
    HistFactory
)

target_link_libraries(RooFitHS3 PUBLIC
    RooFitCore
    RooFitJSONInterface
)

ROOT_GENERATE_DICTIONARY(G__RooFitHS3 ${sources_headers}
    LINKDEF LinkDef.h
    MODULE RooFitHS3
    OPTIONS --deep
)

# Install the dictionaries.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libRooFitHS3_rdict.pcm
              ${CMAKE_CURRENT_BINARY_DIR}/libRooFitHS3.rootmap
        DESTINATION lib)

# Install the libraries and header files.
install(TARGETS RooFitHS3
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

if(testing)
  add_subdirectory(test)
endif()
