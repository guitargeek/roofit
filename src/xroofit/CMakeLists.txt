add_library(RooFitXRooFit SHARED)

set (XROOFIT_HEADERS
  RooBrowser.h
  XRooFit.h
  RooFit/xRooFit/xRooFit.h
  RooFit/xRooFit/xRooNode.h
  RooFit/xRooFit/xRooNLLVar.h
  RooFit/xRooFit/xRooHypoSpace.h
  RooFit/xRooFit/xRooBrowser.h
)

set(RELATIVE_INC_HEADERS ${XROOFIT_HEADERS})
list(TRANSFORM RELATIVE_INC_HEADERS PREPEND inc/)

set(sources_cxx
    src/Asymptotics.cxx
    src/xRooBrowser.cxx
    src/xRooFit.cxx
    src/xRooHypoSpace.cxx
    src/xRooNLLVar.cxx
    src/xRooNode.cxx
    src/xRooNode_interactive.cxx
    src/PythonInterface.cxx
)

target_sources(RooFitXRooFit PRIVATE ${RELATIVE_INC_HEADERS} ${sources_cxx})

target_include_directories(RooFitXRooFit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# These are the DEPENDENCIES in the ROOT build, which are linked publicly there.
target_link_libraries(RooFitXRooFit PUBLIC
    HistFactory
    RooFit
    RooFitHS3
    RooStats
    FitPanel
    Gui
    Ged
    RooFitCore
)

ROOT_GENERATE_DICTIONARY(G__RooFitXRooFit ${RELATIVE_INC_HEADERS}
    LINKDEF inc/LinkDef.h
    MODULE RooFitXRooFit
    OPTIONS --deep
)

# Install the libraries and header files.
install(TARGETS RooFitXRooFit
        LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/" DESTINATION "include")

target_include_directories(RooFitXRooFit PRIVATE inc/RooFit)

if(NOT CMAKE_VERSION VERSION_LESS "3.23.0")
  target_sources(
      RooFitXRooFit
      PRIVATE
      FILE_SET private_header_files
      TYPE HEADERS
      BASE_DIRS inc/ src/
      FILES
          ${RELATIVE_INC_HEADERS}
          inc/LinkDef.h
          inc/RooFit/xRooFit/Config.h # was not part of XROOFIT_HEADERS
          src/PythonInterface.h
          src/coutCapture.h
          src/xRooFitVersion.h
  )
endif()

# The only test of this package is a Python one.
if(testing AND Python3_Interpreter_FOUND)
  add_subdirectory(test)
endif()
