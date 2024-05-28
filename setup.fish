set -g -x ROOT_INCLUDE_PATH $PWD/install/include $ROOT_INCLUDE_PATH
set -g -x -a LD_LIBRARY_PATH $PWD/install/lib
set -g -x -a DYLD_LIBRARY_PATH $PWD/install/lib
# Like $ROOTSYS/lib in a ROOT installation, install/lib also holds the Python
# modules: the RooFit pythonizations plus the sitecustomize hook that splices
# them into the ROOT package. See python/_roofit_bootstrap.py.
set -g -x PYTHONPATH $PWD/install/lib $PYTHONPATH
