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
    //        << " row_send_milli_sec_ = " << row_send_milli_sec_;
    if (row_send_milli_sec_ > send_elapsed_milli + 1)
      return (row_send_milli_sec_ - send_elapsed_milli);
  }

  if (server_obj_.AccumedOpLogSinceLastPush()) {
    size_t sent_bytes
        = server_obj_.CreateSendServerPushRowMsgsPartial(SendServerPushRowMsg);
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
  return GlobalContext::get_server_idle_milli();
}

void SSPAggrServerThread::ServerPushRow(bool clock_changed) {
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

  //LOG(INFO) << "Server clock sent_size = " << sent_bytes
  //        << " row_send_milli_sec = " << row_send_milli_sec_
  //        << " left_over_send_milli_sec = " << left_over_send_milli_sec;
}
}
