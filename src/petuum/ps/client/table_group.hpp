/*
 * table_group.hpp
 * author: jinliang
 */

#pragma once

#include <map>
#include <cstdint>

#include <petuum/include/configs.hpp>
#include <petuum/include/table.hpp>
#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/ps_common/util/class_register.hpp>
#include <petuum/ps_common/client/abstract_table_group.hpp>

#include <petuum/ps/client/client_table.hpp>
#include <petuum/ps/client/client_tables.hpp>

namespace petuum {

class TableGroup : public AbstractTableGroup {
public:
  TableGroup(const TableGroupConfig &table_group_config,
             bool table_access, int32_t *init_thread_id);

  ~TableGroup();

  bool CreateTable(int32_t table_id,
      const ClientTableConfig& table_config);

  void CreateTableDone();

  void WaitThreadRegister();

  AbstractClientTable *GetTableOrDie(int32_t table_id);

  int32_t RegisterThread();

  void DeregisterThread();

  void Clock();

  void GlobalBarrier();

  void TurnOnEarlyComm();

  void TurnOffEarlyComm();

private:
  ClientTables tables_;
  pthread_barrier_t register_barrier_;
  std::atomic<int> num_app_threads_registered_;

  // Max staleness among all tables.
  int32_t max_table_staleness_;
  VectorClockMT vector_clock_;
};

}   // namespace petuum
