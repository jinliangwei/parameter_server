#include <petuum_ps_common/thread/msg_tracker.hpp>
#include <glog/logging.h>

namespace petuum {

void MsgTracker::AddEntity(int32_t id) {
  info_map_.emplace(id, MsgTrackerInfo());
}

bool MsgTracker::CheckSendAll() {
  for (const auto & info_pair : info_map_) {
    if ((info_pair.second.max_sent_seq_ - info_pair.second.max_ack_seq_)
        >= max_num_penging_msgs_)
      return false;
  }
  return true;
}

bool MsgTracker::PendingAcks() {
  for (const auto & info_pair : info_map_) {
    if ((info_pair.second.max_sent_seq_ > info_pair.second.max_ack_seq_))
      return true;
  }
  return false;
}

uint64_t MsgTracker::IncGetSeq(int32_t id) {
  auto info_iter = info_map_.find(id);
  CHECK(info_iter != info_map_.end()) << id << " not found!";

  uint64_t seq = ++(info_iter->second.max_sent_seq_);
  CHECK_NE(seq, 0);
  return seq;
}

void MsgTracker::RecvAck(int32_t id, uint64_t ack_seq) {
  auto info_iter = info_map_.find(id);
  CHECK(info_iter != info_map_.end());

  CHECK_GE(ack_seq, info_iter->second.max_ack_seq_);
  CHECK_LE(ack_seq, info_iter->second.max_sent_seq_);

  info_iter->second.max_ack_seq_ = ack_seq;
}

}
