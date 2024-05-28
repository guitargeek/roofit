# RooFit Standalone Development Repository

This is a standalone version of [RooFit](https://root.cern/manual/roofit/), a statistical modeling library that is part of [ROOT](https://root.cern).

This repository is for **development purposes only** :warning::wrench::construction:. RooFit is released as part of ROOT, but for contributing to RooFit and testing different versions of it, it is inconvenient that it has to be built together with ROOT, as a full ROOT build can take a very long time :watch:!

Note that this repository is *work in progress*. It doesn't support building all components of RooFit yet.

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
export PYTHONPATH=install/lib:$PYTHONPATH
```

or simply with `source setup.sh` (`source setup.fish` for fish users).

That's it! Please hack away and submit pull requests :smiley:

We will take care of porting them to the main ROOT repository correctly.

## The RooFit pythonizations

The Python part of RooFit lives in `python/` and mirrors the layout of the ROOT
repository one to one:

| this repository | ROOT repository |
| --- | --- |
| `python/ROOT/_pythonization/_roofit/` | `bindings/pyroot/pythonizations/python/ROOT/_pythonization/_roofit/` |
| `python/test/roofit.py` | `bindings/pyroot/pythonizations/test/roofit.py` |

The files are byte-identical to their ROOT counterparts, so porting a change in
either direction is a plain copy.

### How they are hooked into PyROOT

In ROOT, PyROOT finds the pythonizations because they are part of the installed
`ROOT` Python package: `ROOT._pythonization` imports every submodule it finds in
its own `__path__`. Here, we build against a ROOT installation configured with
`-Droofit=OFF`, which is usually read-only, so we cannot put our `_roofit`
directory into it.

Instead, the build tree keeps the same directory layout next to the libraries —
just like `$ROOTSYS/lib` in a ROOT installation:

```
<build|install>/lib/ROOT/_pythonization/_roofit/*.py
<build|install>/lib/_roofit_bootstrap.py
<build|install>/lib/sitecustomize.py
```

Putting `<build|install>/lib` on `PYTHONPATH` (which `setup.sh` does) makes
Python pick up the `sitecustomize` module at interpreter startup, and that
prepends our directory to `ROOT._pythonization.__path__`. From there on, PyROOT
behaves exactly as it does in ROOT:

```python
import ROOT  # no extra import needed

x = ROOT.RooRealVar("x", "x", -10, 10)
```

If you run with `python -S`, or if something else already owns the
`sitecustomize` name, do it explicitly instead:

```python
import _roofit_bootstrap
_roofit_bootstrap.install()

import ROOT
```

Do this before the first RooFit class is used: cppyy applies pythonizations when
it creates a class proxy, so a class that was already looked up keeps its
un-pythonized proxy.

The gory details are documented in
[`python/_roofit_bootstrap.py`](python/_roofit_bootstrap.py).

### Developing

The pythonizations are pure Python, so the edit/test cycle is short: the build
just copies the changed files into `<build>/lib`.

```bash
cd build
cmake --build .            # takes well under a second for a Python-only change
ctest -R pyroot_roofit     # run the pythonization tests
```

Adding a new pythonization is a matter of dropping a file into
`python/ROOT/_pythonization/_roofit/` and listing it in that directory's
`__init__.py`; CMake picks up new files by itself.

If you prefer to work against the installed tree (`source setup.sh`), run
`cmake --build . --target install` and use `python` as usual.

Pass `-Dpyroot=OFF` to CMake if you don't want the Python part at all. numpy is
needed for the `to_numpy()` interfaces and their tests, pandas for
`RooAbsData.to_pandas()`.

## What you can expect from this repo in the future

* Integration of the RooFit parts from [roottest](https://github.com/root-project/roottest/tree/master/root/roofitstats) and [rootbench](https://github.com/root-project/rootbench/tree/master/root/roofit).
* Mirror also RooFit/RooStats tutorials for testing
* Integration of key experiment frameworks for increased test coverage
* Improved CI setup that also includes the RooFit CUDA backend
* If relevant, it will also mirror [Minuit 2](https://github.com/root-project/root/tree/master/math/minuit2), since minimizer development is increasingly driven by RooFit needs
* Reducing divergence in the CMake code in ROOT and this repo
