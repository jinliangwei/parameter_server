#include <petuum/ps/thread/bg_workers.hpp>
#include <petuum/include/configs.hpp>
#include <petuum/ps/thread/bg_worker_group.hpp>
#include <petuum/ps/thread/global_context.hpp>

namespace petuum {
BgWorkerGroup *BgWorkers::bg_worker_group_;

void BgWorkers::Start(ClientTables *tables) {
  bg_worker_group_ = new BgWorkerGroup(tables);
  bg_worker_group_->Start();
}

void BgWorkers::ShutDown() {
  bg_worker_group_->ShutDown();
  delete bg_worker_group_;
}

void BgWorkers::AppThreadRegister() {
  bg_worker_group_->AppThreadRegister();
}

void BgWorkers::AppThreadDeregister() {
  bg_worker_group_->AppThreadDeregister();
}

bool BgWorkers::CreateTable(int32_t table_id,
                            const ClientTableConfig& table_config) {
  return bg_worker_group_->CreateTable(table_id, table_config);
}

void BgWorkers::WaitCreateTable() {
  bg_worker_group_->WaitCreateTable();
}

bool BgWorkers::RequestRow(int32_t table_id, int32_t row_id, int32_t clock) {
  return bg_worker_group_->RequestRow(table_id, row_id, clock);
}

void BgWorkers::ClockAllTables() {
  bg_worker_group_->ClockAllTables();
}

void BgWorkers::SendOpLogsAllTables() {
  bg_worker_group_->SendOpLogsAllTables();
}

void BgWorkers::TurnOnEarlyComm() {
  bg_worker_group_->TurnOnEarlyComm();
}

void BgWorkers::TurnOffEarlyComm() {
  bg_worker_group_->TurnOffEarlyComm();
}

}
