# Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

# The RooFit tutorials as tests, the same way the ROOT build runs them: every
# macro is executed with `root.exe -b -q` (or the Python interpreter) and is
# considered failed when it returns non-zero or prints something matching one of
# the FAILREGEX patterns. The latter is what does most of the work, because the
# interpreter happily returns 0 after an "Error in <...>".
#
# In ROOT this lives in tutorials/CMakeLists.txt, which covers all of ROOT in
# more than a thousand lines. What is mirrored here is the RooFit part of it:
# the same invocation, the same failure regexes, and the same vetoes and
# inter-tutorial dependencies. The paths differ by one level, because this
# repository drops the redundant "roofit/" prefix:
#
#     this repository        ROOT repository
#     tutorials/roofit/      tutorials/roofit/roofit/
#     tutorials/roostats/    tutorials/roofit/roostats/
#     tutorials/histfactory/ tutorials/roofit/histfactory/
#
# so a veto that reads roofit/roofit/rf208_convolution.C in ROOT is written
# roofit/rf208_convolution.C here. Test names follow the same rule:
# tutorial-roofit-roofit-rf101_basics becomes tutorial-roofit-rf101_basics.

set(ROOT_root_CMD ${ROOT_BINDIR}/root.exe)

# PYTHONPATH, LD_LIBRARY_PATH and ROOT_INCLUDE_PATH are not set here: the
# top-level CMakeLists.txt appends them to every test in this directory as an
# ENVIRONMENT_MODIFICATION, so that the tutorials pick up the RooFit of this
# build tree rather than looking for one in $ROOTSYS.
set(TUTORIAL_ENV
    MKL_NUM_THREADS=1
    MPLBACKEND=AGG
    OMP_NUM_THREADS=1
    OPENBLAS_NUM_THREADS=1
    ROOT_BATCH=1
)

# Tutorials write files into the current directory, and some read back what an
# earlier one wrote (see the dependencies below), so they all share this one
# scratch directory: it is the working directory of every test defined here.
#
# The rootlogon.C points ACLiC at it too, otherwise rf104_classfactory would
# compile its generated class next to the sources of this repository.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rootlogon.C "{
  // Needed by ACLiC to use the current directory for scratch area
  gSystem->SetBuildDir(\".\", kTRUE);
}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rootalias.C "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rootlogoff.C "{}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/.rootrc "
Rint.History:  .root_hist
ACLiC.LinkLibs:  1
")

# Some tutorials (rf711, rf712 and rf512) read an input file from
# gROOT->GetTutorialDir(), which is $ROOTSYS/tutorials -- the tutorials of the
# RooFit-less ROOT build, not the ones mirrored here. ROOT ships its tutorial
# tree in full regardless of the roofit build option, so the file is there, but
# a stripped-down ROOT installation would not have it.
if(NOT EXISTS "${ROOT_BINDIR}/../tutorials/roofit/roofit/input_histos_rf_lagrangianmorph.root")
  message(WARNING
    "The tutorials of the ROOT installation were not found. The rf512, rf711 "
    "and rf712 tutorial tests read an input file from there and will fail.")
endif()

#---Tutorials disabled depending on the build components-------------

# Not tutorials: helper macros used by the tutorials proper.
set(extra_veto histfactory/makeExample.C
               histfactory/ModifyInterpolation.C
               roostats/StandardFrequentistDiscovery.C)

#---These ones require a display to run------------------------------
set(gui_veto roostats/ModelInspector.C
             roostats/ModelInspector.py)

if(NOT xml)
  # HistFactory's XML model parser is only built with xml=ON. Most of the
  # roostats tutorials need it too, because they create their test data with
  # HistFactory.
  set(xml_veto histfactory/*.C
               histfactory/*.py
               roostats/*.C
               roostats/*.py)
endif()

if(NOT mathmore)
  set(mathmore_veto roostats/TestNonCentral.C
                    roostats/TestNonCentral.py)
endif()

if(NOT fftw3)
  # Without FFTW, RooFFTConvPdf falls back to a jitted DFT that is far too slow
  # for these tutorials.
  set(fftw3_veto roofit/rf208_convolution.C
                 roofit/rf210_angularconv.C
                 roofit/rf211_paramconv.C
                 roofit/rf512_wsfactory_oper.C
                 roofit/rf208_convolution.py
                 roofit/rf210_angularconv.py
                 roofit/rf211_paramconv.py
                 roofit/rf512_wsfactory_oper.py)
endif()

if(NOT ROOT_dataframe_FOUND)
  set(dataframe_veto roofit/rf408_RDataFrameToRooFit.C
                     roofit/rf408_RDataFrameToRooFit.py)
endif()

#---Tutorials that cannot work in this standalone build--------------
#
# The libraries built here have no C++ modules: ROOT's runtime_cxxmodules makes
# the interpreter know every declaration of libRooFit(Core) without an include,
# and cppyy resolves Python lookups the same way. Here the interpreter only
# knows what a macro includes explicitly and what the dictionaries select, so
# the tutorials that rely on that implicit knowledge cannot run. This is the
# same limitation that RooFit/CodegenContext.cxx works around by including
# RooFit/CodegenImpl.h by hand in this repository.
set(no_cxxmodules_veto
    # Calls bindFunction(const char*, ...), declared in RooCFunction1Binding.h,
    # which neither the macro nor RooTFnBinding.h includes. Python fails in the
    # same way, on RooFit::_bindFunction.
    roofit/rf105_funcbinding.C
    roofit/rf105_funcbinding.py
    # RooClassFactory is not selected by any LinkDef, so PyROOT cannot find it.
    # The C++ version works, because the macro includes RooClassFactory.h.
    roofit/rf104_classfactory.py
    # Use RooStats::NumberCountingUtils and RooStats::PValueToSignificance
    # without including RooStats/NumberCountingUtils.h. A namespace of free
    # functions is not autoloaded from a rootmap the way a class is.
    roostats/Zbi_Zgamma.C
    roostats/Zbi_Zgamma.py
    roostats/rs_numbercountingutils.C
    roostats/rs_numbercountingutils.py
    # These make the interpreter parse the whole libRooFitCore dictionary
    # payload, which does not compile: the payload includes the headers in one
    # translation unit in list order, and the cycle between RooGlobalFunc.h and
    # RooAbsReal.h then leaves RooFit::MPSplit, RooFit::MsgTopic and
    # RooFit::Detail::createCmdList undeclared. ROOT does not hit this because
    # its C++ modules make the payload unnecessary. This is a property of the
    # standalone build rather than of these tutorials, so removing them from
    # this list is the way to check whether it has been fixed.
    roostats/HybridInstructional.C
    roostats/HybridStandardForm.C
    roostats/HybridStandardForm.py
    roostats/MultivariateGaussianTest.C
    roostats/rs401c_FeldmanCousins.C
    roostats/rs401d_FeldmanCousins.C
    roostats/StandardHypoTestInvDemo.py
    roostats/HybridInstructional.py
    )

set(standalone_veto
    ${no_cxxmodules_veto}
    # Reads the output of the df106_HiggsToFourLeptons dataframe tutorial,
    # which is not part of this repository.
    roofit/rf618_mixture_models.py
    )

set(all_veto ${extra_veto}
             ${gui_veto}
             ${xml_veto}
             ${mathmore_veto}
             ${fftw3_veto}
             ${dataframe_veto}
             ${standalone_veto}
             )

#---Dependencies------------------------------------------------------
# A tutorial that reads what another one wrote has to run after it. The
# variable name is the test name without the "tutorial-" prefix, exactly as in
# ROOT, so that these lines port over unchanged.
set(roofit-rf503_wspaceread-depends tutorial-roofit-rf502_wspacewrite)

#--Many histfactory and roostats tutorials need the example workspace that
#--CreateExampleFile writes first.
foreach(tname histfactory-hf001_example
              roostats-OneSidedFrequentistUpperLimitWithBands
              roostats-StandardBayesianMCMCDemo
              roostats-StandardBayesianNumericalDemo
              roostats-StandardFeldmanCousinsDemo
              roostats-StandardHistFactoryPlotsWithCategories
              roostats-StandardHypoTestDemo
              roostats-StandardHypoTestInvDemo
              roostats-StandardProfileInspectorDemo
              roostats-StandardProfileLikelihoodDemo
              roostats-StandardTestStatDistributionDemo
              roostats-TwoSidedFrequentistUpperLimitWithBands)
  set(${tname}-depends tutorial-roostats-CreateExampleFile)
endforeach()

#--List long-running tutorials to label them as "longtest"
set(long_running
    roofit/rf613_global_observables.C
    roofit/rf614_binned_fit_problems.C
    roofit/rf617_simulation_based_inference_multidimensional.py
    roostats/HybridInstructional.C
    roostats/HybridInstructional.py
    roostats/OneSidedFrequentistUpperLimitWithBands.C
    roostats/OneSidedFrequentistUpperLimitWithBands.py
    roostats/StandardHypoTestInvDemo.C
    roostats/StandardHypoTestInvDemo.py
    roostats/StandardTestStatDistributionDemo.C
    roostats/StandardTestStatDistributionDemo.py
    roostats/TwoSidedFrequentistUpperLimitWithBands.C
    roostats/TwoSidedFrequentistUpperLimitWithBands.py
    )
file(GLOB long_running RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${long_running})

#---C++ tutorials-----------------------------------------------------

file(GLOB_RECURSE tutorials RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.C)
file(GLOB tutorials_veto RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${all_veto})

list(LENGTH tutorials nTotal)
if(tutorials_veto)
  list(REMOVE_ITEM tutorials ${tutorials_veto})
endif()
list(LENGTH tutorials nAfterVeto)
message(STATUS "${nAfterVeto}/${nTotal} C++ RooFit tutorials have been activated.")

foreach(t ${tutorials})
  string(REPLACE ".C" "" tname ${t})
  string(REPLACE "/" "-" tname ${tname})

  set(labels tutorial)
  set(timeout 1200)
  if(${t} IN_LIST long_running)
    list(APPEND labels longtest)
    set(timeout 3000)
  endif()

  ROOT_ADD_TEST(tutorial-${tname}
                COMMAND ${ROOT_root_CMD} -b -q ${CMAKE_CURRENT_SOURCE_DIR}/${t}
                PASSRC 0
                FAILREGEX "Error in <" ": error:" "segmentation violation"
                          "FROM HESSE     STATUS=FAILED" "warning: Failed to call"
                LABELS ${labels}
                DEPENDS ${${tname}-depends}
                ENVIRONMENT ${TUTORIAL_ENV}
                TIMEOUT ${timeout})
endforeach()

#---Python tutorials--------------------------------------------------

if(pyroot AND Python3_Interpreter_FOUND)

  file(GLOB_RECURSE pytutorials RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py)

  # Python-specific vetoes.
  set(pyveto histfactory/makeQuickModel.py # not a tutorial
             # TODO: fix the problem and re-enable the tutorial test. The rf615
             # tutorial occasionally fails on cleanup on different platforms,
             # hinting to a PyROOT issue. The covered RooFit functionality is
             # also covered by rf617 (the multidimensional case).
             roofit/rf615_simulation_based_inference.py
             )

  file(GLOB pytutorials_veto RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${all_veto} ${pyveto})

  list(LENGTH pytutorials nTotal)
  if(pytutorials_veto)
    list(REMOVE_ITEM pytutorials ${pytutorials_veto})
  endif()
  list(LENGTH pytutorials nAfterVeto)
  message(STATUS "${nAfterVeto}/${nTotal} Python RooFit tutorials have been activated.")

  #---Python tutorial dependencies------------------------------------
  set(roofit-rf503_wspaceread-depends tutorial-roofit-rf502_wspacewrite-py)
  set(roostats-CreateExampleFile-depends tutorial-roostats-CreateExampleFile)

  # Avoid a race condition: make sure the Python tutorial runs after the C++ one
  # when both write the same file.
  set(roofit-rf104_classfactory-depends tutorial-roofit-rf104_classfactory)
  set(roofit-rf512_wsfactory_oper-depends tutorial-roofit-rf512_wsfactory_oper)

  foreach(tname histfactory-hf001_example
                roostats-OneSidedFrequentistUpperLimitWithBands
                roostats-StandardBayesianMCMCDemo
                roostats-StandardBayesianNumericalDemo
                roostats-StandardFeldmanCousinsDemo
                roostats-StandardHistFactoryPlotsWithCategories
                roostats-StandardHypoTestDemo
                roostats-StandardHypoTestInvDemo
                roostats-StandardProfileInspectorDemo
                roostats-StandardProfileLikelihoodDemo
                roostats-StandardTestStatDistributionDemo
                roostats-TwoSidedFrequentistUpperLimitWithBands)
    set(${tname}-depends tutorial-roostats-CreateExampleFile-py)
  endforeach()

  #---Runtime requirements of the Python tutorials---------------------
  # ROOT expresses these as PYTHON_DEPS fixtures of ROOT_ADD_TEST. Here the
  # modules are simply probed once at configuration time, and the tutorials
  # that need a missing one are not turned into tests: an optional plotting or
  # machine-learning dependency that is not installed should not look like a
  # RooFit regression.
  function(roofit_veto_tutorials_without_module module)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import ${module}"
        RESULT_VARIABLE module_missing
        OUTPUT_QUIET ERROR_QUIET)
    if(module_missing)
      message(STATUS "${module} was not found: skipping ${ARGN}")
      list(REMOVE_ITEM pytutorials ${ARGN})
      set(pytutorials ${pytutorials} PARENT_SCOPE)
    endif()
  endfunction()

  roofit_veto_tutorials_without_module(numpy
      roofit/rf102_dataimport.py
      roofit/rf401_importttreethx.py
      roofit/rf409_NumPyPandasToRooFit.py
      roofit/rf617_simulation_based_inference_multidimensional.py)
  roofit_veto_tutorials_without_module(pandas
      roofit/rf409_NumPyPandasToRooFit.py)
  roofit_veto_tutorials_without_module(sklearn
      roofit/rf617_simulation_based_inference_multidimensional.py)

  foreach(t ${pytutorials})
    string(REPLACE ".py" "" tname ${t})
    string(REPLACE "/" "-" tname ${tname})

    set(labels tutorial)
    set(timeout 1200)
    if(${t} IN_LIST long_running)
      list(APPEND labels longtest)
      set(timeout 3000)
    endif()

    ROOT_ADD_TEST(tutorial-${tname}-py
                  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${t}
                  PASSRC 0
                  FAILREGEX "Error in" ": error:" "segmentation violation"
                  LABELS ${labels}
                  DEPENDS ${${tname}-depends}
                  ENVIRONMENT ${TUTORIAL_ENV}
                  TIMEOUT ${timeout})
  endforeach()

endif()
