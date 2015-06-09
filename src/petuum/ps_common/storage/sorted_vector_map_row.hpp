#pragma once

#include <petuum/ps_common/storage/numeric_store_row.hpp>
#include <petuum/ps_common/storage/sorted_vector_map_store.hpp>

namespace petuum {

template<typename V>
using SortedVectorMapRowCore  = NumericStoreRow<SortedVectorMapStore, V >;

template<typename V>
class SortedVectorMapRow : public SortedVectorMapRowCore<V> {
public:
  SortedVectorMapRow() { }
  ~SortedVectorMapRow() { }

  V operator [](int32_t col_id) const {
    return SortedVectorMapRowCore<V>::store_.Get(col_id);
  }
};

}
