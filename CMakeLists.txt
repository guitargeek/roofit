cmake_minimum_required(VERSION 3.16)
project(RooFit
    VERSION 6.32.00
    DESCRIPTION "RooFit"
    LANGUAGES CXX)


find_package(ROOT REQUIRED COMPONENTS Core MathCore Minuit Tree)
include(${ROOT_USE_FILE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "header files")
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "libraries")

if(ccache)
  set(CMAKE_C_COMPILER_LAUNCHER ccache)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

if(testing)
  enable_testing()
endif()

add_subdirectory(src)
