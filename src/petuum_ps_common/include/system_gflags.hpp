#pragma once

#include <glog/logging.h>
#include <gflags/gflags.h>

#include <petuum_ps_common/include/configs.hpp>
#include <petuum_ps_common/util/utils.hpp>

DEFINE_string(stats_path, "", "stats file path prefix");

// Topology Configs
DEFINE_int32(num_clients, 1, "total number of clients");
DEFINE_int32(num_comm_channels_per_client, 1, "no. of comm channels per client");
DEFINE_bool(init_thread_access_table, false, "whether init thread accesses table");
DEFINE_int32(num_table_threads, 1, "no. of worker threads per client");
DEFINE_int32(client_id, 0, "This client's ID");
DEFINE_string(hostfile, "", "path to Petuum PS server configuration file");

// Execution Configs
DEFINE_string(consistency_model, "SSPPush", "SSPAggr/SSPPush/SSP");

// SSPAggr Configs -- client side
DEFINE_uint64(bandwidth_mbps, 40, "per-thread bandwidth limit, in mbps");
DEFINE_uint64(bg_idle_milli, 10, "Bg idle millisecond");

DEFINE_uint64(thread_oplog_batch_size, 100*1000*1000, "thread oplog batch size");

// SSPAggr Configs -- server side
DEFINE_uint64(row_candidate_factor, 5, "server row candidate factor");
DEFINE_int32(server_idle_milli, 10, "server idle time out in millisec");
DEFINE_string(update_sort_policy, "Random", "Update sort policy");

// Snapshot Configs
DEFINE_int32(snapshot_clock, -1, "snapshot clock");
DEFINE_int32(resume_clock, -1, "resume clock");
DEFINE_string(snapshot_dir, "", "snap shot directory");
DEFINE_string(resume_dir, "", "resume directory");

// numa flags
DEFINE_bool(numa_opt, false, "numa opt on?");
DEFINE_int32(numa_index, 0, "numa node index");
DEFINE_string(numa_policy, "Even", "numa policy");
DEFINE_bool(naive_table_oplog_meta, true, "naive table oplog meta");
DEFINE_bool(suppression_on, false, "suppression on");
DEFINE_bool(use_approx_sort, true, "use_approx_sort");

namespace petuum {
void InitTableGroupConfig(TableGroupConfig *config, int32_t num_tables) {
  config->stats_path = FLAGS_stats_path;
  config->num_comm_channels_per_client = FLAGS_num_comm_channels_per_client;
  config->num_tables = num_tables;
  config->num_total_clients = FLAGS_num_clients;
  config->num_local_app_threads = FLAGS_init_thread_access_table ?
                                  FLAGS_num_table_threads : FLAGS_num_table_threads + 1;

  GetHostInfos(FLAGS_hostfile, &(config->host_map));

  config->client_id = FLAGS_client_id;

  config->consistency_model = GetConsistencyModel(FLAGS_consistency_model);

  config->aggressive_clock = false;
  config->aggressive_cpu = false;
  config->server_ring_size = 0;

  config->snapshot_clock = FLAGS_snapshot_clock;
  config->resume_clock = FLAGS_resume_clock;
  config->snapshot_dir = FLAGS_snapshot_dir;
  config->resume_dir = FLAGS_resume_dir;

  config->update_sort_policy = GetUpdateSortPolicy(FLAGS_update_sort_policy);

  config->bg_idle_milli = FLAGS_bg_idle_milli;
  config->bandwidth_mbps = FLAGS_bandwidth_mbps;
  config->thread_oplog_batch_size = FLAGS_thread_oplog_batch_size;
  config->row_candidate_factor = FLAGS_row_candidate_factor;
  config->server_idle_milli = FLAGS_server_idle_milli;

  config->numa_opt = FLAGS_numa_opt;
  config->numa_index = FLAGS_numa_index;

  if (FLAGS_numa_opt) {
    if (FLAGS_numa_policy == "Even") {
      config->numa_policy = Even;
    } else if (FLAGS_numa_policy == "Center") {
      config->numa_policy = Center;
    } else {
      LOG(FATAL) << "Unknown NUMA policy = " << FLAGS_numa_policy;
    }
  }

  config->naive_table_oplog_meta = FLAGS_naive_table_oplog_meta;
  config->suppression_on = FLAGS_suppression_on;
  config->use_approx_sort = FLAGS_use_approx_sort;
}

}
