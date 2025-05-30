# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

############################################################################
# CMakeLists.txt file for building ROOT rootfit/histfactory package
# @author Pere Mato, CERN
############################################################################

# The ConfigParser can create HistFactory models from XML files. It uses TXML,
# so we won't build it if the xml build option is OFF.
if (xml)
    set(HISTFACTORY_XML_HEADERS RooStats/HistFactory/ConfigParser.h)
    set(HISTFACTORY_XML_SOURCES src/ConfigParser.cxx)
    set(HISTFACTORY_XML_LIBRARIES XMLParser)
endif()

set (EXTRA_DICT_OPTS)
if (runtime_cxxmodules AND WIN32)
  set (EXTRA_DICT_OPTS NO_CXXMODULE)
endif()

add_library(HistFactory SHARED)

set(sources_headers
    RooStats/HistFactory/Asimov.h
    RooStats/HistFactory/Channel.h
    RooStats/HistFactory/Data.h
    RooStats/HistFactory/Detail/HistFactoryImpl.h
    RooStats/HistFactory/FlexibleInterpVar.h
    RooStats/HistFactory/HistFactoryException.h
    RooStats/HistFactory/HistFactoryModelUtils.h
    RooStats/HistFactory/HistFactoryNavigation.h
    RooStats/HistFactory/HistoToWorkspaceFactoryFast.h
    RooStats/HistFactory/HistRef.h
    RooStats/HistFactory/LinInterpVar.h
    RooStats/HistFactory/MakeModelAndMeasurementsFast.h
    RooStats/HistFactory/Measurement.h
    RooStats/HistFactory/ParamHistFunc.h
    RooStats/HistFactory/PiecewiseInterpolation.h
    RooStats/HistFactory/PreprocessFunction.h
    RooStats/HistFactory/RooBarlowBeestonLL.h
    RooStats/HistFactory/Sample.h
    RooStats/HistFactory/Systematics.h
    ${HISTFACTORY_XML_HEADERS}
)

list(TRANSFORM sources_headers PREPEND "inc/")

set(sources_cxx
    src/Asimov.cxx
    src/Channel.cxx
    src/Data.cxx
    src/FlexibleInterpVar.cxx
    src/HistFactoryImpl.cxx
    src/HistFactoryModelUtils.cxx
    src/HistFactoryNavigation.cxx
    src/HistRef.cxx
    src/HistoToWorkspaceFactoryFast.cxx
    src/LinInterpVar.cxx
    src/MakeModelAndMeasurementsFast.cxx
    src/Measurement.cxx
    src/ParamHistFunc.cxx
    src/PiecewiseInterpolation.cxx
    src/PreprocessFunction.cxx
    src/RooBarlowBeestonLL.cxx
    src/Sample.cxx
    src/Systematics.cxx
    ${HISTFACTORY_XML_SOURCES}
)

target_sources(HistFactory PRIVATE ${sources_headers} ${sources_cxx})

target_include_directories(HistFactory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(HistFactory PRIVATE
    RooBatchCompute
    ${HISTFACTORY_XML_LIBRARIES}
)

target_link_libraries(HistFactory PUBLIC
    RooFit
    RooFitCore
    Tree
    RIO
    Hist
    Matrix
    MathCore
    Graf
    Gpad
    RooStats
    RooFitJSONInterface
)

ROOT_GENERATE_DICTIONARY(G__HistFactory ${sources_headers}
    LINKDEF inc/LinkDef.h
    MODULE HistFactory
    OPTIONS --deep
)

# Install the dictionaries.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libHistFactory_rdict.pcm
              ${CMAKE_CURRENT_BINARY_DIR}/libHistFactory.rootmap
        DESTINATION lib)

# Install the libraries and header files.
install(TARGETS HistFactory
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

# For recent clang, this can facilitate auto-vectorisation.
# In RooFit, the errno side effect is not needed, anyway:
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  target_compile_options(HistFactory PUBLIC -fno-math-errno)
endif()

# The hist2workspace executable uses the HistFactory::ConfigParser to read the
# XML model specification. The ConfigParser is only built when xml is ON, so we
# can't build hist2workspace without xml.
if(xml)
  ROOT_EXECUTABLE(hist2workspace hist2workspace.cxx LIBRARIES HistFactory)
  target_compile_definitions(HistFactory PUBLIC HISTFACTORY_XML)
endif()

if(MSVC)
  set(prepareHistFactory_script prepareHistFactory.bat)
else()
  set(prepareHistFactory_script prepareHistFactory)
endif()
file(COPY config/${prepareHistFactory_script} DESTINATION  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${prepareHistFactory_script}
                PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                            GROUP_EXECUTE GROUP_READ
                            WORLD_EXECUTE WORLD_READ
                DESTINATION ${CMAKE_INSTALL_BINDIR})

if(testing)
  add_subdirectory(test)
endif()
