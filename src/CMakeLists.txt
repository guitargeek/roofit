# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

set(roofit_legacy_eval_backend ON CACHE BOOL "" FORCE)

add_subdirectory(codegen)
add_subdirectory(batchcompute)
#if (roofit_multiprocess)
  #add_subdirectory(roofitZMQ)
  #add_subdirectory(multiprocess)
#endif()
add_subdirectory(roofitcore)
add_subdirectory(roofit)
if(mathmore)
  add_subdirectory(roofitmore)
endif()
add_subdirectory(roostats)
add_subdirectory(histfactory)
add_subdirectory(jsoninterface)
add_subdirectory(hs3)
#if(roofit_legacy_eval_backend AND NOT MSVC)
  #add_subdirectory(xroofit)
#endif()
