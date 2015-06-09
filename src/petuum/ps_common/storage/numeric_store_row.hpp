#pragma once

#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/ps_common/storage/ns_sum_imp_calc.hpp>
#include <petuum/ps_common/util/utils.hpp>

#include <mutex>

namespace petuum {

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc = NSSumImpCalc >
class NumericStoreRow : public AbstractRow {
public:
  NumericStoreRow() { }
  virtual ~NumericStoreRow() { }

  void Init(size_t capacity);

  virtual void CopyToVector(void *vec) const;

  virtual size_t get_update_size() const;

  virtual size_t SerializedSize() const;

  virtual size_t Serialize(void *bytes) const;

  virtual void Deserialize(const void* data, size_t num_bytes);

  virtual void ResetRowData(const void *data, size_t num_bytes);

  virtual void ApplyInc(int32_t column_id, const void *update);

  virtual double ApplyIncGetReImportance(int32_t column_id, const void *update);

  virtual void ApplyBatchInc(const int32_t *column_ids,
    const void* update_batch, int32_t num_updates);

  virtual double ApplyBatchIncGetReImportance(
      const int32_t *column_ids, const void* update_batch, int32_t num_updates);

  virtual void ApplyDenseBatchInc(
      const void* update_batch, int32_t index_st, int32_t num_updates);

  virtual double ApplyDenseBatchIncGetReImportance(
      const void* update_batch, int32_t index_st, int32_t num_updates);

protected:
  StoreType<V> store_;
  ImpCalc<V> imp_cal_;
};

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::Init(size_t capacity) {
  store_.Init(capacity);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::CopyToVector(void *vec) const {
  store_.CopyToVector(vec);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
size_t NumericStoreRow<StoreType, V, ImpCalc>::get_update_size() const {
  return sizeof(V);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
size_t NumericStoreRow<StoreType, V, ImpCalc>::SerializedSize() const {
  return store_.SerializedSize();
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
size_t NumericStoreRow<StoreType, V, ImpCalc>::Serialize(void *bytes) const {
  return store_.Serialize(bytes);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::Deserialize(const void* data,
                                                         size_t num_bytes) {
  store_.Deserialize(data, num_bytes);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::ResetRowData(const void *data,
                                                          size_t num_bytes) {
  store_.ResetData(data, num_bytes);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::ApplyInc(int32_t column_id,
                                                      const void *update) {
  store_.Inc(column_id, *(reinterpret_cast<const V*>(update)));
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::ApplyBatchInc(
    const int32_t *column_ids,
    const void* update_batch, int32_t num_updates) {
  const V *update_array = reinterpret_cast<const V*>(update_batch);
  for (int i = 0; i < num_updates; ++i) {
    store_.Inc(column_ids[i], update_array[i]);
  }
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
void NumericStoreRow<StoreType, V, ImpCalc>::ApplyDenseBatchInc(
    const void* update_batch, int32_t index_st, int32_t num_updates) {
  V *val_array = store_.GetPtr(index_st);
  const V *update_array = reinterpret_cast<const V*>(update_batch);

  for (int32_t i = 0; i < num_updates; ++i) {
    val_array[i] += update_array[i];
  }
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
double NumericStoreRow<StoreType, V, ImpCalc>::ApplyIncGetReImportance(
    int32_t column_id, const void *update) {
  return imp_cal_.ApplyIncGetReImportance(&store_, column_id, update);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
double NumericStoreRow<StoreType, V, ImpCalc>::ApplyBatchIncGetReImportance(
    const int32_t *column_ids,
    const void* update_batch, int32_t num_updates) {
  return imp_cal_.ApplyBatchIncGetReImportance(&store_, column_ids, update_batch,
                                             num_updates);
}

template<template<typename> class StoreType, typename V,
         template<typename> class ImpCalc>
double NumericStoreRow<StoreType, V, ImpCalc>::ApplyDenseBatchIncGetReImportance(
    const void* update_batch, int32_t index_st, int32_t num_updates) {
  return imp_cal_.ApplyDenseBatchIncGetReImportance(
      &store_, update_batch, index_st, num_updates);
}

}
