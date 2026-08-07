# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

# In ROOT this is a ROOT_ADD_PYUNITTEST(xroofit_python xroofit_python.py) call.
# That macro bakes the PYTHONPATH of the configuring shell into the test and
# has its test driver set it inside the test process, which overwrites the
# ENVIRONMENT_MODIFICATION that the top-level CMakeLists.txt uses to point the
# tests at this build tree. The test would then only pass for a developer who
# had sourced thisroot.sh before configuring. Use a plain add_test() instead,
# like python/test/ does, so that ctest works in a plain build directory.

# -B: don't write __pycache__ next to the sources of the repository.
add_test(NAME xroofit_python
         COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
                 -s ${CMAKE_CURRENT_SOURCE_DIR} -p xroofit_python.py -v)
