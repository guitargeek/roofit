# Builds RooFit's builtin clad from the copy of the clad repository in clad/,
# and libRooFitClad, the library through which RooFit uses it.
#
# The copy in clad/ is kept identical to the clad repository, so that it can be
# synced by copying the directory over, and so that clad developments made here
# can be applied upstream as they are. The only difference is the opt-in
# CLAD_BUILT_STANDALONE in clad/CMakeLists.txt, which lets a host project embed
# clad with add_subdirectory(). Everything else in clad/ is used as it is: clad
# is configured as a standalone project against the LLVM and Clang found via
# LLVM_DIR and Clang_DIR, and its targets (the clad.so plugin, cladDifferentiator,
# clad-tblgen) are part of this build, so edits in clad/ compile on
# cmake --build like any other change.
#
# How the builtin clad coexists with the clad of the ROOT installation
# --------------------------------------------------------------------
#
# ROOT builds clad as static libraries and links them whole-archive into
# libCling, which hides all of its Clang and LLVM symbols behind a version
# script. A clad built here can therefore neither bind to the Clang inside the
# interpreter nor register itself as a plugin there, and the interpreter never
# loads plugins from shared libraries anyway. So, unlike the builtin Minuit2,
# the builtin clad cannot be a renamed copy that slots into the same place. It
# does not touch the interpreter at all.
#
# Instead, it works the way ACLiC works, with clad in the picture: RooFit writes
# the code of a function and the clad request for its derivative to a file,
# compiles that with clang and the clad.so plugin into a shared library in a
# separate process, loads the library, and takes the derivative from it as a
# plain symbol. libRooFitClad implements the compile-and-load step, see
# inc/RooFit/Detail/BuiltinClad.h, and the RooFit side of it lives in
# RooFuncWrapper (src/roofitcore/src/RooEvaluatorWrapper.cxx). The clad of the
# ROOT installation stays available in the interpreter for everything else,
# like the tutorials.
#
# The builtin clad can be built against any LLVM that clad supports. It does
# not have to be the LLVM of the ROOT installation, since the two never meet in
# the same compiler. The clang++ that runs the plugin has to belong to that
# LLVM, see clad_compiler below.

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG HINTS "${LLVM_DIR}/../clang" "${LLVM_INSTALL_PREFIX}/lib/cmake/clang")
message(STATUS "Building the builtin clad against LLVM ${LLVM_PACKAGE_VERSION} (${LLVM_DIR})")

# The compiler that runs the plugin at runtime. It has to be the clang of the
# LLVM that clad is built against, which is where it is looked for first.
find_program(clad_compiler NAMES clang++
  HINTS "${LLVM_TOOLS_BINARY_DIR}" "${CLANG_INSTALL_PREFIX}/bin"
  DOC "The clang++ that runs the builtin clad plugin. Must belong to the LLVM in LLVM_DIR.")
if(NOT clad_compiler)
  message(FATAL_ERROR "No clang++ found for the builtin clad. Point clad_compiler to the clang++ of LLVM ${LLVM_PACKAGE_VERSION}.")
endif()
execute_process(COMMAND "${clad_compiler}" --version
  OUTPUT_VARIABLE clad_compiler_version ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT clad_compiler_version MATCHES "clang version ${LLVM_VERSION_MAJOR}\\.")
  message(FATAL_ERROR "clad_compiler=${clad_compiler} is not a clang of LLVM ${LLVM_VERSION_MAJOR}. It reports:\n${clad_compiler_version}")
endif()
message(STATUS "The builtin clad runs with ${clad_compiler}")

# Embed clad/ as if it were configured on its own, see the comment at the top.
# LLVM's HandleLLVMOptions probes the C compiler too, which this project does
# not enable otherwise.
enable_language(C)
set(CLAD_BUILT_STANDALONE 1)
if(clad_tests)
  set(CLAD_DISABLE_TESTS OFF)
else()
  set(CLAD_DISABLE_TESTS ON)
endif()
add_subdirectory(${PROJECT_SOURCE_DIR}/clad ${PROJECT_BINARY_DIR}/clad)

# The language standard that the compiled code is parsed with, matching the
# RooFit headers that it includes.
set(clad_cxx_standard ${CMAKE_CXX_STANDARD})
if(NOT clad_cxx_standard)
  set(clad_cxx_standard 20)
endif()

add_library(RooFitClad SHARED
  src/BuiltinClad.cxx
  inc/RooFit/Detail/BuiltinClad.h
)

target_include_directories(RooFitClad PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
  $<INSTALL_INTERFACE:include>
)

# Core for the include path of the interpreter, which the compiled code shares.
target_link_libraries(RooFitClad PUBLIC Core)

# The plugin and the clad headers have to be there when the library is used.
add_dependencies(RooFitClad clad)

# Where the pieces are in the build tree. At runtime, the library first looks
# next to itself, which is where everything is in the install tree.
target_compile_definitions(RooFitClad PRIVATE
  ROOFIT_CLAD_COMPILER="${clad_compiler}"
  ROOFIT_CLAD_CXX_STANDARD="${clad_cxx_standard}"
  ROOFIT_CLAD_PLUGIN="$<TARGET_FILE:clad>"
  ROOFIT_CLAD_PLUGIN_FILE="$<TARGET_FILE_NAME:clad>"
  ROOFIT_CLAD_INCLUDE_DIRS="${PROJECT_SOURCE_DIR}/clad/include:${PROJECT_BINARY_DIR}/clad/include"
)

install(TARGETS RooFitClad LIBRARY DESTINATION lib)
install(DIRECTORY inc/ DESTINATION include)

if(testing)
  add_subdirectory(test)
endif()
