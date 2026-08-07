cmake_minimum_required(VERSION 3.20)
project(RooFit
    VERSION 6.41.01
    DESCRIPTION "RooFit"
    LANGUAGES CXX)


find_package(ROOT REQUIRED COMPONENTS Core MathCore Minuit Tree)
include(${ROOT_USE_FILE})

# Like ROOT, install into lib and not into lib64. This has to happen before
# GNUInstallDirs is included, because that module only fills in the variables
# that are not defined yet.
set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "header files")
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "libraries")

include(GNUInstallDirs)

# Follow the layout of the ROOT build tree: all executables in <build>/bin and
# all libraries in <build>/lib. ROOT_GENERATE_DICTIONARY() puts the .rootmap and
# .pcm files next to the libraries and takes care of installing them as soon as
# CMAKE_LIBRARY_OUTPUT_DIRECTORY is set, so the packages don't have to.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(ccache)
  set(CMAKE_C_COMPILER_LAUNCHER ccache)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

if(testing)
  find_package(GTest)
  # ROOT's test macros and test definitions refer to the ROOT::TestSupport
  # target. Inside the ROOT build, that name is only an ALIAS for the actual
  # library target, and aliases are not exported. An installed ROOT therefore
  # exports the library under its real name, which since ROOT 6.40 is prefixed
  # with the library prefix: ROOT::libTestSupport. Recreate the alias here so
  # that ROOT_ADD_GTEST() and the tests that explicitly link ROOT::TestSupport
  # work with an installed ROOT.
  if(NOT TARGET ROOT::TestSupport AND TARGET ROOT::libTestSupport)
    add_library(ROOT::TestSupport ALIAS ROOT::libTestSupport)
  endif()
  enable_testing()
endif()

option(pyroot "Build the RooFit pythonizations" ON)

# The interpreter is looked up here rather than only in python/, because
# xroofit/test is a Python test too and needs it even when the pythonizations
# are not built. It is not REQUIRED: without an interpreter, only the Python
# tests are skipped. python/CMakeLists.txt asks for it again with REQUIRED,
# since the pythonizations cannot be built without it.
find_package(Python3 COMPONENTS Interpreter)

add_subdirectory(src)

if(pyroot)
  add_subdirectory(python)
endif()

# The tutorials are not built, only run as tests.
if(testing)
  add_subdirectory(tutorials)
endif()

# The tests of the ROOT build tree find everything they need under $ROOTSYS: the
# libraries that are not linked but loaded at runtime (libRooBatchCompute_GENERIC
# and libRooFitMore) together with their .rootmap and .pcm files in $ROOTSYS/lib,
# and the headers that the interpreter parses when RooFit jit-compiles code in
# $ROOTSYS/include. Here, $ROOTSYS is the RooFit-less ROOT installation, so the
# tests have to be pointed to the build tree: to <build>/lib for the libraries,
# and to the include directories of the packages, which are not copied to a
# common directory like ROOT does. That way, "ctest" works in a plain build
# directory without any environment setup.
function(roofit_collect_targets dir result)
  get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
  get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
  foreach(subdir ${subdirs})
    roofit_collect_targets(${subdir} subdir_targets)
    list(APPEND targets ${subdir_targets})
  endforeach()
  set(${result} ${targets} PARENT_SCOPE)
endfunction()

function(roofit_set_test_environment dir modification)
  get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
  foreach(test ${tests})
    set_property(TEST ${test} DIRECTORY ${dir}
                 APPEND PROPERTY ENVIRONMENT_MODIFICATION "${modification}")
  endforeach()
  get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
  foreach(subdir ${subdirs})
    roofit_set_test_environment(${subdir} "${modification}")
  endforeach()
endfunction()

if(testing)
  # set_property(TEST ... DIRECTORY ...) requires CMake 3.28.
  if(CMAKE_VERSION VERSION_LESS 3.28)
    message(WARNING "CMake >= 3.28 is required to set up the library and "
                    "include paths of the tests automatically. Set them by hand "
                    "before running ctest, or install RooFit and source setup.sh.")
  else()
    if(APPLE)
      set(library_path_var DYLD_LIBRARY_PATH)
    else()
      set(library_path_var LD_LIBRARY_PATH)
    endif()
    # <build>/lib holds the libraries and, like $ROOTSYS/lib in a ROOT
    # installation, the Python modules, so it goes on both search paths.
    #
    # $ROOTSYS/lib goes on PYTHONPATH as well, so that "import ROOT" works even
    # when the caller has not sourced thisroot.sh. Our <build>/lib/ROOT/ has no
    # __init__.py and is therefore only a namespace package, which Python ranks
    # below the regular ROOT package of the installation: "import ROOT" always
    # resolves to the real one, and sitecustomize.py then grafts our
    # pythonizations onto it.
    # <build>/bin goes on PATH because some tests and tutorials shell out to the
    # executables built here: the HistFactory tutorials run prepareHistFactory
    # and hist2workspace, which $ROOTSYS/bin does not have in a RooFit-less ROOT.
    set(test_environment
        "${library_path_var}=path_list_prepend:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        "PYTHONPATH=path_list_prepend:${ROOT_LIBRARY_DIR}"
        "PYTHONPATH=path_list_prepend:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        "PATH=path_list_prepend:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    roofit_collect_targets(${CMAKE_CURRENT_SOURCE_DIR}/src all_targets)

    set(include_paths)
    foreach(target ${all_targets})
      get_target_property(type ${target} TYPE)
      if(NOT type STREQUAL "SHARED_LIBRARY")
        continue()
      endif()
      get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
      if(NOT dirs)
        # Libraries that are only loaded at runtime, like RooBatchCompute_GENERIC,
        # have no interface include directories at all.
        continue()
      endif()
      foreach(dir ${dirs})
        # Unwrap $<BUILD_INTERFACE:...> and skip any other generator expression,
        # which would confuse the ENVIRONMENT_MODIFICATION syntax below.
        string(REGEX REPLACE "^\\$<BUILD_INTERFACE:(.*)>$" "\\1" dir "${dir}")
        if(NOT dir MATCHES "\\$<")
          list(APPEND include_paths "${dir}")
        endif()
      endforeach()
    endforeach()

    list(REMOVE_DUPLICATES include_paths)
    foreach(dir ${include_paths})
      list(APPEND test_environment "ROOT_INCLUDE_PATH=path_list_prepend:${dir}")
    endforeach()

    list(REMOVE_DUPLICATES test_environment)
    roofit_set_test_environment(${CMAKE_CURRENT_SOURCE_DIR}/src "${test_environment}")
    roofit_set_test_environment(${CMAKE_CURRENT_SOURCE_DIR}/tutorials "${test_environment}")
    if(pyroot)
      roofit_set_test_environment(${CMAKE_CURRENT_SOURCE_DIR}/python "${test_environment}")
    endif()
  endif()
endif()
