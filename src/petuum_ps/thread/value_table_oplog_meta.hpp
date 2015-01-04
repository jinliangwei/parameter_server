#pragma once

#include <petuum_ps/thread/random_table_oplog_meta.hpp>

#include <vector>
#include <unordered_map>
#include <random>

namespace petuum {

class ValueTableOpLogMeta : public RandomTableOpLogMeta {
public:
  ValueTableOpLogMeta(const AbstractRow *sample_row);
  virtual ~ValueTableOpLogMeta();

  virtual void InsertMergeRowOpLogMeta(int32_t row_id,
                                       const RowOpLogMeta& row_oplog_meta);
  virtual void Prepare(size_t num_rows_to_send);
  virtual int32_t GetAndClearNextInOrder();

private:
  std::vector<std::pair<int32_t, RowOpLogMeta> > sorted_vec_;
  std::vector<std::pair<int32_t, RowOpLogMeta> >::iterator vec_iter_;
};

}
