# Author: Jonas Rembser CERN 08/2026

################################################################################
# Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.                      #
# All rights reserved.                                                         #
#                                                                              #
# For the licensing terms see $ROOTSYS/LICENSE.                                #
# For the list of contributors see $ROOTSYS/README/CREDITS.                    #
################################################################################

"""Hook that activates the RooFit pythonizations of this repository.

Python imports ``sitecustomize`` at interpreter startup, so putting this
directory on ``PYTHONPATH`` (which ``setup.sh`` does) is enough to make a plain
``import ROOT`` see the pythonizations built here. See ``_roofit_bootstrap`` for
what actually happens.

Registering a ``sitecustomize`` module means claiming a global name, so we first
run any other ``sitecustomize`` that we might be shadowing.
"""

import os
import sys

_THIS_DIR = os.path.dirname(os.path.abspath(__file__))


def _run_shadowed_sitecustomize():
    """Execute the ``sitecustomize`` module that this one hides, if there is one."""
    import importlib.util
    from importlib.machinery import PathFinder

    other_paths = [p for p in sys.path if p and os.path.abspath(p) != _THIS_DIR]

    try:
        spec = PathFinder.find_spec("sitecustomize", other_paths)
    except Exception:
        return
    if spec is None or spec.loader is None:
        return

    # Not registered in sys.modules: the import machinery is about to store
    # *this* module under the "sitecustomize" key anyway.
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception:
        import traceback

        print("Error running the sitecustomize module at {}:".format(spec.origin), file=sys.stderr)
        traceback.print_exc()


_run_shadowed_sitecustomize()

import _roofit_bootstrap  # noqa: E402

_roofit_bootstrap.install()
