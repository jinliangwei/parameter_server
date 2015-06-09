#pragma once

#include <petuum/ps_common/storage/abstract_store.hpp>

namespace petuum {

template<typename V>
class NSAbstractImpCalc {
public:
  NSAbstractImpCalc() { }
  virtual ~NSAbstractImpCalc() { }

  virtual double ApplyIncGetReImportance(
      AbstractStore<V> *store,
      int32_t column_id, const void *update) const = 0;

  virtual double ApplyBatchIncGetReImportance(
      AbstractStore<V> *store, const int32_t *column_ids,
      const void* update_batch, int32_t num_updates) const = 0;

  virtual double ApplyDenseBatchIncGetReImportance(
      AbstractStore<V> *store,
      const void* update_batch, int32_t index_st, int32_t num_updates) const = 0;

  // Get importance of this update as if it is allied on to the given value.
  virtual double GetReImportance(int32_t column_id, const void *update,
                               const void *value) const = 0;

  virtual double GetImportance(int32_t column_id,
                               const void *update) const = 0;

  virtual double GetAccumImportance(
    const int32_t *column_ids,
    const void *update_batch, int32_t num_updates) const = 0;

  virtual double GetDenseAccumReImportance(
      const AbstractStore<V> *store,
      const void *update_batch, int32_t index_st,
      int32_t num_updates) const = 0;
};

}
