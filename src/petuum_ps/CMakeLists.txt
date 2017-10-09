#
# CMakeLists.txt  cmake file for the petuum_ps lib
#

set (PETUUM_PS_SRC client/client_table.cpp client/table_group.cpp
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


add_library (petuum_ps ${PETUUM_PS_SRC})

#
# "make install" rules
#
install (TARGETS petuum_ps EXPORT petuum_ps-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
