#pragma once

#include <petuum/ps_common/storage/numeric_store_row.hpp>
#include <petuum/ps_common/storage/sorted_vector_store.hpp>

namespace petuum {

template<typename V>
using SortedVectorRowCore  = NumericStoreRow<SortedVectorStore, V >;

template<typename V>
class SortedVectorRow : public SortedVectorRowCore<V> {
public:
  SortedVectorRow() { }
  ~SortedVectorRow() { }

  V operator [](int32_t col_id) const {
    return SortedVectorRowCore<V>::store_.Get(col_id);
  }
};

}
