#pragma once

#include <petuum/ps_common/storage/numeric_store_row.hpp>
#include <petuum/ps_common/storage/map_store.hpp>

namespace petuum {

template<typename V>
using SparseRowCore  = NumericStoreRow<MapStore, V >;

template<typename V>
class SparseRow : public SparseRowCore<V> {
public:
  SparseRow() { }
  ~SparseRow() { }

  V operator [](int32_t col_id) const {
    return SparseRowCore<V>::store_.Get(col_id);
  }
};

}
