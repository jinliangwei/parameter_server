#pragma once

#include <stdint.h>
#include <petuum/ps_common/index/abstract_index.hpp>

namespace petuum {

class DenseIndex : public AbstractIndex {
public:
  DenseIndex(size_t capacity):
      capacity_(capacity) { }

  ~DenseIndex() { }

  int32_t Translate(int32_t row_id) const {
    return row_id;
  }

  size_t GetSizeOfSerializedRowIDs() const {
    return 0;
  }

  size_t SerializeRowIDs(void *mem) const {
    return 0;
  }

  bool CheckInRange(int32_t row_id) const {
    return (row_id >= 0 && row_id < capacity_);
  }

private:
  size_t capacity_;
};

}
