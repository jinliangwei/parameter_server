#include <petuum_ps/server/ssp_aggr_server_thread.hpp>
#include <petuum_ps/thread/trans_time_estimate.hpp>
#include <petuum_ps/thread/context.hpp>
#include <petuum_ps_common/util/stats.hpp>
#include <algorithm>

namespace petuum {
void SSPAggrServerThread::SetWaitMsg() {
  WaitMsg_ = WaitMsgTimeOut;
}

long SSPAggrServerThread::ServerIdleWork() {
  if (row_send_milli_sec_ > 1) {
    double send_elapsed_milli = msg_send_timer_.elapsed() * kOneThousand;
    //LOG(INFO) << "send_elapsed_milli = " << send_elapsed_milli
    //      << " row_send_milli_sec_ = " << row_send_milli_sec_;
    if (row_send_milli_sec_ > send_elapsed_milli + 1)
      return (row_send_milli_sec_ - send_elapsed_milli);
  }

  if (!msg_tracker_.CheckSendAll()) {
    STATS_SERVER_ACCUM_WAITS_ON_ACK_IDLE();
    return GlobalContext::get_server_idle_milli();
  }

  STATS_SERVER_IDLE_INVOKE_INC_ONE();

  STATS_SERVER_ACCUM_IDLE_SEND_BEGIN();
  size_t sent_bytes
      = server_obj_.CreateSendServerPushRowMsgsPartial(SendServerPushRowMsg);

  STATS_SERVER_ACCUM_IDLE_SEND_END();

  if (sent_bytes > 0) {
      STATS_SERVER_IDLE_SEND_INC_ONE();
      STATS_SERVER_ACCUM_IDLE_ROW_SENT_BYTES(sent_bytes);

      row_send_milli_sec_ = TransTimeEstimate::EstimateTransMillisec(sent_bytes);

      //LOG(INFO) << "ServerIdle send bytes = " << sent_bytes
      //        << " bw = " << GlobalContext::get_bandwidth_mbps()
      //        << " send milli sec = " << row_send_milli_sec_
      //        << " server_id = " << my_id_;

      msg_send_timer_.restart();

      return row_send_milli_sec_;
  }
  //LOG(INFO) << "Server nothing to send";

  return GlobalContext::get_server_idle_milli();
}

long SSPAggrServerThread::ResetServerIdleMilli() {
  return (this->*ResetServerIdleMilli_)();
}

long SSPAggrServerThread::ResetServerIdleMilliNoEarlyComm() {
  return -1;
}

long SSPAggrServerThread::ResetServerIdleMilliEarlyComm() {
  return GlobalContext::get_server_idle_milli();
}

void SSPAggrServerThread::ServerPushRow() {
  if (!msg_tracker_.CheckSendAll()) {
    STATS_SERVER_ACCUM_WAITS_ON_ACK_CLOCK();
    pending_clock_push_row_ = true;
    return;
  }

  if ((suppression_level_ > 0)
      && (last_clock_sent_ >=
          (server_obj_.GetMinClock() - suppression_level_))) {
    LOG(INFO) << "PushRow is suppressed, suppression level = "
              << suppression_level_;
    return;
  }

  STATS_SERVER_ACCUM_PUSH_ROW_BEGIN();
  size_t sent_bytes
      = server_obj_.CreateSendServerPushRowMsgs(SendServerPushRowMsg);
  STATS_SERVER_ACCUM_PUSH_ROW_END();

  double left_over_send_milli_sec = 0;

  if (row_send_milli_sec_ > 1) {
    double send_elapsed_milli = msg_send_timer_.elapsed() * kOneThousand;
    left_over_send_milli_sec = std::max<double>(0, row_send_milli_sec_ - send_elapsed_milli);
  }

  row_send_milli_sec_ = TransTimeEstimate::EstimateTransMillisec(sent_bytes)
                        + left_over_send_milli_sec;
  msg_send_timer_.restart();

  if ((suppression_level_ > 0)) {
    double comm_sec = row_send_milli_sec_*2;
    double window_sec = min_table_staleness_ * clock_tick_sec_ / (2.0);
    double comm_clock_ticks = comm_sec / clock_tick_sec_;
    if (comm_clock_ticks < 1) comm_clock_ticks = 1;
    int32_t comm_clock_ticks_int = (int32_t) comm_clock_ticks;

    if (comm_clock_ticks_int <= 1 || comm_sec < window_sec) {
      suppression_level_ = min_table_staleness_ - comm_clock_ticks_int;
    } else {
      suppression_level_ = comm_clock_ticks_int;
      if (suppression_level_ > min_table_staleness_)
        suppression_level_ = min_table_staleness_;
    }
    LOG(INFO) << "suppression level changed to" << suppression_level_;
  }

  //LOG(INFO) << "Server clock sent_size = " << sent_bytes
  //        << " row_send_milli_sec = " << row_send_milli_sec_
  //        << " left_over_send_milli_sec = " << left_over_send_milli_sec;
}

void SSPAggrServerThread::HandleEarlyCommOn() {
  if (!early_comm_on_) {
    ResetServerIdleMilli_ = &SSPAggrServerThread::ResetServerIdleMilliEarlyComm;
    early_comm_on_ = true;
    num_early_comm_off_msgs_ = 0;
  }
}

void SSPAggrServerThread::HandleEarlyCommOff() {
  num_early_comm_off_msgs_++;
  if (num_early_comm_off_msgs_ == bg_worker_ids_.size()) {
    ResetServerIdleMilli_ = &SSPAggrServerThread::ResetServerIdleMilliNoEarlyComm;
    early_comm_on_ = false;
    num_early_comm_off_msgs_ = 0;
  }
}

void SSPAggrServerThread::PrepareBeforeInfiniteLoop() {
  clock_timer_.restart();
  if (GlobalContext::get_suppression_on()) {
    if (min_table_staleness_ <= 2) {
      suppression_level_ = 0;
    } else {
      suppression_level_ = 2;
    }
  } else {
    suppression_level_ = 0;
  }

  LOG(INFO) << "supression level initialized to " << suppression_level_;
}

void SSPAggrServerThread::ClockNotice() {
  clock_tick_sec_ = clock_timer_.elapsed();
  clock_timer_.restart();
}

}
