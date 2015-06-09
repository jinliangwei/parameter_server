#include <petuum/ps/thread/bg_worker_group.hpp>
#include <petuum/ps/thread/ssp_push_bg_worker.hpp>
#include <petuum/ps/thread/ssp_aggr_bg_worker.hpp>
#include <petuum/ps/thread/global_context.hpp>

namespace petuum {

BgWorkerGroup::BgWorkerGroup(ClientTables *tables):
    tables_(tables),
    bg_worker_vec_(GlobalContext::get_num_comm_channels_per_client()),
    bg_worker_id_st_(GlobalContext::get_head_bg_id(
        GlobalContext::get_client_id())) {

  pthread_barrier_init(&init_barrier_, NULL,
                       GlobalContext::get_num_comm_channels_per_client() + 1);
  pthread_barrier_init(&create_table_barrier_, NULL,
                       GlobalContext::get_num_comm_channels_per_client() + 1);

  for (int32_t i = 0;
       i < GlobalContext::get_num_comm_channels_per_client(); ++i) {
    bg_server_clock_.AddClock(bg_worker_id_st_ + i, 0);
  }
}

BgWorkerGroup::~BgWorkerGroup() {
  for (auto &worker : bg_worker_vec_) {
    if (worker != 0) {
      delete worker;
      worker = 0;
    }
  }
}

void BgWorkerGroup::CreateBgWorkers() {
  switch (GlobalContext::get_consistency_model()) {
    case SSPPush:
      {
        int32_t idx = 0;
        for (auto &worker : bg_worker_vec_) {
          worker = new SSPPushBgWorker(bg_worker_id_st_ + idx, idx, tables_,
                                       &init_barrier_, &create_table_barrier_,
                                       &system_clock_, &system_clock_mtx_,
                                       &system_clock_cv_,
                                       &bg_server_clock_);
          ++idx;
        }
      }
      break;
    case SSPAggr:
      {
        int32_t idx = 0;
        for (auto &worker : bg_worker_vec_) {
          worker = new SSPAggrBgWorker(bg_worker_id_st_ + idx, idx, tables_,
                                       &init_barrier_, &create_table_barrier_,
                                       &system_clock_, &system_clock_mtx_,
                                       &system_clock_cv_,
                                       &bg_server_clock_);
          ++idx;
        }
      }
      break;
    default:
      LOG(FATAL) << "Unsupported consistency model "
                 << GlobalContext::get_consistency_model();
  }
}

void BgWorkerGroup::Start() {
  CreateBgWorkers();
  for (auto &worker : bg_worker_vec_) {
    worker->Start();
  }
  pthread_barrier_wait(&init_barrier_);
}

void BgWorkerGroup::ShutDown() {
  for (const auto &worker : bg_worker_vec_) {
    worker->ShutDown();
  }
}

void BgWorkerGroup::AppThreadRegister() {
  for (const auto &worker : bg_worker_vec_) {
    worker->AppThreadRegister();
  }
}

void BgWorkerGroup::AppThreadDeregister() {
  for (const auto &worker : bg_worker_vec_) {
    worker->AppThreadDeregister();
  }
}

bool BgWorkerGroup::CreateTable(int32_t table_id,
                                const ClientTableConfig &table_config) {
  return bg_worker_vec_[0]->CreateTable(table_id, table_config);
}

void BgWorkerGroup::WaitCreateTable() {
  pthread_barrier_wait(&create_table_barrier_);
}

bool BgWorkerGroup::RequestRow(int32_t table_id, int32_t row_id,
                               int32_t clock) {
  int32_t bg_idx = GlobalContext::GetPartitionCommChannelIndex(row_id);
  return bg_worker_vec_[bg_idx]->RequestRow(table_id, row_id, clock);
}

void BgWorkerGroup::ClockAllTables() {
  for (const auto &worker : bg_worker_vec_) {
    worker->ClockAllTables();
  }
}

void BgWorkerGroup::SendOpLogsAllTables() {
  for (const auto &worker : bg_worker_vec_) {
    worker->SendOpLogsAllTables();
  }
}

void BgWorkerGroup::TurnOnEarlyComm() {
  for (const auto &worker : bg_worker_vec_) {
    worker->TurnOnEarlyComm();
  }
}

void BgWorkerGroup::TurnOffEarlyComm() {
  for (const auto &worker : bg_worker_vec_) {
    worker->TurnOffEarlyComm();
  }
}

}
