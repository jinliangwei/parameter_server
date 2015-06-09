#include <petuum/ps/thread/row_request_mgr.hpp>
#include <utility>
#include <glog/logging.h>
#include <list>
#include <vector>

namespace petuum {

bool RowRequestMgr::AddRowRequest(
    RowRequestInfo &request,
    int32_t table_id, int32_t row_id) {
  request.sent = true;

  {
    std::pair<int32_t, int32_t> request_key(table_id, row_id);
    if (pending_row_requests_.count(request_key) == 0) {
      pending_row_requests_.insert(
          std::make_pair(
              request_key,
              std::list<RowRequestInfo>()));
    }
    std::list<RowRequestInfo> &request_list
      = pending_row_requests_[request_key];
    bool request_added = false;
    // Requests are sorted in increasing order of clock number.
    // When a request is to be inserted, start from the end as the requst's
    // clock is more likely to be larger.
    for (auto iter = request_list.end(); iter != request_list.begin(); iter--) {
      auto iter_prev = std::prev(iter);
      int32_t clock = request.clock;
      if (clock >= iter_prev->clock) {
        // insert before iter
	request.sent = false;
        request_list.insert(iter, request);
        request_added = true;
        break;
      }
    }
    if (!request_added)
      request_list.push_front(request);
  }

  return request.sent;
}

void RowRequestMgr::InformReply(int32_t table_id, int32_t row_id,
  int32_t clock, std::vector<int32_t> *requester_ids) {
  (*requester_ids).clear();
  std::pair<int32_t, int32_t> request_key(table_id, row_id);
  std::list<RowRequestInfo> &request_list = pending_row_requests_[request_key];

  while (!request_list.empty()) {
    RowRequestInfo &request = request_list.front();
    if (request.clock <= clock) {
      // remove the request
      requester_ids->push_back(request.requester_id);
      request_list.pop_front();
    } else {
      if (!request.sent) {
	request.sent = true;
      }
      break;
    }
  }
}

}  // namespace petuum
