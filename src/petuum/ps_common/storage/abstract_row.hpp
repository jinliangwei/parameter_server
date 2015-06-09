#pragma once

#include <petuum/ps_common/util/noncopyable.hpp>
#include <stdint.h>
#include <cstdlib>

namespace petuum {

// This class defines the interface of the Row type.  ApplyUpdate() and
// ApplyBatchUpdate() have to be concurrent with each other and with other
// functions that may be invoked by application threads.  Petuum system does
// not require thread safety of other functions.
class AbstractRow : NonCopyable {
public:
  virtual ~AbstractRow() { }

  virtual void Init(size_t capacity) = 0;

  virtual void CopyToVector(void *vec) const = 0;

  virtual size_t get_update_size() const = 0;

  // Upper bound of the number of bytes that serialized row shall occupy.
  // Find some balance between tightness and time complexity.
  virtual size_t SerializedSize() const = 0;

  // Bytes points to a chunk of allocated memory whose size is guaranteed to
  // be at least SerializedSize(). Need not be thread safe. Return the exact
  // size of serialized row.
  virtual size_t Serialize(void *bytes) const = 0;

  // Deserialize and initialize a row. Init is not called yet.
  // Return true on success, false otherwise. Need not be thread-safe.
  virtual void Deserialize(const void* data, size_t num_bytes) = 0;

  // Init or Deserialize has been called on this row.
  virtual void ResetRowData(const void *data, size_t num_bytes) = 0;

  virtual void ApplyInc(int32_t column_id, const void *update) = 0;

  virtual double ApplyIncGetReImportance(int32_t column_id,
                                         const void *update) = 0;

  // Updates are stored contiguously in the memory pointed by update_batch.
  virtual void ApplyBatchInc(const int32_t *column_ids,
    const void* update_batch, int32_t num_updates) = 0;

  // Updates are stored contiguously in the memory pointed by update_batch.
  virtual double ApplyBatchIncGetReImportance(
      const int32_t *column_ids,
      const void* update_batch, int32_t num_updates) = 0;

  virtual void ApplyDenseBatchInc(
      const void* update_batch, int32_t index_st, int32_t num_updates) = 0;

  // The update batch contains an update for each each element within the
  // capacity of the row, in the order of increasing column_ids.
  virtual double ApplyDenseBatchIncGetReImportance(
      const void* update_batch, int32_t index_st, int32_t num_updates) = 0;

};

}   // namespace petuum
