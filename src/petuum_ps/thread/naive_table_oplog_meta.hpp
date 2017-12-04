#pragma once

#include <petuum_ps/oplog/row_oplog_meta.hpp>
#include <petuum_ps_common/include/abstract_row.hpp>
#include <petuum_ps_common/include/configs.hpp>

#include <boost/unordered_map.hpp>
#include <list>
#include <stdint.h>
#include <petuum_ps/thread/context.hpp>
#include <boost/noncopyable.hpp>

#include <petuum_ps/thread/abstract_table_oplog_meta.hpp>

namespace petuum {

class NaiveTableOpLogMeta : public AbstractTableOpLogMeta {
public:
  NaiveTableOpLogMeta(const AbstractRow *sample_row);
  ~NaiveTableOpLogMeta();

  typedef bool (*CompRowOpLogMetaFunc)(
      const std::pair<RowId, RowOpLogMeta*> &oplog1,
      const std::pair<RowId, RowOpLogMeta*> &oplog2);

  typedef void (*ReassignImportanceFunc)(
      std::list<std::pair<RowId, RowOpLogMeta*> > *oplog_list);

  typedef void (*MergeRowOpLogMetaFunc)(RowOpLogMeta* row_oplog_meta,
                                        const RowOpLogMeta& to_merge);

  void InsertMergeRowOpLogMeta(RowId row_id,
                               const RowOpLogMeta& row_oplog_meta);

  size_t GetCleanNumNewOpLogMeta();

  void Prepare(size_t num_rows_to_send);
  RowId GetAndClearNextInOrder();

  RowId InitGetUptoClock(int32_t clock);
  RowId GetAndClearNextUptoClock();

  size_t GetNumRowOpLogs() const {
    return oplog_map_.size();
  }

private:
  static bool CompRowOpLogMetaClock(
      const std::pair<RowId, RowOpLogMeta*> &oplog1,
      const std::pair<RowId, RowOpLogMeta*> &oplog2);

  static bool CompRowOpLogMetaImportance(
      const std::pair<RowId, RowOpLogMeta*> &oplog1,
      const std::pair<RowId, RowOpLogMeta*> &oplog2);

  static bool CompRowOpLogMetaRelativeFIFONReMag(
      const std::pair<RowId, RowOpLogMeta*> &oplog1,
      const std::pair<RowId, RowOpLogMeta*> &oplog2);

  static void ReassignImportanceRandom(
      std::list<std::pair<RowId, RowOpLogMeta*> > *oplog_list);

  static void ReassignImportanceNoOp(
      std::list<std::pair<RowId, RowOpLogMeta*> > *oplog_list);

  static void MergeRowOpLogMetaAccum(RowOpLogMeta* row_oplog_meta,
                                     const RowOpLogMeta& to_merge);

  // return importance1
  static void MergeRowOpLogMetaNoOp(RowOpLogMeta* row_oplog_meta,
                                    const RowOpLogMeta& to_merge);

  boost::unordered_map<RowId, RowOpLogMeta*> oplog_map_;
  std::list<std::pair<RowId, RowOpLogMeta*> > oplog_list_;

  const AbstractRow *sample_row_;
  MergeRowOpLogMetaFunc MergeRowOpLogMeta_;
  CompRowOpLogMetaFunc CompRowOpLogMeta_;
  ReassignImportanceFunc ReassignImportance_;

  std::list<std::pair<RowId, RowOpLogMeta*> >::iterator list_iter_;

  // After GetAndClearNextUptoClock(), all row oplogs will be above this clock
  // (exclusive)
  // Assume oplogs below this clock will not be added after GetAndClear...
  int32_t clock_to_clear_;

  size_t num_new_oplog_metas_;
};

}
