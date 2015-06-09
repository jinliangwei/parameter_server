// Author: jinliang
#pragma once

#include <stdint.h>
#include <map>

#include <utility>
#include <glog/logging.h>

#include <sparsehash/dense_hash_map>
#include <petuum/ps_common/oplog/abstract_row_oplog.hpp>

namespace petuum {

template<typename V>
class SparseRowOpLog : public virtual AbstractRowOpLog {
public:
  SparseRowOpLog() { }

  ~SparseRowOpLog() { }

  void Init(size_t capacity) { }

  void Reset() {
    update_map_.clear();
  }

  void Inc(int32_t col_id, const void *update) {
    const V &u = *(reinterpret_cast<const V*>(update));
    update_map_[col_id] += u;
  }

  void BatchInc(const int32_t *col_ids, const void *updates,
                size_t num_updates) {
    const V *u_array = reinterpret_cast<const V*>(updates);
    for (int i = 0; i < num_updates; ++i) {
      update_map_[col_ids[i]] += u_array[i];
    }
  }

  void DenseBatchInc(int32_t col_id_st, const void *updates,
                     size_t num_updates) {
    const V *u_array = reinterpret_cast<const V*>(updates);
    for (int32_t i = 0; i < num_updates; ++i) {
      int32_t col_id = i + col_id_st;
      update_map_[col_id] += u_array[i];
    }
  }

  size_t GetNumUpdates() const {
    return update_map_.size();
  }

  size_t ClearZerosAndGetNoneZeroSize() {
    auto update_iter = update_map_.begin();
    for (; update_iter != update_map_.end(); update_iter++) {
      if (update_iter->second == 0) {
        update_map_.erase(update_iter->first);
      }
    }

    update_map_.resize(0);
    return update_map_.size();
  }

  size_t GetSparseSerializedSize() const {
    size_t num_updates = update_map_.size();
    return sizeof(int32_t) + sizeof(int32_t) * num_updates
        + sizeof(V) * num_updates;
  }

  size_t GetDenseSerializedSize() const {
    LOG(FATAL) << "Sparse OpLog does not support dense serialize";
    return 0;
  }

  // Serization format:
  // 1) number of updates in that row
  // 2) total size for column ids
  // 3) total size for update array
  size_t SerializeSparse(void *mem) {
    size_t num_oplogs = update_map_.size();
    int32_t *mem_num_updates = reinterpret_cast<int32_t*>(mem);
    *mem_num_updates = num_oplogs;

    uint8_t *mem_index = reinterpret_cast<uint8_t*>(mem) + sizeof(int32_t);
    uint8_t *mem_oplogs = mem_index + num_oplogs*sizeof(int32_t);

    for (auto update_iter = update_map_.begin(); update_iter != update_map_.end();
         ++update_iter) {
      *(reinterpret_cast<int32_t*>(mem_index)) = update_iter->first;
      memcpy(mem_oplogs, update_iter->second, sizeof(V));
      mem_index += sizeof(int32_t);
      mem_oplogs += sizeof(V);
    }
    return GetSparseSerializedSize();
  }

  size_t SerializeDense(void *mem) {
    LOG(FATAL) << "Sparse OpLog does not support dense serialize";
    return GetSparseSerializedSize();
  }

  const void *ParseDenseSerializedOpLog(
      const void *mem, int32_t *num_updates,
      size_t *serialized_size) const {
    LOG(FATAL) << "Sparse OpLog does not support dense serialize";
    return mem;
  }

  virtual const void *ParseSparseSerializedOpLog(
      const void *mem, int32_t const ** column_ids,
      int32_t *num_updates, size_t *serialized_size) const {
    const uint8_t *mem_uint8 = reinterpret_cast<const uint8_t*>(mem);
    *num_updates = *(reinterpret_cast<const int32_t*>(mem_uint8));

    mem_uint8 += sizeof(int32_t);
    *column_ids = reinterpret_cast<const int32_t*>(mem_uint8);

    mem_uint8 += *num_updates*sizeof(int32_t);

    *serialized_size
        = sizeof(int32_t) + *num_updates*(sizeof(int32_t) + sizeof(V));
    return mem_uint8;
  }

protected:
  google::dense_hash_map<int32_t, int32_t> update_map_;
};
}
