#include <petuum/ps/thread/ssp_push_bg_worker.hpp>
#include <petuum/ps_common/util/stats.hpp>

namespace petuum {

void SSPPushBgWorker::PrepareBeforeInfiniteLoop() { }

long SSPPushBgWorker::ResetBgIdleMilli() {
  return 0;
}

long SSPPushBgWorker::BgIdleWork() {
  return 0;
}

void SSPPushBgWorker::HandleServerPushRow(int32_t sender_id,
                                          ServerPushRowMsg &server_push_row_msg) {
}

void SSPPushBgWorker::ApplyServerPushedRow(void *mem,
                                           size_t mem_size) {
}


}
