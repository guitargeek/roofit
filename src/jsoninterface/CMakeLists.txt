# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT RooFitJSONInterface package
# @author Jonas Rembser, CERN
############################################################################

add_library(RooFitJSONInterface SHARED)

set(sources_headers
    RooFit/Detail/JSONInterface.h
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/JSONInterface.cxx
    src/JSONParser.cxx
)

target_sources(RooFitJSONInterface PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(RooFitJSONInterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(RooFitJSONInterface PRIVATE
    Core
)

# Install the libraries and header files.
install(TARGETS RooFitJSONInterface
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

find_package(nlohmann_json)

target_link_libraries(RooFitJSONInterface PRIVATE nlohmann_json::nlohmann_json)

if(testing)
  #add_subdirectory(test)
endif()
