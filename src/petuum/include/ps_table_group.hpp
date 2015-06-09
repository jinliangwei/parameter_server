/*
 * ps_table_group.hpp
 * author: jinliang
 */

#pragma once

#include <map>
#include <cstdint>

#include <petuum/include/configs.hpp>
#include <petuum/include/table.hpp>

#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/ps_common/oplog/abstract_row_oplog.hpp>
#include <petuum/ps_common/util/class_register.hpp>
#include <petuum/ps_common/client/abstract_table_group.hpp>
#include <petuum/ps_common/client/abstract_client_table.hpp>

#include <petuum/ps/client/table_group.hpp>

namespace petuum {

class PSTableGroup {
public:
  // Can be called only once per process. Must be called after RegisterRow() to
  // be a barrier for CreateTable(), and "happen before" any other call to
  // TableGroup. The thread that calls Init() is refered to as the init
  // thread.  If the init thread needs to access table API (e.g., init thread
  // itself being a worker thread), it should set table_access to true. Init
  // thread is responsible for calling RegisterRow(), CreateTable() and
  // ShutDown(). Calling those functions from other threads is not allowed.
  // Init thread does not need to DeregisterThread() nor RegisterThread().
  static int Init(const TableGroupConfig &table_group_config,
                  bool table_access) {
    int32_t init_thread_id;
    abstract_table_group_ = new TableGroup(
        table_group_config, table_access, &init_thread_id);
    return init_thread_id;
  }

  // Init thread need to call ShutDown() after all other app threads have
  // deregistered. Any other call to TableGroup and Table API must return
  // before calling ShutDown().
  static void ShutDown() {
    delete abstract_table_group_;
  }

  // Should be called before Init(). Not thread-safe. We strongly recommend to
  // call RegisterRow from init thread to avoid race condition.
  template<typename ROW>
  static void RegisterRow(int32_t row_type) {
    ClassRegistry<AbstractRow>::GetRegistry().AddCreator(
        row_type, CreateObj<AbstractRow, ROW>);
  }

  // Should be called before Init(). Not thread-safe. We strongly recommend to
  // call RegisterRow from init thread to avoid race condition.
  template<typename ROW>
  static void RegisterRowOpLog(int32_t row_oplog_type) {
    ClassRegistry<AbstractRowOpLog>::GetRegistry().AddCreator(
        row_oplog_type, CreateObj<AbstractRowOpLog, ROW>);
  }

  static bool CreateTable(int32_t table_id,
                          const ClientTableConfig& table_config) {
    return abstract_table_group_->CreateTable(table_id, table_config);
  }

  // Must be called by Init thread after creating all tables and before any
  // other thread calls RegisterThread().
  static void CreateTableDone() {
    abstract_table_group_->CreateTableDone();
  }

  // Called by Init thread only before it access any table API.
  // Must be called after CreateTableDone().
  // If Init thread does not access table API, it makes no difference calling
  // this function.
  static void WaitThreadRegister() {
    abstract_table_group_->WaitThreadRegister();
  }

  // GetTableOrDie is thread-safe with respect to other calls to
  // GetTableOrDie() Getter, terminate if table is not found.
  template<typename UPDATE>
  static Table<UPDATE> GetTableOrDie(int32_t table_id) {
    AbstractClientTable *abstract_table
        = abstract_table_group_->GetTableOrDie(table_id);
    return Table<UPDATE>(abstract_table);
  }

  // A app threads except init thread should register itself before accessing
  // any Table API. In SSP mode, if a thread invokes RegisterThread with
  // true, its clock will be kept track of, so it should call Clock()
  // properly.
  static int32_t RegisterThread() {
    return abstract_table_group_->RegisterThread();
  }

  // A registered thread must deregister itself.
  static void DeregisterThread() {
    return abstract_table_group_->DeregisterThread();
  }

  // Advance clock for the application thread.
  static void Clock() {
    return abstract_table_group_->Clock();
  }

  // Called by application threads that access table API
  // (referred to as table threads).
  // Threads that calls GlobalBarrier must be at the same clock.
  // 1) A table thread may not go beyond the barrier until all table threads
  // have reached the barrier;
  // 2) Table threads that move beyond the barrier are guaranteed to see
  // the updates that other table threads apply to the table.
  static void GlobalBarrier() {
    return abstract_table_group_->GlobalBarrier();
  }

  static void TurnOnEarlyComm() {
    return abstract_table_group_->TurnOnEarlyComm();
  }

  static void TurnOffEarlyComm() {
    return abstract_table_group_->TurnOffEarlyComm();
  }

private:
  static AbstractTableGroup *abstract_table_group_;
};

}   // namespace petuum
