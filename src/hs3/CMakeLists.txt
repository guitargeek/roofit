# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT rootfit/histfactory package
# @author Carsten Burgard, CERN
############################################################################

# The RooFit-independent part is compiled as a separate object library, so we
# don't accidentally include RooFit again without getting compiler errors.
add_library(hs3cpp OBJECT
  src/HS3Document.cxx
  src/HS3Parser.cxx
  src/HS3Upgrade.cxx
  src/HS3Validate.cxx
  src/HS3Variates.cxx
  src/HS3Writer.cxx
  src/JSONInterface.cxx
)
set_target_properties(hs3cpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(hs3cpp PRIVATE nlohmann_json::nlohmann_json)
# In the ROOT build, the public headers are found in the include directory of
# the build tree.
target_include_directories(hs3cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(RooFitHS3 SHARED)

set(ROOFITHS3_HEADERS
    RooFit/Detail/JSONInterface.h
    RooFitHS3/JSONIO.h
    RooFitHS3/RooJSONFactoryWSTool.h
)

set(RELATIVE_INC_HEADERS ${ROOFITHS3_HEADERS})
list(TRANSFORM RELATIVE_INC_HEADERS PREPEND inc/)

set(sources_cxx
    src/JSONIO.cxx
    src/RooJSONFactoryWSTool.cxx
    src/JSONFactories_RooFitCore.cxx
    src/JSONFactories_HistFactory.cxx
    src/JSONIOUtils.cxx
)

target_sources(RooFitHS3 PRIVATE ${RELATIVE_INC_HEADERS} ${sources_cxx})
target_sources(RooFitHS3 PRIVATE $<TARGET_OBJECTS:hs3cpp>)

target_include_directories(RooFitHS3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# These are the DEPENDENCIES in the ROOT build, which are linked publicly there.
# They have to be public here too, otherwise the include directories and link
# dependencies don't propagate to the users of this library, like the tests that
# use HistFactory classes.
target_link_libraries(RooFitHS3 PUBLIC
    RooFitCore
    RooFit
    HistFactory
)

ROOT_GENERATE_DICTIONARY(G__RooFitHS3 ${RELATIVE_INC_HEADERS}
    LINKDEF LinkDef.h
    MODULE RooFitHS3
    OPTIONS --deep
)

# Install the libraries and header files.
install(TARGETS RooFitHS3
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

target_sources(
    RooFitHS3
    PRIVATE
    FILE_SET private_header_files
    TYPE HEADERS
    BASE_DIRS inc/ src/
    FILES
        ${RELATIVE_INC_HEADERS}
        # LinkDef.h # seemingly not being used by ROOT_STANDARD_LIBRARY_PACKAGE call
        src/HS3Builder.h
        src/HS3Document.h
        src/HS3Schema.h
        src/JSONIOUtils.h
        src/JSONReadRecorder.h
)

if(testing)
  add_subdirectory(test)
endif()
