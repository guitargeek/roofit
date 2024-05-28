# Author: Jonas Rembser CERN 08/2026

################################################################################
# Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.                      #
# All rights reserved.                                                         #
#                                                                              #
# For the licensing terms see $ROOTSYS/LICENSE.                                #
# For the list of contributors see $ROOTSYS/README/CREDITS.                    #
################################################################################

"""Attach the RooFit pythonizations of this repository to an installed ROOT.

In the ROOT repository, the RooFit pythonizations live in the ``ROOT`` Python
package as ``ROOT._pythonization._roofit``, and PyROOT picks them up
automatically: ``ROOT._pythonization._register_pythonizations()`` imports every
submodule it finds in ``ROOT._pythonization.__path__``.

Here, RooFit is built against a ROOT installation that was configured with
``-Droofit=OFF``, which is usually read-only (a system package, a CVMFS
directory, a Nix store path, ...), so we cannot drop our ``_roofit`` directory
into it. Instead, we keep the very same directory layout in this repository and
splice it into ``ROOT._pythonization.__path__`` at import time. The
pythonizations themselves are therefore byte-identical to the ones in the ROOT
repository and can be copied back and forth without any modification.

The splicing is triggered by the ``sitecustomize`` module that sits next to this
one, so that ``import ROOT`` is all a user has to write. If you disabled the
``site`` module (``python -S``) or shadowed our ``sitecustomize``, call
:func:`install` by hand before importing ROOT::

    import _roofit_bootstrap
    _roofit_bootstrap.install()

    import ROOT
"""

import os
import sys

__all__ = ["install", "pythonization_dir"]

# The directory that plays the role of $ROOTSYS/lib/ROOT/_pythonization. It is
# resolved relative to this file so that the build tree and the install tree
# both work, and so that the whole tree stays relocatable.
_PYTHONIZATION_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ROOT", "_pythonization")


def pythonization_dir():
    """Return the directory that is spliced into ``ROOT._pythonization.__path__``."""
    return _PYTHONIZATION_DIR


class _RooFitPythonizationFinder:
    """Meta path finder that extends the search path of ``ROOT._pythonization``.

    We cannot touch ``ROOT._pythonization.__path__`` before the module exists,
    and we do not want to import ROOT eagerly from ``sitecustomize`` (that would
    make every Python process in the environment pay for the ROOT import). So we
    wait on ``sys.meta_path`` until somebody imports ``ROOT._pythonization``,
    resolve the module the way Python would have resolved it anyway, and only
    add our directory to the resulting search path.
    """

    def find_spec(self, fullname, path=None, target=None):
        if fullname != "ROOT._pythonization":
            return None

        # This finder is only ever useful once.
        try:
            sys.meta_path.remove(self)
        except ValueError:
            pass

        # Deliberately not importlib.util.find_spec(): `path` is already the
        # __path__ of the parent package, so we avoid re-entering the import of
        # ROOT itself, which is what got us here in the first place.
        from importlib.machinery import PathFinder

        spec = PathFinder.find_spec(fullname, path)
        if spec is None or spec.submodule_search_locations is None:
            return spec

        _prepend(spec.submodule_search_locations)
        return spec


def _prepend(search_locations):
    """Put our directory first, so a local ``_roofit`` wins over a ROOT-provided one.

    Only ``_roofit`` exists in our directory, so every other pythonization
    module is still resolved from the ROOT installation.
    """
    if _PYTHONIZATION_DIR not in search_locations:
        search_locations.insert(0, _PYTHONIZATION_DIR)


def install():
    """Make ``ROOT._pythonization._roofit`` resolve to the pythonizations here.

    Safe to call more than once, but call it *before* the first RooFit class is
    used. cppyy applies pythonizors when it creates a class proxy, so a class
    that has already been looked up keeps the un-pythonized proxy it got. This
    is a property of PyROOT, not of this module: registering a pythonizor late
    has the same effect in ROOT itself. Calling it before ``import ROOT``, as
    the ``sitecustomize`` module next to this one does, always works.
    """
    if not os.path.isdir(os.path.join(_PYTHONIZATION_DIR, "_roofit")):
        # Not an error: RooFit may have been configured with -Dpyroot=OFF.
        return

    module = sys.modules.get("ROOT._pythonization")
    if module is None:
        # The common case: nobody has imported ROOT yet, so wait for it.
        for finder in sys.meta_path:
            if isinstance(finder, _RooFitPythonizationFinder):
                return
        sys.meta_path.insert(0, _RooFitPythonizationFinder())
        return

    # We are too late for ROOT._pythonization._register_pythonizations(), which
    # imports every pythonization module it finds on the search path, so extend
    # the path *and* do that one import ourselves.
    _prepend(module.__path__)
    if "ROOT._pythonization._roofit" not in sys.modules:
        import importlib

        importlib.import_module("ROOT._pythonization._roofit")
