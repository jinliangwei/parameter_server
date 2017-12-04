// Author: jinliang

#pragma once

#include <petuum_ps_common/include/row_access.hpp>
#include <petuum_ps_common/client/client_row.hpp>
#include <petuum_ps_common/util/striped_lock.hpp>
#include <petuum_ps_common/storage/clock_lru.hpp>
#include <petuum_ps_common/storage/abstract_process_storage.hpp>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <utility>
#include <cstdint>

namespace petuum {

class UnboundedSparseProcessStorage : public AbstractProcessStorage {
public:
  typedef std::function<ClientRow*(int32_t)> CreateClientRowFunc;
  UnboundedSparseProcessStorage(CreateClientRowFunc CreateClientRow);

  ~UnboundedSparseProcessStorage();

  // Find row row_id. Return true if found, otherwise false.
  ClientRow *Find(RowId row_id, RowAccessor* row_accessor);

  bool Find(RowId row_id);

  bool Insert(RowId row_id, ClientRow* client_row);

  size_t BulkInit(const RowId* row_id_set,
                  size_t num_rows,
                  size_t total_num_rows);

  void GetLocalRowIdSet(std::vector<RowId> *row_id_vec,
                        size_t num_clients,
                        size_t num_table_threads,
                        size_t table_thread_id,
                        size_t *total_num_rows);

private:
  std::unordered_map<RowId, ClientRow*> storage_map_;
  CreateClientRowFunc CreateClientRow_;
  std::mutex bulk_init_mtx_;
  size_t total_num_rows_ {0};
  size_t num_bulk_inits_ {0};
};


}  // namespace petuum
