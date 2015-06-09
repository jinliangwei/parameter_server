#include <glog/logging.h>
#include <gflags/gflags.h>

#include <petuum/include/configs.hpp>

// just provide the definitions for simplicity

DEFINE_int32(staleness, 0, "table staleness");
DEFINE_int32(row_type, 0, "table row type");
DEFINE_int32(row_oplog_type, 0, "row oplog type");
DEFINE_bool(oplog_dense_serialized, true, "dense serialized oplog");

DEFINE_string(index_type, "Dense", "use append only oplog?");
DEFINE_bool(no_oplog_replay, false, "oplog replay?");

DEFINE_uint64(server_push_row_upper_bound, 100, "Server push row threshold");
DEFINE_uint64(client_send_oplog_upper_bound, 100, "client send oplog upper bound");
DEFINE_int32(server_table_logic, -1, "server table logic");
DEFINE_bool(version_maintain, false, "version maintain");
