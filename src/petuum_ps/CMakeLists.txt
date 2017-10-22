#
# CMakeLists.txt  cmake file for the petuum_ps lib
# 21-Oct-2017  chuck@ece.cmu.edu
#

# list of soruce files
set (petuum_ps_src client/client_table.cpp client/table_group.cpp
    client/thread_table.cpp consistency/ssp_aggr_consistency_controller.cpp
    consistency/ssp_aggr_value_consistency_controller.cpp
    consistency/ssp_consistency_controller.cpp
    consistency/ssp_push_append_only_consistency_controller.cpp
    consistency/ssp_push_consistency_controller.cpp
    oplog/append_only_oplog_partition.cpp oplog/create_row_oplog.cpp
    oplog/dense_oplog.cpp oplog/oplog_index.cpp oplog/sparse_oplog.cpp
    server/adarevision_server_table_logic.cpp server/name_node.cpp
    server/name_node_thread.cpp server/server.cpp server/server_table.cpp
    server/server_thread.cpp server/server_thread_group.cpp
    server/server_threads.cpp server/ssp_aggr_server_thread.cpp
    server/ssp_push_server_thread.cpp thread/abstract_bg_worker.cpp
    thread/append_only_row_oplog_buffer.cpp thread/bg_oplog_partition.cpp
    thread/bg_worker_group.cpp thread/bg_workers.cpp thread/context.cpp
    thread/fixed_table_oplog_meta_dense.cpp thread/naive_table_oplog_meta.cpp
    thread/numa_mgr.cpp thread/random_table_oplog_meta.cpp
    thread/random_table_oplog_meta_dense.cpp thread/row_oplog_serializer.cpp
    thread/row_request_oplog_mgr.cpp thread/server_version_mgr.cpp
    thread/ssp_aggr_bg_worker.cpp thread/ssp_bg_worker.cpp
    thread/ssp_push_bg_worker.cpp thread/ssp_push_bg_worker_group.cpp
    thread/ssp_push_row_request_oplog_mgr.cpp
    thread/value_table_oplog_meta.cpp
    thread/value_table_oplog_meta_approx.cpp)

#
# configure/load in standard modules we plan to use and probe the enviroment
#
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (petuum_ps_pkgloc "share/cmake/petuum_ps")

#
# library version set here (e.g. for shared libs).
#
set (PETUMM_PS_VERSION_MAJOR 1)
set (PETUMM_PS_VERSION_MINOR 0)
set (PETUMM_PS_VERSION_PATCH 0)
set (petuum_ps_vers "${PETUMM_PS_VERSION_MAJOR}.${PETUMM_PS_VERSION_MINOR}")
set (PETUMM_PS_VERSION "${petuum_ps_vers}.${PETUMM_PS_VERSION_PATCH}")

#
# create library target (user can specify shared vs. static using
# BUILD_SHARED_LIBS).  configure include files and targets that we
# depend on ...
#
add_library (petuum_ps ${petuum_ps_src})

# libs we use that are exposed to our users (so they need -I, etc.)
# this will pull in all of petuum_ps_common's public libs
target_link_libraries (petuum_ps PUBLIC petuum_ps_common)

# includes for installed lib, and for when building
target_include_directories (petuum_ps PUBLIC
                            $<INSTALL_INTERFACE:include>)
target_include_directories (petuum_ps BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

if (CMAKE_THREAD_LIBS_INIT)
  target_link_libraries (petuum_ps PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

set_target_properties(petuum_ps
                      PROPERTIES VERSION ${PETUMM_PS_VERSION}
                      SOVERSION ${PETUMM_PS_VERSION_MAJOR})

#
# installation stuff (packaging and install commands)
#
write_basic_package_version_file(
    "petuum_ps-config-version.cmake"
    VERSION ${PETUMM_PS_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
configure_file (petuum_ps-config.cmake.in
                petuum_ps-config.cmake @ONLY)


#
# "make install" rules
#
install (TARGETS petuum_ps EXPORT petuum_ps-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT petuum_ps-targets
         DESTINATION ${petuum_ps_pkgloc}
         FILE "petuum_ps-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/petuum_ps-config.cmake"
         "${CMAKE_CURRENT_BINARY_DIR}/petuum_ps-config-version.cmake"
         DESTINATION ${petuum_ps_pkgloc} )
install (DIRECTORY ../petuum_ps
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
