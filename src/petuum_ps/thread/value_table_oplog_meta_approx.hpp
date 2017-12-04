#pragma once

#include <petuum_ps/thread/random_table_oplog_meta.hpp>

#include <vector>
#include <unordered_map>
#include <random>

namespace petuum {

class ValueTableOpLogMetaApprox : public RandomTableOpLogMeta {
public:
  ValueTableOpLogMetaApprox(const AbstractRow *sample_row);
  virtual ~ValueTableOpLogMetaApprox();

  virtual void InsertMergeRowOpLogMeta(RowId row_id,
                                       const RowOpLogMeta& row_oplog_meta);
  virtual void Prepare(size_t num_rows_to_send);
  virtual RowId GetAndClearNextInOrder();

private:
  std::vector<std::pair<RowId, RowOpLogMeta> > sorted_vec_;
  std::vector<std::pair<RowId, RowOpLogMeta> >::iterator vec_iter_;

  std::mt19937 generator_; // max 4.2 billion
  std::uniform_real_distribution<> uniform_dist_;
};

}
