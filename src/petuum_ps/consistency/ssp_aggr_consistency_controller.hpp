#pragma once

#include <petuum_ps/consistency/ssp_push_consistency_controller.hpp>

namespace petuum {

class SSPAggrConsistencyController : public SSPPushConsistencyController {
public:
  SSPAggrConsistencyController(
      const TableInfo& info,
      int32_t table_id,
      AbstractProcessStorage& process_storage,
      AbstractOpLog& oplog,
      const AbstractRow* sample_row,
      boost::thread_specific_ptr<ThreadTable> &thread_cache,
      TableOpLogIndex &oplog_index,
      int32_t row_oplog_type);

    // Return immediately.
  virtual void Inc(RowId row_id, int32_t column_id, const void* delta);

  virtual void BatchInc(RowId row_id, const int32_t* column_ids,
                        const void* updates, int32_t num_updates);

  virtual void DenseBatchInc(RowId row_id, const void *updates,
                             int32_t index_st, int32_t num_updates);

  virtual void ThreadInc(RowId row_id, int32_t column_id, const void* delta);

  // Increment column_ids.size() entries of a row. deltas points to an array.
  virtual void ThreadBatchInc(RowId row_id, const int32_t* column_ids,
			      const void* updates, int32_t num_updates);
  virtual void ThreadDenseBatchInc(
      RowId row_id, const void *updates, int32_t index_st,
      int32_t num_updates);
protected:
  void CheckAndFlushThreadCache(size_t thread_update_count);
};

}  // namespace petuum
