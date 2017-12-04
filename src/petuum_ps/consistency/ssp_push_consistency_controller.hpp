// author: Jinliang
#pragma once

#include <petuum_ps/consistency/ssp_consistency_controller.hpp>
#include <boost/thread/tss.hpp>
#include <utility>
#include <vector>
#include <cstdint>
#include <atomic>

namespace petuum {

class SSPPushConsistencyController : public SSPConsistencyController {
public:
  SSPPushConsistencyController(
      const TableInfo& info,
      int32_t table_id,
      AbstractProcessStorage& process_storage,
      AbstractOpLog& oplog,
      const AbstractRow* sample_row,
      boost::thread_specific_ptr<ThreadTable> &thread_cache,
      TableOpLogIndex &oplog_index,
      int32_t row_oplog_type);

  void GetAsyncForced(RowId row_id);
  void GetAsync(RowId row_id);
  void WaitPendingAsnycGet();

  // Check freshness; make request and block if too stale or row_id not found
  // in storage.
  ClientRow *Get(RowId row_id, RowAccessor* row_accessor);

  void ThreadGet(RowId row_id, ThreadRowAccessor* row_accessor);

private:
  static const size_t kMaxPendingAsyncGetCnt = 256;
  boost::thread_specific_ptr<size_t> pending_async_get_cnt_;
};

}  // namespace petuum
