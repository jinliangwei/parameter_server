#pragma once

#include <petuum/ps_common/index/sparse_index.hpp>

namespace petuum {

class ExtensibleSparseIndex : public SparseIndex {
public:
  void Add(int32_t row_id, int32_t index) {
    map_[row_id] = index;
  }
};

}
