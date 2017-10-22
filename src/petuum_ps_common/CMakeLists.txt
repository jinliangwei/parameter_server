#
# CMakeLists.txt  cmake file for the petuum_ps_common lib
# 20-Oct-2017  chuck@ece.cmu.edu
#

# list of source files
set (petuum_psc_src comm_bus/comm_bus.cpp comm_bus/zmq_util.cpp
  include/init_table_config.cpp include/init_table_group_config.cpp
  include/ps_table_group.cpp include/system_gflags.cpp include/table_gflags.cpp
  oplog/batch_inc_append_only_buffer.cpp oplog/dense_append_only_buffer.cpp
  oplog/inc_append_only_buffer.cpp storage/bounded_dense_process_storage.cpp
  storage/bounded_sparse_process_storage.cpp
  storage/clock_lru.cpp thread/msg_tracker.cpp util/high_resolution_timer.cpp
  util/lock.cpp util/max_vector_clock.cpp util/sparse_vector.cpp
  util/stats.cpp util/timer_thr.cpp util/utils.cpp util/vector_clock.cpp
  util/vector_clock_mt.cpp)

#
# configure/load in standard modules we plan to use and probe the enviroment
#
include (CMakePackageConfigHelpers)

# pull in all the external targets that we depend on
include (cmake/external-targets.cmake)

# where to install files for "find_package"
set (petuum_psc_pkgloc "share/cmake/petuum_ps_common")

#
# library version set here (e.g. for shared libs).
#
set (PETUMM_PSC_VERSION_MAJOR 1)
set (PETUMM_PSC_VERSION_MINOR 0)
set (PETUMM_PSC_VERSION_PATCH 0)
set (petuum_psc_vers "${PETUMM_PSC_VERSION_MAJOR}.${PETUMM_PSC_VERSION_MINOR}")
set (PETUMM_PSC_VERSION "${petuum_psc_vers}.${PETUMM_PSC_VERSION_PATCH}")

#
# create library target (user can specify shared vs. static using
# BUILD_SHARED_LIBS).  configure include files and targets that we
# depend on ...
#
add_library (petuum_ps_common ${petuum_psc_src})

# tag us with a c++11 feature so that users will have c++11 turned on
# in their compile when they use this target.
# XXX: newer cmakes have a "cxx_std_11" feature that could be used
target_compile_features (petuum_ps_common INTERFACE cxx_constexpr)

# libs we use that are exposed to our users (so they need -I, etc.)
target_link_libraries (petuum_ps_common PUBLIC float16_compressor gflags
                       glog::glog libcuckoo petuum_boost yaml-cpp)

# libs we use but are not exposed externally
target_link_libraries (petuum_ps_common PRIVATE Snappy::snappy zmqhpp
                       Eigen3::Eigen)

# includes for installed lib, and for when building
target_include_directories (petuum_ps_common PUBLIC
                            $<INSTALL_INTERFACE:include>)
target_include_directories (petuum_ps_common BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

if (CMAKE_THREAD_LIBS_INIT)
  target_link_libraries (petuum_ps_common PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

set_target_properties(petuum_ps_common
                      PROPERTIES VERSION ${PETUMM_PSC_VERSION}
                      SOVERSION ${PETUMM_PSC_VERSION_MAJOR})

#
# installation stuff (packaging and install commands)
#
write_basic_package_version_file(
    "petuum_ps_common-config-version.cmake"
    VERSION ${PETUMM_PSC_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
configure_file (cmake/petuum_ps_common-config.cmake.in
                petuum_ps_common-config.cmake @ONLY)

#
# "make install" rules
#
install (TARGETS petuum_ps_common EXPORT petuum_ps_common-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT petuum_ps_common-targets
         DESTINATION ${petuum_psc_pkgloc}
         FILE "petuum_ps_common-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/petuum_ps_common-config.cmake"
         "${CMAKE_CURRENT_BINARY_DIR}/petuum_ps_common-config-version.cmake"
         cmake/libcuckoo.cmake
         cmake/petuum_boost.cmake
         cmake/xpkg-import.cmake
         DESTINATION ${petuum_psc_pkgloc} )
install (DIRECTORY ../petuum_ps_common
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
