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
add_subdirectory(hs3)
if(roofit_legacy_eval_backend AND NOT MSVC)
  add_subdirectory(xroofit)
endif()

# The DTD that HistFactory validates its XML model files against. ROOT copies
# it to <build>/etc and installs it into $ROOTSYS/etc, which is where
# prepareHistFactory looks for it (via `root-config --etcdir`). Here $ROOTSYS is
# the RooFit-less ROOT installation and has no copy, so both the build tree and
# the install tree of this repository provide one.
set(roofit_etc_files
  etc/HistFactorySchema.dtd
)

foreach(roofit_etc_file ${roofit_etc_files})
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${roofit_etc_file}" "${CMAKE_BINARY_DIR}/${roofit_etc_file}" COPYONLY)
endforeach()

install(FILES ${roofit_etc_files} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
