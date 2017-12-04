#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <boost/noncopyable.hpp>

#include <petuum_ps_common/include/abstract_row.hpp>
#include <petuum_ps_common/storage/abstract_process_storage.hpp>
#include <petuum_ps_common/include/configs.hpp>

#include <petuum_ps/oplog/oplog_index.hpp>
#include <petuum_ps/oplog/abstract_oplog.hpp>
#include <petuum_ps/oplog/create_row_oplog.hpp>

namespace petuum {

class ThreadTable : boost::noncopyable {
public:
  explicit ThreadTable(const AbstractRow *sample_row, int32_t row_oplog_type,
                       size_t dense_row_oplog_capacity);
  ~ThreadTable();
  void IndexUpdate(RowId row_id);
  void FlushOpLogIndex(TableOpLogIndex &oplog_index);

  AbstractRow *GetRow(RowId row_id);
  void InsertRow(RowId row_id, const AbstractRow *to_insert);
  void Inc(RowId row_id, int32_t column_id, const void *delta);
  void BatchInc(RowId row_id, const int32_t *column_ids,
                const void *deltas, int32_t num_updates);

  void DenseBatchInc(RowId row_id, const void *updates, int32_t index_st,
                     int32_t num_updates);

  void FlushCache(AbstractProcessStorage &process_storage, AbstractOpLog &table_oplog,
		  const AbstractRow *sample_row);
  void FlushCacheOpLog(AbstractProcessStorage &process_storage, AbstractOpLog &table_oplog,
                       const AbstractRow *sample_row);

  size_t IndexUpdateAndGetCount(RowId row_id, size_t num_updates = 1);
  void ResetUpdateCount();

  void AddToUpdateCount(size_t num_updates);

  size_t get_update_count() {
    return update_count_;
  }

private:
  std::vector<OpLogRowIdSet> oplog_index_;
  std::unordered_map<RowId, AbstractRow* > row_storage_;
  std::unordered_map<RowId, AbstractRowOpLog* > oplog_map_;
  const AbstractRow *sample_row_;

  size_t update_count_;

  size_t dense_row_oplog_capacity_;

  typedef void (*UpdateOpLogClockFunc)(AbstractRowOpLog *row_oplog);

  static void UpdateOpLogClockSSPAggr(AbstractRowOpLog *row_oplog);
  static void UpdateOpLogClockNoOp(AbstractRowOpLog *row_oplog) { }
  UpdateOpLogClockFunc UpdateOpLogClock_;

  CreateRowOpLog::CreateRowOpLogFunc CreateRowOpLog_;

  void ApplyThreadOpLogSSP(
      OpLogAccessor *oplog_accessor, ClientRow *client_row,
      AbstractRowOpLog *row_oplog, RowId row_id);

  void ApplyThreadOpLogGetImportance(
      OpLogAccessor *oplog_accessor, ClientRow *client_row,
      AbstractRowOpLog *row_oplog, RowId row_id);

  typedef void (ThreadTable::*ApplyThreadOpLogFunc)(
      OpLogAccessor *oplog_accessor, ClientRow *client_row,
      AbstractRowOpLog *row_oplog, RowId row_id);

  ThreadTable::ApplyThreadOpLogFunc ApplyThreadOpLog_;
};

}
