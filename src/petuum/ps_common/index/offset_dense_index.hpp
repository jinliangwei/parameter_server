#pragma once

#include <stdint.h>
#include <petuum/ps_common/index/abstract_index.hpp>

namespace petuum {

class OffsetDenseIndex : public AbstractIndex {
public:
  OffsetDenseIndex(size_t offset):
      offset_(offset) { }

  OffsetDenseIndex(void *mem):
      offset_(*reinterpret_cast<int32_t*>(mem)) { }

  ~OffsetDenseIndex() { }

  int32_t Translate(int32_t row_id) const {
    return row_id - offset;
  }

  size_t GetSizeOfSerializedRowIDs() const {
    return sizeof(int32_t);
  }

  size_t SerializeRowIDs(void *mem) const {
    *reinterpret_cast<int32_t*>(mem) = offset_;
    return sizeof(int32_t);
  }

  bool CheckInRange(int32_t row_id) const {
    return (row_id >= 0 && (row_id - offset) < capacity_);
  }

private:
  size_t offset_;
};

}
