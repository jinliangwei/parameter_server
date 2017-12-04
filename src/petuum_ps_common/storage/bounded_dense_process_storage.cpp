// Author: Jinliang, Yihua Fang

#include <utility>
#include <petuum_ps_common/storage/bounded_dense_process_storage.hpp>
#include <petuum_ps_common/include/constants.hpp>
#include <glog/logging.h>

namespace petuum {

BoundedDenseProcessStorage::BoundedDenseProcessStorage(size_t capacity,
                                                       CreateClientRowFunc CreateClientRow,
                                                       int32_t offset):
    storage_vec_(capacity),
    capacity_(capacity),
    CreateClientRow_(CreateClientRow),
    offset_(offset) {

  for (size_t i = 0; i < capacity_; ++i) {
    storage_vec_[i] = CreateClientRow_(0);
  }

  if (offset == 0)
    Find_ = &BoundedDenseProcessStorage::FindNoOffset;
  else
    Find_ = &BoundedDenseProcessStorage::FindOffset;
}

BoundedDenseProcessStorage::~BoundedDenseProcessStorage() {
  for (auto &row : storage_vec_) {
    delete row;
    row = 0;
  }
}

ClientRow *BoundedDenseProcessStorage::Find(RowId row_id,
                                            RowAccessor* row_accessor __attribute__ ((unused))) {
  return (this->*Find_)(row_id);
}

ClientRow *BoundedDenseProcessStorage::FindOffset(RowId row_id) {
  int32_t vec_idx = row_id - offset_;
  return storage_vec_[vec_idx];
}

ClientRow *BoundedDenseProcessStorage::FindNoOffset(RowId row_id) {
  CHECK(row_id < storage_vec_.size()) << "row id = " << row_id;
  //LOG(INFO) << "row_id = " << row_id;
  return storage_vec_[row_id];
}

bool BoundedDenseProcessStorage::Find(RowId row_id) {
  return true;
}

bool BoundedDenseProcessStorage::Insert(RowId row_id, ClientRow* client_row) {
  LOG(FATAL) << "Operation not supported! to be inserted = " << row_id;
  return false;
}

}  // namespace petuum
