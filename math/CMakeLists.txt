# Builds RooFit's builtin Minuit2, libRooFitMinuit2, from the copy of ROOT's
# math/minuit2 directory in math/minuit2.
#
# The copy is kept identical to ROOT's, so that it can be synced by just copying
# the directory over, and so that Minuit2 developments made here can be applied
# to ROOT as they are. Nothing in math/minuit2 is used by the build except the
# sources, headers and tests.
#
# The builtin Minuit2 has to coexist with the Minuit2 of the ROOT installation
# in the same process, because ROOT loads its own libMinuit2 for any fit that
# asks for the "Minuit2" minimizer. Building the sources as they are would
# result in two libraries that define the same symbols and dictionaries, which
# would be mixed up at runtime. Therefore, the build uses a copy of the sources
# in the build tree, where everything that would clash is renamed:
#
#   * the namespace ROOT::Minuit2 becomes ROOT::RooFitMinuit2
#   * the header directory Minuit2/ becomes RooFitMinuit2/
#   * the include guards ROOT_Minuit2_* become ROOT_RooFitMinuit2_*
#   * the class TMinuit2TraceObject becomes TRooFitMinuit2TraceObject
#
# The string "Minuit2" is not renamed, so the Minuit2Minimizer of the builtin
# Minuit2 still takes its extra options from
# ROOT::Math::MinimizerOptions::Default("Minuit2"). The renamed copies start
# with a #line directive that refers to the original file, and the renaming
# preserves line numbers, so that compiler errors and debuggers point to the
# files in math/minuit2, which are the ones to edit.
#
# The builtin Minuit2 is registered as the "RooFitMinuit2" plugin for
# ROOT::Math::Minimizer by RooFitMinuit2Plugin.cxx. RooFit uses it whenever the
# "Minuit2" minimizer is requested (see RooFit::Detail::createMinimizer() in
# src/roofitcore/res/RooMinuit2Helpers.h).

set(minuit2_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/minuit2)
set(minuit2_build_dir ${CMAKE_CURRENT_BINARY_DIR}/minuit2)

# Writes the renamed copy of a file from math/minuit2 to the build tree. The
# paths are relative to math/minuit2 and to the build tree directory of the
# copy, respectively. With RENAME_PLUGIN, the "Minuit2" minimizer that is
# requested from the ROOT::Math::Factory is also renamed, which is done for the
# tests so that they test the builtin Minuit2.
function(roofit_rename_minuit2 input output)
  cmake_parse_arguments(ARG "RENAME_PLUGIN" "" "" ${ARGN})

  file(READ ${minuit2_source_dir}/${input} content)

  # Protect the "Minuit2" strings from the renaming below: the minimizer name,
  # and messages that end with "Minuit2".
  if(ARG_RENAME_PLUGIN)
    string(REPLACE "\"Minuit2\"" "\"RooFitMinuit2\"" content "${content}")
  else()
    string(REPLACE "\"Minuit2\"" "@ROOFIT_MINUIT2_STRING@" content "${content}")
  endif()
  string(REPLACE "Minuit2\"" "@ROOFIT_MINUIT2_STRING_END@" content "${content}")

  # Rename the namespace and the header directory, i.e. every "Minuit2" that is
  # not part of a longer identifier. The CMake regex engine supports neither
  # word boundaries nor lookarounds, so the characters around "Minuit2" are part
  # of the match. A second pass catches the cases where two matches overlap
  # because they share these characters.
  foreach(pass 1 2)
    string(REGEX REPLACE "([^A-Za-z0-9_])Minuit2([^A-Za-z0-9_])" "\\1RooFitMinuit2\\2" content "${content}")
  endforeach()

  string(REPLACE "ROOT_Minuit2_" "ROOT_RooFitMinuit2_" content "${content}")
  string(REPLACE "TMinuit2TraceObject" "TRooFitMinuit2TraceObject" content "${content}")

  string(REPLACE "@ROOFIT_MINUIT2_STRING@" "\"Minuit2\"" content "${content}")
  string(REPLACE "@ROOFIT_MINUIT2_STRING_END@" "Minuit2\"" content "${content}")

  set(content "#line 1 \"${minuit2_source_dir}/${input}\"\n${content}")

  # Only write the file if it changed, so that the files that depend on it are
  # not rebuilt for nothing when CMake runs again.
  set(output ${minuit2_build_dir}/${output})
  if(EXISTS ${output})
    file(READ ${output} old_content)
    if(old_content STREQUAL content)
      return()
    endif()
  endif()
  file(WRITE ${output} "${content}")
endfunction()

# Writes the renamed copies of the files that match the given globbing
# expressions, and returns the paths of the copies in <output_var>. CMake runs
# again if any of the input files changes, or if files are added or removed.
function(roofit_rename_minuit2_files output_var)
  cmake_parse_arguments(ARG "RENAME_PLUGIN" "" "GLOBS" ${ARGN})
  set(rename_args)
  if(ARG_RENAME_PLUGIN)
    set(rename_args RENAME_PLUGIN)
  endif()

  file(GLOB_RECURSE inputs CONFIGURE_DEPENDS RELATIVE ${minuit2_source_dir} ${ARG_GLOBS})
  list(SORT inputs)

  set(outputs)
  foreach(input ${inputs})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${minuit2_source_dir}/${input})
    set(output ${input})
    string(REPLACE "inc/Minuit2/" "inc/RooFitMinuit2/" output "${output}")
    string(REPLACE "TMinuit2TraceObject" "TRooFitMinuit2TraceObject" output "${output}")
    roofit_rename_minuit2(${input} ${output} ${rename_args})
    list(APPEND outputs ${minuit2_build_dir}/${output})
  endforeach()

  set(${output_var} ${outputs} PARENT_SCOPE)
endfunction()

roofit_rename_minuit2_files(minuit2_headers GLOBS
  ${minuit2_source_dir}/inc/Minuit2/*.h
  ${minuit2_source_dir}/inc/TMinuit2TraceObject.h
)
roofit_rename_minuit2_files(minuit2_sources GLOBS
  ${minuit2_source_dir}/src/*.cxx
  ${minuit2_source_dir}/src/*.h
)
# The LinkDef.h is not put in the include directory, because that one is
# installed.
roofit_rename_minuit2(inc/LinkDef.h LinkDef.h)

add_library(RooFitMinuit2 SHARED)

target_sources(RooFitMinuit2 PRIVATE
  ${minuit2_headers}
  ${minuit2_sources}
  RooFitMinuit2Plugin.cxx
)

target_include_directories(RooFitMinuit2 PUBLIC
  $<BUILD_INTERFACE:${minuit2_build_dir}/inc>
  $<INSTALL_INTERFACE:include>
)

# These are the DEPENDENCIES in the ROOT build, which are linked publicly there.
target_link_libraries(RooFitMinuit2 PUBLIC
  Core
  MathCore
  Hist
)

# Like in the ROOT build: report errors via ROOT's error handler.
target_compile_definitions(RooFitMinuit2 PRIVATE USE_ROOT_ERROR)

# The dictionary is needed by the plugin manager, which creates the
# Minuit2Minimizer via the interpreter.
set(minuit2_dict_headers ${minuit2_headers})
list(TRANSFORM minuit2_dict_headers REPLACE "^${minuit2_build_dir}/inc/" "")
ROOT_GENERATE_DICTIONARY(G__RooFitMinuit2 ${minuit2_dict_headers}
    LINKDEF ${minuit2_build_dir}/LinkDef.h
    MODULE RooFitMinuit2
    OPTIONS --deep
)

install(TARGETS RooFitMinuit2 LIBRARY DESTINATION lib)
install(DIRECTORY ${minuit2_build_dir}/inc/ DESTINATION include)

#---Tests: like math/minuit2/test/CMakeLists.txt in ROOT-------------------------------

if(NOT testing)
  return()
endif()

roofit_rename_minuit2_files(minuit2_test_files RENAME_PLUGIN GLOBS
  ${minuit2_source_dir}/test/*.cxx
  ${minuit2_source_dir}/test/*.h
)
set(test_dir ${minuit2_build_dir}/test)

# The executables are prefixed with "minuit2_", which is also the prefix of the
# test names in ROOT, to avoid clashes with the targets of the RooFit tests.

foreach(name Quad1FMain Quad4FMain Quad8FMain Quad12FMain)
  ROOT_EXECUTABLE(minuit2_${name} ${test_dir}/MnTutorial/${name}.cxx LIBRARIES RooFitMinuit2)
  ROOT_ADD_TEST(minuit2_${name} COMMAND minuit2_${name})
endforeach()

add_library(RooFitMinuit2TestMnSim STATIC
  ${test_dir}/MnSim/GaussDataGen.cxx
  ${test_dir}/MnSim/GaussFcn.cxx
  ${test_dir}/MnSim/GaussFcn2.cxx
)
target_link_libraries(RooFitMinuit2TestMnSim PUBLIC RooFitMinuit2)

foreach(file paul.txt paul2.txt paul3.txt paul4.txt)
  configure_file(${minuit2_source_dir}/test/MnSim/${file} ${file} COPYONLY)
endforeach()

foreach(name DemoGaussSim DemoFumili PaulTest PaulTest2 PaulTest3 PaulTest4 ReneTest ParallelTest demoMinimizer)
  ROOT_EXECUTABLE(minuit2_${name} ${test_dir}/MnSim/${name}.cxx LIBRARIES RooFitMinuit2 RooFitMinuit2TestMnSim MathCore)
  ROOT_ADD_TEST(minuit2_${name} COMMAND minuit2_${name})
endforeach()

# These tests use the minimizer via the plugin manager.
set(minuit2_test_root_libraries Core RIO Net Hist Graf Graf3d Gpad Tree
    Rint Postscript Matrix Physics MathCore Thread)
set(minuit2_plugin_tests testMinimizer testCovariance)
# testADMinim differentiates in the interpreter, which needs the clad of the
# ROOT installation, not the builtin one.
if(ROOT_clad_FOUND)
  list(APPEND minuit2_plugin_tests testADMinim)
endif()
foreach(name ${minuit2_plugin_tests})
  ROOT_EXECUTABLE(minuit2_${name} ${test_dir}/${name}.cxx LIBRARIES ${minuit2_test_root_libraries} RooFitMinuit2)
  ROOT_ADD_TEST(minuit2_${name} COMMAND minuit2_${name})
  # These tests reach the builtin Minuit2 only through the plugin manager and
  # reference none of its symbols, so a linker that drops unreferenced
  # libraries (--as-needed, the default on Ubuntu) would not link
  # libRooFitMinuit2 at all. Then the "RooFitMinuit2" plugin handler, which the
  # library registers when it is loaded, does not exist and the minimizer
  # cannot be created. Keep the library linked regardless.
  if(UNIX AND NOT APPLE)
    target_link_options(minuit2_${name} PRIVATE LINKER:--no-as-needed)
  endif()
endforeach()

if(TARGET GTest::gtest)
  ROOT_ADD_GTEST(minuit2_testMinuit2 ${test_dir}/testMinuit2.cxx LIBRARIES RooFitMinuit2)
endif()
