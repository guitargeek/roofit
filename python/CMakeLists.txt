# Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

# The RooFit pythonizations. They are pure Python, so there is nothing to
# compile: "building" them means mirroring this directory into the build tree,
# from where they are installed.
#
# The layout follows the one of a ROOT installation, where the Python modules
# live next to the libraries in $ROOTSYS/lib:
#
#   <build|install>/lib/ROOT/_pythonization/_roofit/*.py
#   <build|install>/lib/_roofit_bootstrap.py
#   <build|install>/lib/sitecustomize.py
#
# Putting that one directory on PYTHONPATH is therefore all it takes to use
# them, exactly like `source thisroot.sh` does for ROOT itself. What the two
# extra modules at the top level are for is explained in _roofit_bootstrap.py.

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# PyROOT is a CPython extension: the interpreter that imports ROOT has to be the
# one ROOT was built against. ROOT checks this at import time and raises a
# readable ImportError, but a warning at configuration time is friendlier than a
# failing test suite.
set(root_python_version_file "${ROOT_LIBRARY_DIR}/ROOT/_python_version.py")
if(EXISTS "${root_python_version_file}")
  file(STRINGS "${root_python_version_file}" root_python_version_line
       REGEX "^_root_python_version[ \t]*=")
  string(REGEX MATCH "[0-9]+\\.[0-9]+" root_python_version "${root_python_version_line}")
  if(root_python_version AND
     NOT root_python_version VERSION_EQUAL "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
    message(WARNING
      "ROOT was built for Python ${root_python_version}, but CMake found Python "
      "${Python3_VERSION}. The RooFit pythonizations will be unusable with this "
      "interpreter. Point CMake at the right one with -DPython3_EXECUTABLE=...")
  endif()
endif()

# The .py files are globbed so that adding a pythonization is a matter of
# dropping a file into ROOT/_pythonization/_roofit/, just like in ROOT.
# CONFIGURE_DEPENDS makes the build system notice new files by itself.
file(GLOB_RECURSE roofit_python_files
     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     CONFIGURE_DEPENDS
     ROOT/_pythonization/_roofit/*.py)
list(APPEND roofit_python_files _roofit_bootstrap.py sitecustomize.py)

set(roofit_python_output_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# Copying (instead of symlinking the directory) keeps the build tree and the
# install tree identical, and stale .py files of deleted pythonizations cannot
# linger in the build tree because CONFIGURE_DEPENDS re-runs the glob.
set(roofit_python_copies)
foreach(file ${roofit_python_files})
  set(output ${roofit_python_output_dir}/${file})
  add_custom_command(
      OUTPUT ${output}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${output}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file}
      COMMENT "Copying ${file} to the build tree"
      VERBATIM)
  list(APPEND roofit_python_copies ${output})
endforeach()

add_custom_target(RooFitPythonizations ALL DEPENDS ${roofit_python_copies})

# __pycache__ directories are deliberately not installed: they are created next
# to the sources at runtime and would be stale after the next build.
install(DIRECTORY ROOT DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILES_MATCHING PATTERN "*.py")
install(FILES _roofit_bootstrap.py sitecustomize.py
        DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(testing)
  add_subdirectory(test)
endif()
