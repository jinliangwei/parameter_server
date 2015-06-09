#pragma once

#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/include/configs.hpp>
#include <petuum/ps_common/storage/storage_types.hpp>
#include <petuum/ps_common/util/vector_clock_mt.hpp>
#include <petuum/ps_common/client/abstract_client_table.hpp>
#include <petuum/ps_common/index/abstract_index.hpp>
#include <petuum/ps/client/parameter_cache_clock.hpp>

namespace petuum {

class ClientTable : public AbstractClientTable {
public:
  // Instantiate AbstractRow, TableOpLog, and ClientStorage using config.
  ClientTable(int32_t table_id, const ClientTableConfig& config);

  ~ClientTable();

  void Get(int32_t row_id, int32_t col_id, AbstractRow *to, int32_t data_clock);

  void CopyToVector(int32_t row_id, void *vec, int32_t data_clock);

  void Inc(int32_t row_id, int32_t column_id, const void *update);

  void BatchInc(int32_t row_id, const int32_t* column_ids, const void* updates,
                size_t num_updates);

  void DenseBatchInc(int32_t row_id, const void *updates, int32_t index_st,
                     size_t num_updates);

  void Clock();

  const ClientTableConfig & get_client_table_config() const {
    return client_table_config_;
  }

  ParameterCache &get_parameter_cache() {
    return parameter_cache_;
  }

  UpdateBuffer &get_update_buffer() {
    return update_buffer_;
  }

  ParameterCacheClock &get_parameter_cache_clock() {
    return parameter_cache_clock_;
  }

  const AbstractRow* get_sample_row () const {
    return sample_row_;
  }

private:
  const int32_t table_id_;
  const AbstractRow* const sample_row_;
  const ClientTableConfig client_table_config_;
  ParameterCache parameter_cache_;
  UpdateBuffer update_buffer_;
  LockEngine lock_engine_;
  AbstractIndex *index_;
  ParameterCacheClock parameter_cache_clock_;
  //AbstractConsistencyController *consistency_controller_;
};

}  // namespace petuum
