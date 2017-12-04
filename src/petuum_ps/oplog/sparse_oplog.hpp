// Author: jinliang

#pragma once

#include <petuum_ps/oplog/abstract_oplog.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include <petuum_ps_common/util/striped_lock.hpp>

namespace petuum {
class SparseOpLog : public AbstractOpLog {
public:
  SparseOpLog(int32_t capacity, const AbstractRow *sample_row,
              size_t dense_row_oplog_capacity,
              int32_t row_oplog_type,
              bool version_maintain);
  ~SparseOpLog();

  void RegisterThread() { }
  void DeregisterThread() { }
  void FlushOpLog() { }

  // exclusive access
  int32_t Inc(RowId row_id, int32_t column_id, const void *delta);
  int32_t BatchInc(RowId row_id, const int32_t *column_ids,
                   const void *deltas, int32_t num_updates);

  int32_t DenseBatchInc(RowId row_id, const void *updates,
                        int32_t index_st, int32_t num_updates);

  // Guaranteed exclusive accesses to the same row id.
  bool FindOpLog(RowId row_id, OpLogAccessor *oplog_accessor);
  // return true if a new row oplog is created
  bool FindInsertOpLog(RowId row_id, OpLogAccessor *oplog_accessor);
  // oplog_accessor aquires the lock on the row whether or not the
  // row oplog exists.
  bool FindAndLock(RowId row_id, OpLogAccessor *oplog_accessor);

  // Not mutual exclusive but is less expensive than FIndOpLog above as it does
  // not use any lock.
  AbstractRowOpLog *FindOpLog(RowId row_id);
  AbstractRowOpLog *FindInsertOpLog(RowId row_id);

  // Mutual exclusive accesses
  bool GetEraseOpLog(RowId row_id, AbstractRowOpLog **row_oplog_ptr);
  bool GetEraseOpLogIf(RowId row_id, GetOpLogTestFunc test,
                       void *test_args, AbstractRowOpLog **row_oplog_ptr);

  bool GetInvalidateOpLogMeta(RowId row_id, RowOpLogMeta *row_oplog_meta);

  AbstractAppendOnlyBuffer *GetAppendOnlyBuffer(int32_t comm_channel_idx);
  void PutBackBuffer(int32_t comm_channel_idx, AbstractAppendOnlyBuffer* buff);

private:
  const size_t update_size_;
  StripedLock<RowId> locks_;
  cuckoohash_map<RowId, AbstractRowOpLog*> oplog_map_;
  const AbstractRow *sample_row_;
  const size_t dense_row_oplog_capacity_;
  CreateRowOpLog::CreateRowOpLogFunc CreateRowOpLog_;
};

}   // namespace petuum
