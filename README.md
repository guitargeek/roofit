# RooFit Standalone Development Repository

This is a standalone version of [RooFit](https://root.cern/manual/roofit/), a statistical modeling library that is part of [ROOT](https://root.cern).

This repository is for **development purposes only** :warning::wrench::construction:. RooFit is released as part of ROOT, but for contributing to RooFit and testing different versions of it, it is inconvenient that it has to be built together with ROOT, as a full ROOT build can take a very long time :watch:!

Note that this repository is *work in progress*. It doesn't support building all components of RooFit yet, and it also doesn't include the RooFit pythonizations.

## Instructions

### Getting a ROOT build without RooFit

The first step is to get a ROOT build that was compiled without RooFit as a base for your developments.
In other words, a ROOT build that was configured with `-Droofit=OFF`.
Such builds are collected on [this website with custom ROOT binaries](https://rembserj.web.cern.ch/data/binaries/).
If binaries for your preferred platform are not available, please get in touch with us.

### Build RooFit from this repository

Make sure you set up the **RooFit-less** ROOT build correctly, e.g. with `source root/bin/thisroot.sh`.

Then, working with this repository is not different from other CMake projects.

Since you're probably going to do development and debugging, it is recommended to create a `RelWithDebInfo` build (don't use `Debug`, all the extra asserts and lacking optimization will make RooFit very slow).

```bash
git clone git@github.com:guitargeek/roofit.git
cd roofit
mkdir build
cd build
cmake -Dtesting=ON -Dmathmore=ON -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --target install -j16
```

Finally, you need to setup the environment variables for RooFit.

Assuming you're in the `roofit` repository directory and using bash, this would be done like:
```bash
export ROOT_INCLUDE_PATH=install/include
export LD_LIBRARY_PATH=install/lib:$LD_LIBRARY_PATH
```

That's it! Please hack away and submit pull requests :smiley:

We will take care of porting them to the main ROOT repository correctly.

## What you can expect from this repo in the future

* Integration of the RooFit parts from [roottest](https://github.com/root-project/roottest/tree/master/root/roofitstats) and [rootbench](https://github.com/root-project/rootbench/tree/master/root/roofit).
* Include [RooFit pythonizations](https://github.com/root-project/root/tree/master/bindings/pyroot/pythonizations/python/ROOT/_pythonization/_roofit) in the repo
* Mirror also RooFit/RooStats tutorials for testing
* Integration of key experiment frameworks for increased test coverage
* Improved CI setup that also includes the RooFit CUDA backend
* If relevant, it will also mirror [Minuit 2](https://github.com/root-project/root/tree/master/math/minuit2), since minimizer development is increasingly driven by RooFit needs
* Reducing divergence in the CMake code in ROOT and this repo
