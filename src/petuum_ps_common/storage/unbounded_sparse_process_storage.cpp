// Author: Jinliang

#include <utility>
#include <petuum_ps_common/storage/unbounded_sparse_process_storage.hpp>
#include <petuum_ps_common/include/constants.hpp>
#include <glog/logging.h>

namespace petuum {

UnboundedSparseProcessStorage::UnboundedSparseProcessStorage(CreateClientRowFunc CreateClientRow):
    CreateClientRow_(CreateClientRow) {
}

UnboundedSparseProcessStorage::~UnboundedSparseProcessStorage() {
  for (auto &row : storage_map_) {
    delete row.second;
    row.second = nullptr;
  }
}

ClientRow *UnboundedSparseProcessStorage::Find(RowId row_id,
                                               RowAccessor* row_accessor __attribute__ ((unused))) {
  auto iter = storage_map_.find(row_id);
  if (iter == storage_map_.end())
    return nullptr;
  return iter->second;
}

bool UnboundedSparseProcessStorage::Find(RowId row_id) {
  auto iter = storage_map_.find(row_id);
  if (iter == storage_map_.end())
    return false;
  return true;
}

bool UnboundedSparseProcessStorage::Insert(RowId row_id, ClientRow* client_row) {
  LOG(FATAL) << "Operation not supported! to be inserted = " << row_id;
  return false;
}

size_t UnboundedSparseProcessStorage::BulkInit(const RowId* row_id_set,
                                               size_t num_rows,
                                               size_t total_num_rows) {
  std::unique_lock<std::mutex> lock(bulk_init_mtx_);
  for (size_t i = 0; i < num_rows; i++) {
    LOG(INFO) << __func__ << " " << row_id_set[i];
    storage_map_[row_id_set[i]] = CreateClientRow_(0);
  }
  num_bulk_inits_++;
  total_num_rows_ += total_num_rows;
  return num_bulk_inits_;
}

void UnboundedSparseProcessStorage::GetLocalRowIdSet(std::vector<RowId> *row_id_vec,
                                                     size_t num_clients,
                                                     size_t num_table_threads,
                                                     size_t table_thread_id,
                                                     size_t *total_num_rows) {
  row_id_vec->clear();
  for (auto &row_pair : storage_map_) {
    auto row_id = row_pair.first;
    if ((row_id / num_clients) % num_table_threads  == table_thread_id)
      row_id_vec->push_back(row_id);
  }
  *total_num_rows = total_num_rows_;
}

}  // namespace petuum
