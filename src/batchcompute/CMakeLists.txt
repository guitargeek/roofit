# Library which powers fast batch computations in Roofit.

add_library(RooBatchCompute SHARED)

set(sources_cxx
    src/Initialisation.cxx
)

target_sources(RooBatchCompute PRIVATE ${sources_headers} ${sources_cxx})

target_link_libraries(RooBatchCompute PUBLIC
    Core
    MathCore
)

install(TARGETS RooBatchCompute LIBRARY DESTINATION lib)

target_include_directories(RooBatchCompute PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/res>)

if(vdt OR builtin_vdt)
  target_link_libraries(RooBatchCompute PUBLIC VDT::VDT)
endif()
if(builtin_vdt)
  add_dependencies(RooBatchCompute VDT)
endif()


############################################################################################################################################
# Instantiations of the shared objects which provide the actual computation functions.

set(shared_object_sources src/RooBatchCompute.cxx src/ComputeFunctions.cxx)

# Generic implementation for CPUs that don't support vector instruction sets.
add_library(RooBatchCompute_GENERIC SHARED ${shared_object_sources})
target_link_libraries(RooBatchCompute_GENERIC PUBLIC RooBatchCompute)
install(TARGETS RooBatchCompute_GENERIC LIBRARY DESTINATION lib)
target_compile_options(RooBatchCompute_GENERIC  PRIVATE ${common-flags} -DRF_ARCH=GENERIC)

# Windows platform and ICC compiler need special code and testing, thus the feature has not been implemented yet for these.
if (ROOT_PLATFORM MATCHES "linux|macosx" AND CMAKE_SYSTEM_PROCESSOR MATCHES x86_64 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

  target_compile_options(RooBatchCompute PRIVATE -DR__RF_ARCHITECTURE_SPECIFIC_LIBS)

  add_library(RooBatchCompute_SSE4.1 SHARED ${shared_object_sources})
  add_library(RooBatchCompute_AVX    SHARED ${shared_object_sources})
  add_library(RooBatchCompute_AVX2   SHARED ${shared_object_sources})
  add_library(RooBatchCompute_AVX512 SHARED ${shared_object_sources})
  target_link_libraries(RooBatchCompute_SSE4.1 PUBLIC RooBatchCompute)
  target_link_libraries(RooBatchCompute_AVX    PUBLIC RooBatchCompute)
  target_link_libraries(RooBatchCompute_AVX2   PUBLIC RooBatchCompute)
  target_link_libraries(RooBatchCompute_AVX512 PUBLIC RooBatchCompute)
  install(TARGETS RooBatchCompute_SSE4.1 LIBRARY DESTINATION lib)
  install(TARGETS RooBatchCompute_AVX    LIBRARY DESTINATION lib)
  install(TARGETS RooBatchCompute_AVX2   LIBRARY DESTINATION lib)
  install(TARGETS RooBatchCompute_AVX512 LIBRARY DESTINATION lib)

  # Flags -fno-signaling-nans, -fno-trapping-math and -O3 are necessary to enable autovectorization (especially for GCC).
  set(common-flags $<$<CXX_COMPILER_ID:GNU>:-fno-signaling-nans>)
  list(APPEND common-flags $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>: -fno-trapping-math -O3>)

  target_compile_options(RooBatchCompute_SSE4.1  PRIVATE ${common-flags} -msse4    -DRF_ARCH=SSE4)
  target_compile_options(RooBatchCompute_AVX     PRIVATE ${common-flags} -mavx     -DRF_ARCH=AVX)
  target_compile_options(RooBatchCompute_AVX2    PRIVATE ${common-flags} -mavx2    -DRF_ARCH=AVX2)
  target_compile_options(RooBatchCompute_AVX512  PRIVATE ${common-flags} -march=skylake-avx512 -DRF_ARCH=AVX512)

endif() # vector versions of library

if (cuda)
  add_library(RooBatchCompute_CUDA SHARED ${shared_object_sources_cu})
  target_link_libraries(RooBatchCompute_CUDA   PUBLIC RooBatchCompute)
  target_compile_options(RooBatchCompute_CUDA  PRIVATE -lineinfo --expt-relaxed-constexpr)
  install(TARGETS RooBatchCompute_CUDA LIBRARY DESTINATION lib)
endif()
