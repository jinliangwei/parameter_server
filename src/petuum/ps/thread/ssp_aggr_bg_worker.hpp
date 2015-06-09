//author: jinliang

#pragma once

#include <petuum/ps/thread/ssp_push_bg_worker.hpp>
#include <petuum/ps_common/util/high_resolution_timer.hpp>

namespace petuum {

class SSPAggrBgWorker : public SSPPushBgWorker {
public:
  SSPAggrBgWorker(int32_t id, int32_t comm_channel_idx,
                  ClientTables *tables,
                  pthread_barrier_t *init_barrier,
                  pthread_barrier_t *create_table_barrier,
                  std::atomic_int_fast32_t *system_clock,
                  std::mutex *system_clock_mtx,
                  std::condition_variable *system_clock_cv,
                  VectorClockMT *bg_server_clock):
      SSPPushBgWorker(id, comm_channel_idx, tables,
                      init_barrier, create_table_barrier,
                      system_clock,
                      system_clock_mtx,
                      system_clock_cv,
                      bg_server_clock),
      min_table_staleness_(INT_MAX),
      oplog_send_milli_sec_(0),
      suppression_level_(0),
      suppression_level_min_(0),
      clock_tick_sec_(0),
    suppression_on_(false),
    early_comm_on_(false) {
    //ResetBgIdleMilli_ = &SSPAggrBgWorker::ResetBgIdleMilliNoEarlyComm;
  }

  ~SSPAggrBgWorker() { }

protected:
  virtual void SetWaitMsg();
  virtual void PrepareBeforeInfiniteLoop();
  virtual long ResetBgIdleMilli();
  virtual long BgIdleWork();
  virtual long HandleClockMsg(bool clock_advanced);

  void HandleEarlyCommOn();
  void HandleEarlyCommOff();

  long ResetBgIdleMilliNoEarlyComm();
  long ResetBgIdleMilliEarlyComm();

  typedef long (SSPAggrBgWorker::*ResetBgIdleMilliFunc)();

  virtual void HandleAdjustSuppressionLevel();

  int32_t min_table_staleness_;

  HighResolutionTimer msg_send_timer_;
  double oplog_send_milli_sec_;

  HighResolutionTimer clock_timer_;
  int32_t suppression_level_;
  int32_t suppression_level_min_;
  double clock_tick_sec_;
  ResetBgIdleMilliFunc ResetBgIdleMilli_;

  bool suppression_on_;

  bool early_comm_on_;
};

}
