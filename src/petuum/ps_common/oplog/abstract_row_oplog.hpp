// Author: jinliang
#pragma once

#include <atomic>
#include <stdint.h>
#include <petuum/ps_common/util/noncopyable.hpp>

namespace petuum {
class AbstractRowOpLog : private NonCopyable {
public:
  AbstractRowOpLog():
      not_empty_(false) { }

  virtual ~AbstractRowOpLog() { }

  virtual void Init(size_t capacity) = 0;

  // clean up the oplogs and allow the RowOpLog to be reused as a fresh one
  virtual void Reset() = 0;

  bool get_acquire_not_empty() const {
    return not_empty_.load(std::memory_order_acquire);
  }

  void set_release_not_empty(bool v) {
    not_empty_.store(v, std::memory_order_release);
  }

  virtual void Inc(int32_t col_id, const void *update) = 0;
  virtual void BatchInc(const int32_t *col_ids, const void *updates,
                        size_t num_updates) = 0;
  virtual void DenseBatchInc(int32_t col_id_st, const void *updates,
                             size_t num_updates) = 0;

  virtual size_t GetNumUpdates() const = 0;
  virtual size_t ClearZerosAndGetNoneZeroSize() = 0;

  virtual size_t GetSparseSerializedSize() const = 0;
  virtual size_t GetDenseSerializedSize() const = 0;

  virtual size_t SerializeSparse(void *mem) const = 0;
  virtual size_t SerializeDense(void *mem) const = 0;

  // Certain oplog types do not support dense serialization.
  virtual const void *ParseDenseSerializedOpLog(
      const void *mem, int32_t *num_updates,
      size_t *serialized_size) const = 0;

  // The default sparse serialization format:
  // 1) number of updates in that row
  // 2) total size for column ids
  // 3) total size for update array
  virtual const void *ParseSparseSerializedOpLog(
      const void *mem, int32_t const ** column_ids,
      int32_t *num_updates, size_t *serialized_size) const = 0;

  typedef size_t (AbstractRowOpLog::*SerializeFunc)(void *mem) const;

private:
  std::atomic_bool not_empty_;
};
}
