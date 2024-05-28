export ROOT_INCLUDE_PATH=$PWD/install/include
export LD_LIBRARY_PATH=$PWD/install/lib:$LD_LIBRARY_PATH
#export DYLD_LIBRARY_PATH=$PWD/install/lib:$DYLD_LIBRARY_PATH
# Like $ROOTSYS/lib in a ROOT installation, install/lib also holds the Python
# modules: the RooFit pythonizations plus the sitecustomize hook that splices
# them into the ROOT package. See python/_roofit_bootstrap.py.
export PYTHONPATH=$PWD/install/lib:$PYTHONPATH
