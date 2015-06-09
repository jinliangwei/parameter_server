#pragma once

#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/include/configs.hpp>
#include <boost/noncopyable.hpp>

namespace petuum {

class AbstractClientTable : boost::noncopyable {
public:
  virtual ~AbstractClientTable() { }

  virtual void Get(int32_t row_id, int32_t col_id, AbstractRow *to,
                   int32_t data_clock) = 0;

  virtual void CopyToVector(int32_t row_id, void *vec,
                            int32_t data_clock) = 0;

  virtual void Inc(int32_t row_id, int32_t column_id, const void *update) = 0;

  virtual void BatchInc(int32_t row_id, const int32_t* column_ids,
                        const void* updates, size_t num_updates) = 0;

  virtual void DenseBatchInc(int32_t row_id, const void *updates,
                             int32_t index_st, size_t num_updates) = 0;

  virtual void Clock() = 0;

  virtual const ClientTableConfig &get_client_table_config() const = 0;
};

}  // namespace petuum
