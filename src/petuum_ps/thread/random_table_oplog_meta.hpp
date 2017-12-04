#pragma once

#include <petuum_ps/thread/abstract_table_oplog_meta.hpp>

#include <vector>
#include <unordered_map>
#include <random>

namespace petuum {

class RandomTableOpLogMeta : public AbstractTableOpLogMeta {
public:
  RandomTableOpLogMeta(const AbstractRow *sample_row);
  virtual ~RandomTableOpLogMeta();

  virtual void InsertMergeRowOpLogMeta(RowId row_id,
                                       const RowOpLogMeta& row_oplog_meta);

  virtual size_t GetCleanNumNewOpLogMeta();

  virtual RowId GetAndClearNextInOrder();

  virtual RowId InitGetUptoClock(int32_t clock);
  virtual RowId GetAndClearNextUptoClock();

  virtual size_t GetNumRowOpLogs() const;

protected:
  const AbstractRow *sample_row_;
  std::unordered_map<RowId, RowOpLogMeta> oplog_meta_;

  std::unordered_map<RowId, RowOpLogMeta>::iterator meta_iter_;

  int32_t clock_to_clear_;
  size_t num_new_oplog_metas_;

private:
  std::mt19937 generator_; // max 4.2 billion
  std::uniform_int_distribution<int> uniform_dist_;
};

}
