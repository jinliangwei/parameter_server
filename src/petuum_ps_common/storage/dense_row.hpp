#pragma once

#include <petuum_ps_common/storage/numeric_store_row.hpp>
#include <petuum_ps_common/storage/vector_store.hpp>

namespace petuum {

template<typename V>
using DenseRowCore  = NumericStoreRow<VectorStore, V >;

template<typename V>
class DenseRow : public DenseRowCore<V> {
public:
  DenseRow() { }
  ~DenseRow() { }

  V operator [](int32_t col_id) const {
    std::unique_lock<std::mutex> lock(DenseRowCore<V>::mtx_);
    return DenseRowCore<V>::store_.Get(col_id);
  }

  // Bulk read. Thread-safe.
  void CopyToVector(std::vector<V> *to) const {
    std::unique_lock<std::mutex> lock(DenseRowCore<V>::mtx_);
    DenseRowCore<V>::store_.Copy(to);
  }
};

}
