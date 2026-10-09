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

## Minuit 2

This repository also contains [Minuit 2](https://github.com/root-project/root/tree/master/math/minuit2),
since minimizer development is increasingly driven by RooFit needs. By default,
RooFit is built against this builtin Minuit 2, so the two can be developed
together. Configure with `-Dbuiltin_minuit2=OFF` to use the Minuit 2 of the ROOT
installation instead.

`math/minuit2/` is a copy of the same directory in the ROOT repository, and it
is kept byte-identical to it: syncing is a plain copy of the directory, and
changes made here apply to ROOT as they are.

The builtin Minuit 2 has to live in the same process as the Minuit 2 of the ROOT
installation, which ROOT loads whenever something else asks for the `"Minuit2"`
minimizer. To avoid clashes, the build compiles a renamed copy of the sources
that it generates in `<build>/math/minuit2/`:

| | ROOT | builtin |
| --- | --- | --- |
| library | `libMinuit2` | `libRooFitMinuit2` |
| namespace | `ROOT::Minuit2` | `ROOT::RooFitMinuit2` |
| headers | `Minuit2/*.h` | `RooFitMinuit2/*.h` |
| minimizer plugin | `"Minuit2"` | `"RooFitMinuit2"` |

Always edit the files in `math/minuit2/`; the generated copies are rewritten
whenever the originals change. They start with a `#line` directive that points
back to the originals, so compiler errors and debuggers show the right files.

RooFit keeps calling the minimizer `"Minuit2"`, e.g. in `RooFit::Minimizer("Minuit2")`
or as the default minimizer, but creates the builtin one for it. The RooFit
code that uses Minuit 2 classes directly does so via the namespace alias
`RooFit::Detail::Minuit2`, and creates minimizers via
`RooFit::Detail::createMinimizer()`, both in
[`src/roofitcore/res/RooMinuit2Helpers.h`](src/roofitcore/res/RooMinuit2Helpers.h).
All the details are in [`math/CMakeLists.txt`](math/CMakeLists.txt).

The Minuit 2 tests are built with the other tests, and they all have `minuit2`
in their names:

```bash
cd build
ctest -R minuit2
```

## Clad

This repository also contains [clad](https://github.com/vgvassilev/clad), the
Clang plugin for automatic differentiation that RooFit's `codegen` backend uses
for its gradients and Hessians. By default, RooFit takes its derivatives from
this builtin clad, so that RooFit and clad can be developed together. Configure
with `-Dbuiltin_clad=OFF` to use the clad inside the ROOT installation instead,
which then has to be a ROOT built with clad.

`clad/` is a copy of the clad repository and is kept identical to it, so that
syncing is a plain copy of the directory and changes made here apply to clad
as they are. All of clad's targets, the `clad.so` plugin, `cladDifferentiator`
and `clad-tblgen`, are part of this build, so edits anywhere in `clad/` compile
on `cmake --build`. Until they are upstream, the copy carries two changes that
this integration needed:

* `clad/CMakeLists.txt`: the `CLAD_BUILT_STANDALONE` opt-in, which lets a
  host project embed clad with `add_subdirectory()`.
* `clad/tools/ClangPlugin.cpp`: `#pragma clad checkpoint loop` in an included
  header was attributed to functions of the main file, because the pragma
  locations were compared by raw source offset instead of in translation unit
  order. That broke `RooFit::Detail::MathFuncs::constraintSum()` as soon as
  the generated code lived in a file of its own.

### Why the builtin clad does not run inside the interpreter

ROOT links clad statically into libCling and hides all of its Clang and LLVM
symbols. A clad built here can therefore neither bind to the Clang inside the
interpreter nor register itself as a plugin there, so, unlike the builtin
Minuit 2, it cannot be a renamed copy that slots into the same place.

Instead, it works the way ACLiC works, with clad in the picture. When the
`codegen` backend needs a derivative, RooFit writes the generated function and
the clad request to a file, compiles that with `clang++` and the `clad.so`
plugin into a shared library in a separate process, loads the library, and
takes the derivative from it as a plain symbol. The primal function keeps
coming from the interpreter as before. The compile-and-load step is
`RooFit::Detail::BuiltinClad::Library` in
[`src/clad/inc/RooFit/Detail/BuiltinClad.h`](src/clad/inc/RooFit/Detail/BuiltinClad.h),
and the RooFit side of it is `RooFuncWrapper` in
`src/roofitcore/src/RooEvaluatorWrapper.cxx`. The design and its trade-offs
are documented in [`src/clad/CMakeLists.txt`](src/clad/CMakeLists.txt).

This has a few consequences that are good for development:

* The builtin clad can be built against any LLVM that clad supports, which
  need not be the LLVM inside ROOT. The flake uses LLVM 22.
* The generated source, the code that clad generated, and the compiled library
  with debug info are all files on disk, in a temporary directory that is
  removed at exit. Set `ROOFIT_CLAD_WORKDIR=<dir>` to keep them in `<dir>`,
  and `ROOFIT_CLAD_EXTRA_FLAGS` to pass extra flags to the compiler.
* The clad of the ROOT installation stays available in the interpreter, e.g.
  for the tutorials and for `#include <Math/CladDerivator.h>` in macros.

The price is one compiler process per derivative. Two things keep that in
check:

* Fits of the same model generate the same code up to the numbering of the
  functions and the comments, which carry the current values of the nodes. The
  compiled libraries are cached per process, keyed on the code with the
  numbering canonicalized and the comments stripped, so toy studies and
  repeated fits of a model compile once.
* Parsing the headers takes a couple of seconds, the function itself a fraction
  of that. The headers are therefore precompiled once per process, and each
  translation unit is compiled on top of that precompiled header. One header
  stays out of it: `RooFit/Detail/MathFuncs.h` carries a `#pragma clad`
  directive, which the clad plugin only sees while the header is lexed, and a
  precompiled header is not lexed again. The headers that it includes are
  precompiled instead, see `cladPreamble()` in `RooEvaluatorWrapper.cxx`.

### Building

The build needs the CMake packages of LLVM and Clang, and the `clang++` that
belongs to them:

```bash
cmake -DLLVM_DIR=<llvm>/lib/cmake/llvm -DClang_DIR=<llvm>/lib/cmake/clang ..
```

`clang++` is looked for next to the LLVM tools; pass `-Dclad_compiler=<path>`
if it lives elsewhere. The nix dev shell sets all of this up, see
`$CONFIGURE_ARGS`. With `-Dclad_tests=ON`, clad's own unit tests and lit tests
are built too and available as the `check-clad` target.

The RooFit tests that exercise the builtin clad are the ones of the `codegen`
backend, e.g. `testRooFuncWrapper` and `test-stressroofit-codegen`, and
`testBuiltinClad` tests the compile-and-load step on its own.

## What you can expect from this repo in the future

* Integration of the RooFit parts from [roottest](https://github.com/root-project/roottest/tree/master/root/roofitstats) and [rootbench](https://github.com/root-project/rootbench/tree/master/root/roofit).
* Mirror also RooFit/RooStats tutorials for testing
* Integration of key experiment frameworks for increased test coverage
* Improved CI setup that also includes the RooFit CUDA backend
* Reducing divergence in the CMake code in ROOT and this repo
