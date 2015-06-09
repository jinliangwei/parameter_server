// author: jinliang

#pragma once

#include <petuum/ps/thread/bg_worker_group.hpp>
#include <petuum/ps/client/client_tables.hpp>

namespace petuum {

class BgWorkers {
public:
  static void Start(ClientTables *tables);

  static void ShutDown();
  static void AppThreadRegister();
  static void AppThreadDeregister();

  // Assuming table does not yet exist
  static bool CreateTable(int32_t table_id,
                          const ClientTableConfig& table_config);
  static void WaitCreateTable();
  static bool RequestRow(int32_t table_id, int32_t row_id, int32_t clock);

  static void ClockAllTables();
  static void SendOpLogsAllTables();

  static void TurnOnEarlyComm();
  static void TurnOffEarlyComm();

private:
  static BgWorkerGroup *bg_worker_group_;
};

}
