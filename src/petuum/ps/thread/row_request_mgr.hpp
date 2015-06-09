// author: jinliang

#pragma once

#include <map>
#include <vector>
#include <list>
#include <utility>
#include <boost/noncopyable.hpp>
#include <glog/logging.h>

namespace petuum {

struct RowRequestInfo {
public:
  int32_t requester_id;
  int32_t clock;
  bool sent;

  RowRequestInfo() :
    requester_id(0),
    clock(0),
    sent(false) { }

  RowRequestInfo(const RowRequestInfo & other):
      requester_id(other.requester_id),
      clock(other.clock),
      sent(other.sent) { }

  RowRequestInfo & operator=(const RowRequestInfo & other) {
    requester_id = other.requester_id;
    clock = other.clock;
    sent = other.sent;
    return *this;
  }
};

// Keep track of row requests that are sent to server or that could
// be potentially sent to server (row does not exist in process cache
// but a request for that row has been sent out).

// When a requested row is not found in process cache, the bg worker
// checks with RowRequestMgr to see if it has sent out a request for
// that row. If not, send the current request; otherwise, wait for
// server response.

// When the bg worker, receives a reply for a row, it inserts that
// row into process cache and checks with RowRequestMgr to see how many
// row requests that reply may satisfy.

// After a row request is sent to server and before the bg worker receives the
// reply, the bg worker might have sent out multiple sets of updates to server.
// Since the server may buffer the row and reply later, those updates might or
// might not be applied to the row on server.

class RowRequestMgr : boost::noncopyable {
public:
  RowRequestMgr() {}

  ~RowRequestMgr() { }

  // return true unless there's a previous request with lower or same clock
  // number
  bool AddRowRequest(RowRequestInfo &request, int32_t table_id, int32_t row_id);

  // Get a list of app thread ids that can be satisfied with this reply.
  // Corresponding row requests are removed upon returning.
  // If all row requests prior to some version are removed, those OpLogs are
  // removed as well.
  void InformReply(
      int32_t table_id, int32_t row_id, int32_t clock,
      std::vector<int32_t> *requester_ids);

private:
  // map <table_id, row_id> to a list of requests
  // The list is in increasing order of clock.
  std::map<std::pair<int32_t, int32_t>,
           std::list<RowRequestInfo> > pending_row_requests_;

};

}  // namespace petuum
