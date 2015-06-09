// author: jinliang

#pragma once
#include <petuum/ps/thread/abstract_bg_worker.hpp>
#include <petuum/ps/client/client_tables.hpp>
#include <glog/logging.h>

namespace petuum {
class SSPPushBgWorker : public AbstractBgWorker {
public:
  SSPPushBgWorker(int32_t id, int32_t comm_channel_idx,
                    ClientTables *tables,
                    pthread_barrier_t *init_barrier,
                    pthread_barrier_t *create_table_barrier,
                    std::atomic_int_fast32_t *system_clock,
                    std::mutex *system_clock_mtx,
                    std::condition_variable *system_clock_cv,
                    VectorClockMT *bg_server_clock):
      AbstractBgWorker(id, comm_channel_idx, tables,
                    init_barrier, create_table_barrier),
    system_clock_mtx_(system_clock_mtx),
    system_clock_cv_(system_clock_cv),
    system_clock_(system_clock),
    bg_server_clock_(bg_server_clock) {
    for (const auto &server_id : server_ids_) {
      server_vector_clock_.AddClock(server_id);
    }
  }

  virtual ~SSPPushBgWorker(){ }

protected:
  virtual void HandleServerPushRow(int32_t sender_id,
                                   ServerPushRowMsg &server_push_row_msg);
  void ApplyServerPushedRow(void *mem, size_t mem_size);

  virtual void PrepareBeforeInfiniteLoop();
  // invoked after all tables have been created
  virtual long ResetBgIdleMilli();
  virtual long BgIdleWork();

  // shared with SSPBgWorkerGroup
  std::mutex *system_clock_mtx_;
  std::condition_variable *system_clock_cv_;
  std::atomic_int_fast32_t *system_clock_;
  VectorClockMT *bg_server_clock_;

  VectorClock server_vector_clock_;
};
}
