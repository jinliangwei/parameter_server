// Author: jinliang
#pragma once

#include <stdint.h>

#include <memory>
#include <string.h>
#include <glog/logging.h>

#include <petuum/ps_common/oplog/abstract_row_oplog.hpp>

namespace petuum {

template<typename V>
class DenseRowOpLog : public virtual AbstractRowOpLog {
public:
  DenseRowOpLog() { }

  ~DenseRowOpLog() { }

  void Reset() {
    update_vec_.resize(row_size_, 0);
  }

  void Init(size_t capacity) {
    row_size_ = capacity;
    Reset();
  }

  void Inc(int32_t col_id, const void *update) {
    const V &u = *(reinterpret_cast<const V*>(update));
    update_vec_[col_id] += u;
  }

  void BatchInc(const int32_t *col_ids, const void *updates,
                size_t num_updates) {
    const V *u_array = reinterpret_cast<const V*>(updates);
    for (int i = 0; i < num_updates; ++i) {
      update_vec_[col_ids[i]] += u_array[i];
    }
  }

  void DenseBatchInc(int32_t col_id_st, const void *updates,
                     size_t num_updates) {
    const V *u_array = reinterpret_cast<const V*>(updates);
    for (int32_t i = 0; i < num_updates; ++i) {
      int32_t col_id = i + col_id_st;
      update_vec_[col_id] += u_array[i];
    }
  }

  size_t GetNumUpdates() const {
    return row_size_;
  }

  virtual size_t ClearZerosAndGetNoneZeroSize() {
    num_nonzeros_ = 0;
    for (auto update : update_vec_) {
      if (update == 0) num_nonzeros_++;
    }
    return num_nonzeros_;
  }

  virtual size_t GetSparseSerializedSize() const {
    return sizeof(int32_t) + sizeof(int32_t) * num_nonzeros_
        + sizeof(V) * num_nonzeros_;
  }

  virtual size_t GetDenseSerializedSize() const {
    return row_size_ * sizeof(V);
  }

  virtual size_t SerializeSparse(void *mem) const {
    int32_t col_id = -1;
    int32_t *mem_num_updates = reinterpret_cast<int32_t*>(mem);
    *mem_num_updates = num_nonzeros_;

    uint8_t *mem_index = reinterpret_cast<uint8_t*>(mem) + sizeof(int32_t);
    uint8_t *mem_oplogs = mem_index + num_nonzeros_ * sizeof(int32_t);

    for (auto update : update_vec_) {
      col_id++;
      if (update == 0) continue;
      *(reinterpret_cast<int32_t*>(mem_index)) = col_id;
      *(reinterpret_cast<V*>(mem_oplogs)) = update;
      mem_index += sizeof(int32_t);
      mem_oplogs += sizeof(V);
    }

    return GetSparseSerializedSize();
  }

  virtual size_t SerializeDense(void *mem) const {
    memcpy(mem, update_vec_.data(), row_size_ * sizeof(V));
    return GetDenseSerializedSize();
  }

  virtual const void *ParseDenseSerializedOpLog(const void *mem,
                                        int32_t *num_updates,
                                        size_t *serialized_size) const {
    *num_updates = row_size_;
    *serialized_size = row_size_ * sizeof(V);
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
  std::vector<V> update_vec_;
  size_t row_size_; // capacity
  size_t num_nonzeros_;
};

}
