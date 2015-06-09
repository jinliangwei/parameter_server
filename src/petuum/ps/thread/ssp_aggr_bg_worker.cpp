#include <petuum/ps/thread/ssp_aggr_bg_worker.hpp>
#include <petuum/ps/thread/trans_time_estimate.hpp>
#include <petuum/ps/thread/global_context.hpp>
#include <petuum/ps_common/util/stats.hpp>
#include <algorithm>

namespace petuum {

void SSPAggrBgWorker::HandleEarlyCommOn() {
  if (GlobalContext::get_suppression_on()) {
    if (min_table_staleness_ <= 2) {
      suppression_level_ = 0;
    } else {
      suppression_level_ = 0;
      suppression_on_ = true;
    }
  } else {
    suppression_on_ = false;
    suppression_level_ = 0;
  }

  ResetBgIdleMilli_ = &SSPAggrBgWorker::ResetBgIdleMilliEarlyComm;
  EarlyCommOnMsg msg;
  for (const auto &server_id : server_ids_) {
    size_t sent_size = (comm_bus_->*(comm_bus_->SendAny_))(
        server_id, msg.get_mem(), msg.get_size());
    CHECK_EQ(sent_size, msg.get_size());
  }
  early_comm_on_ = true;
}

void SSPAggrBgWorker::HandleEarlyCommOff() {
  //ResetBgIdleMilli_ = &SSPAggrBgWorker::ResetBgIdleMilliNoEarlyComm;

  EarlyCommOffMsg msg;
  for (const auto &server_id : server_ids_) {
    size_t sent_size = (comm_bus_->*(comm_bus_->SendAny_))(
        server_id, msg.get_mem(), msg.get_size());
    CHECK_EQ(sent_size, msg.get_size());
  }
  early_comm_on_ = false;
}

void SSPAggrBgWorker::HandleAdjustSuppressionLevel() {
  suppression_level_ = suppression_level_min_;
  LOG(INFO) << "Adjust suppression level to "
            << suppression_level_min_
            << " id = " << my_id_;
}

}
