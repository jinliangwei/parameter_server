#
# CMakeLists.txt  cmake file for the petuum_ps_common lib
#

set (PETUUM_PS_COMMON_SRC comm_bus/comm_bus.cpp comm_bus/zmq_util.cpp
  include/init_table_config.cpp include/init_table_group_config.cpp
  include/ps_table_group.cpp include/system_gflags.cpp include/table_gflags.cpp
  oplog/batch_inc_append_only_buffer.cpp oplog/dense_append_only_buffer.cpp
  oplog/inc_append_only_buffer.cpp storage/bounded_dense_process_storage.cpp
  storage/bounded_sparse_process_storage.cpp
  storage/clock_lru.cpp thread/msg_tracker.cpp util/high_resolution_timer.cpp
  util/lock.cpp util/max_vector_clock.cpp util/sparse_vector.cpp
  util/stats.cpp util/timer_thr.cpp util/utils.cpp util/vector_clock.cpp
  util/vector_clock_mt.cpp)


add_library (petuum_ps_common ${PETUUM_PS_COMMON_SRC})

#
# "make install" rules
#
install (TARGETS petuum_ps_common EXPORT petuum_ps_common-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
