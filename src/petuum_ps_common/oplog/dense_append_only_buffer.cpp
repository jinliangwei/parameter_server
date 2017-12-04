#include <petuum_ps_common/oplog/dense_append_only_buffer.hpp>
#include <string.h>
#include <glog/logging.h>

namespace petuum {

bool DenseAppendOnlyBuffer::Inc(RowId row_id, int32_t col_id,
                                const void *delta) {
  LOG(FATAL) << "Unsupported operation for this buffer type";
  return false;
}

bool DenseAppendOnlyBuffer::BatchInc(RowId row_id, const int32_t *col_ids,
                                     const void *deltas, int32_t num_updates) {
  LOG(FATAL) << "Unsupported operation for this buffer type";
  return false;
}

// The assumption is row_capacity_ == num_updates
bool DenseAppendOnlyBuffer::DenseBatchInc(RowId row_id, const void *deltas,
                                          int32_t index_st,
                                          int32_t num_updates) {
  if (size_ + sizeof(RowId) + update_size_*num_updates > capacity_)
    return false;

  *(reinterpret_cast<RowId*>(buff_.get() + size_)) = row_id;
  size_ += sizeof(RowId);

  memcpy(buff_.get() + size_, deltas, update_size_*num_updates);
  size_ += update_size_*num_updates;

  return true;
}


void DenseAppendOnlyBuffer::InitRead() {
  read_ptr_ = buff_.get();
}

const void *DenseAppendOnlyBuffer::Next(
    RowId *row_id, int32_t const **col_ids, int32_t *num_updates) {
  if (read_ptr_ >= buff_.get() + size_)
    return 0;

  *row_id = *(reinterpret_cast<RowId*>(read_ptr_));
  read_ptr_ += sizeof(RowId);

  *num_updates = row_capacity_;

  void *update_ptr = read_ptr_;

  read_ptr_ += update_size_*row_capacity_;
  return update_ptr;
}

}
