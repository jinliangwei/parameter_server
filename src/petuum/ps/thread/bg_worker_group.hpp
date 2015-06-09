// author: jinliang
#pragma once

#include <pthread.h>
#include <petuum/ps/thread/abstract_bg_worker.hpp>

namespace petuum {

class BgWorkerGroup {
public:
  BgWorkerGroup(ClientTables *tables);
  virtual ~BgWorkerGroup();

  void Start();

  void ShutDown();
  void AppThreadRegister();
  void AppThreadDeregister();

  // Assuming table does not yet exist
  bool CreateTable(int32_t table_id,
                   const ClientTableConfig& table_config);
  void WaitCreateTable();
  bool RequestRow(int32_t table_id, int32_t row_id, int32_t clock);

  void ClockAllTables();
  void SendOpLogsAllTables();

  virtual void TurnOnEarlyComm();
  virtual void TurnOffEarlyComm();

private:
  virtual void CreateBgWorkers();

  ClientTables *tables_;

  /* Helper Functions */
  std::vector<AbstractBgWorker*> bg_worker_vec_;
  int32_t bg_worker_id_st_;

  pthread_barrier_t init_barrier_;
  pthread_barrier_t create_table_barrier_;

  // Servers might not update all the rows in a client's process cache
  // (there could be a row that nobody writes to for many many clocks).
  // Therefore, we need a locally shared clock to denote the minimum clock that
  // the entire system has reached, which determines if a local thread may read
  // a local cached copy of the row.
  // Each bg worker maintains a vector clock for servers. The clock denotes the
  // clock that the server has reached, that is the minimum clock that the
  // server has seen from all clients.
  // This bg_server_clock_ contains one clock for each bg worker which is the
  // min server clock that the bg worker has seen.
  std::mutex system_clock_mtx_;
  std::condition_variable system_clock_cv_;

  std::atomic_int_fast32_t system_clock_;
  VectorClockMT bg_server_clock_;
};

}
