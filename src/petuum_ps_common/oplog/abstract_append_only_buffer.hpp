// Author: Jinliang
#pragma once

#include <stdint.h>
#include <memory>

#include <boost/noncopyable.hpp>
#include <petuum_ps_common/include/row_id.hpp>

namespace petuum {

class AbstractAppendOnlyBuffer : boost::noncopyable {
public:
  AbstractAppendOnlyBuffer(int32_t thread_id,
                           size_t capacity, size_t update_size):
      thread_id_(thread_id),
      capacity_(capacity),
      size_(0),
      update_size_(update_size),
      buff_(new uint8_t[capacity]) { }

  virtual ~AbstractAppendOnlyBuffer() { }

  int32_t get_thread_id() const {
    return thread_id_;
  }

  void ResetSize() {
    size_ = 0;
  }

  // return true if succeed, otherwise false
  virtual bool Inc(RowId row_id, int32_t col_id, const void *delta) = 0;
  virtual bool BatchInc(RowId row_id, const int32_t *col_ids,
                        const void *deltas, int32_t num_updates) = 0;
  virtual bool DenseBatchInc(RowId row_id, const void *updates, int32_t index_st,
                             int32_t num_updates) = 0;
  virtual void InitRead() = 0;
  virtual const void *Next(RowId *row_id, int32_t const **col_ids,
                           int32_t *num_updates) = 0;

protected:
  const int32_t thread_id_;

  const size_t capacity_;
  size_t size_;

  const size_t update_size_;
  std::unique_ptr<uint8_t[]> buff_;

  uint8_t* read_ptr_;
};

}
