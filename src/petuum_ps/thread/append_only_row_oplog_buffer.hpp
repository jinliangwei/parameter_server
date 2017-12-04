#pragma once

#include <petuum_ps/thread/row_oplog_recycle.hpp>
#include <petuum_ps_common/include/configs.hpp>
#include <petuum_ps_common/include/abstract_row.hpp>

#include <glog/logging.h>
#include <boost/noncopyable.hpp>
#include <unordered_map>

namespace petuum {

class AppendOnlyRowOpLogBuffer : boost::noncopyable {
public:
  AppendOnlyRowOpLogBuffer(
      int32_t row_oplog_type,
      const AbstractRow *sample_row,
      size_t update_size,
      size_t dense_row_oplog_capacity,
      AppendOnlyOpLogType append_only_oplog_type):
      update_size_(update_size),
      sample_row_(sample_row),
      row_oplog_generator_(row_oplog_type, sample_row, update_size,
                           dense_row_oplog_capacity) {
    if (append_only_oplog_type == DenseBatchInc) {
      BatchInc_ = &AppendOnlyRowOpLogBuffer::BatchIncDense;
    } else {
      BatchInc_ = &AppendOnlyRowOpLogBuffer::BatchIncGeneral;
    }
  }

  ~AppendOnlyRowOpLogBuffer() {
    CHECK_EQ(tmp_row_oplog_map_.size(), 0);
    for (auto &row_oplog : row_oplog_map_) {
      delete row_oplog.second;
      row_oplog.second = 0;
    }
  }

  void BatchIncTmp(RowId row_id, const int32_t *col_ids,
                   const void *updates, int32_t num_updates);

  void BatchInc(RowId row_id, const int32_t *col_ids,
                const void *updates, int32_t num_updates);

  void MergeTmpOpLog();

  AbstractRowOpLog *InitReadTmpOpLog(RowId *row_id);
  AbstractRowOpLog *NextReadTmpOpLog(RowId *row_id);

  AbstractRowOpLog *InitReadRmOpLog(RowId *row_id);
  AbstractRowOpLog *NextReadRmOpLog(RowId *row_id);

  AbstractRowOpLog *InitReadOpLog(RowId *row_id);
  AbstractRowOpLog *NextReadOpLog(RowId *row_id);

  AbstractRowOpLog *GetRowOpLog(RowId row_id) const;

private:
  void BatchIncGeneral(
      AbstractRowOpLog *row_oplog,
      const int32_t *col_ids, const void *updates, int32_t num_updates);

  void BatchIncDense(
      AbstractRowOpLog *row_oplog,
      const int32_t *col_ids, const void *updates, int32_t num_updates);

  typedef void (AppendOnlyRowOpLogBuffer::*BatchIncFunc)(
      AbstractRowOpLog *row_oplog,
      const int32_t *col_ids, const void *updates, int32_t num_updates);

  BatchIncFunc BatchInc_;

  const size_t update_size_;
  const AbstractRow *sample_row_;
  RowOpLogRecycle row_oplog_generator_;
  std::unordered_map<RowId, AbstractRowOpLog*> row_oplog_map_;
  std::unordered_map<RowId, AbstractRowOpLog*> tmp_row_oplog_map_;
  std::unordered_map<RowId, AbstractRowOpLog*>::iterator map_iter_;
};

}
