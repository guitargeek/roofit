# Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

# Tests for the RooFit pythonizations, mirrored from
# bindings/pyroot/pythonizations/test/ in the ROOT repository.
#
# The tests run against the build tree: the top-level CMakeLists.txt puts
# <build>/lib on PYTHONPATH (for the pythonizations) and on the library search
# path (for the RooFit libraries) of every test, so `ctest` works in a plain
# build directory without sourcing anything.

# numpy is an optional dependency of the pythonizations, but the tests of
# RooDataSet.to_numpy() and friends need it. Rather than letting a third of the
# test file fail with a ModuleNotFoundError, say so once at configuration time.
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import numpy"
    RESULT_VARIABLE numpy_missing
    OUTPUT_QUIET ERROR_QUIET)
if(numpy_missing)
  message(WARNING
    "numpy was not found for ${Python3_EXECUTABLE}. The pyroot_roofit test "
    "needs it for the RooAbsData/RooDataSet numpy interfaces and will fail.")
endif()

# -B: don't write __pycache__ next to the sources of the repository.
add_test(NAME pyroot_roofit
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/roofit.py -v)
