#pragma once

#include <sparsehash/dense_hash_map>
#include <petuum/ps_common/index/abstract_index.hpp>

namespace petuum {

class SparseIndex : public AbstractIndex {
public:
  SparseIndex(void *mem, size_t num_bytes) {
    int32_t *array = reinterpret_cast<int32_t*>(mem);
    size_t num_entries = num_bytes / sizeof(int32_t);
    for (size_t i = 0; i < num_entries; ++i) {
      map_[array[i]] = i;
    }
  }

  ~SparseIndex() { }

  int32_t Translate(int32_t row_id) const {
    auto iter = map_.find(row_id);
    if (iter == map_.end()) return -1;
    return iter->second;
  }

  size_t GetSizeOfSerializedRowIDs() const {
    return sizeof(int32_t) * map_.size();
  }

  size_t SerializeRowIDs(void *mem) const {
    int32_t *array = reinterpret_cast<int32_t*>(mem);
    for (auto iter = map_.begin(); iter != map_.end(); ++iter) {
      array[i] = iter->first;
    }
    return sizeof(int32_t) * map_.size();
  }

  bool CheckInRange(int32_t row_id) const {
    auto iter = map_.find(row_id);
    if (iter == map_.end()) return false;
    return true;
  }

protected:
  google::dense_hash_map<int32_t, int32_t> map_;
};

}
