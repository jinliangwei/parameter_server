#pragma once

#include <petuum/ps_common/storage/numeric_store_row.hpp>
#include <petuum/ps_common/storage/vector_store.hpp>

namespace petuum {

template<typename V>
using DenseRowCore  = NumericStoreRow<VectorStore, V>;

//template<typename V>
//using DenseRowCore  = NumericStoreRow<VectorStore, V, NSCountImplCalc>;

template<typename V>
class DenseRow : public DenseRowCore<V> {
public:
  DenseRow() { }
  ~DenseRow() { }

  V operator [](int32_t col_id) const {
    return DenseRowCore<V>::store_.Get(col_id);
  }

  void CopyToMem(void *to) const {
    DenseRowCore<V>::store_.CopyToMem(to);
  }

  // not thread-safe
  const void *GetDataPtr() const {
    return DenseRowCore<V>::store_.GetDataPtr();
  }
};

}
