#pragma once

#include <petuum_ps_common/include/abstract_row.hpp>
#include <petuum_ps_common/include/row_access.hpp>
#include <petuum_ps_common/include/configs.hpp>
#include <petuum_ps_common/client/client_row.hpp>
#include <petuum_ps_common/include/row_id.hpp>
#include <libcuckoo/cuckoohash_map.hh>

namespace petuum {

class AbstractClientTable : boost::noncopyable {
public:
  AbstractClientTable() { }

  virtual ~AbstractClientTable() { }

  virtual void RegisterThread() = 0;

  virtual void GetAsyncForced(RowId row_id) = 0;
  virtual void GetAsync(RowId row_id) = 0;
  virtual void WaitPendingAsyncGet() = 0;
  virtual void RegisterRowSet(const std::set<RowId> &row_id_set) = 0;
  virtual void WaitForBulkInit() = 0;
  virtual void GetLocalRowIdSet(std::vector<RowId> *row_id_vec,
                                size_t num_clients,
                                size_t num_table_threads,
                                size_t table_thread_id,
                                size_t *total_num_rows) = 0;
  virtual void ThreadGet(RowId row_id, ThreadRowAccessor *row_accessor) = 0;
  virtual void ThreadInc(RowId row_id, int32_t column_id, const void *update) = 0;
  virtual void ThreadBatchInc(RowId row_id, const int32_t* column_ids,
                              const void* updates,
                              int32_t num_updates) = 0;
  virtual void ThreadDenseBatchInc(RowId row_id, const void *updates,
                                   int32_t index_st,
                                   int32_t num_updates) = 0;
  virtual void FlushThreadCache() = 0;

  virtual ClientRow *Get(RowId row_id, RowAccessor *row_accessor) = 0;
  virtual void Inc(RowId row_id, int32_t column_id, const void *update) = 0;
  virtual void BatchInc(RowId row_id, const int32_t* column_ids,
                        const void* updates,
                        int32_t num_updates) = 0;
  virtual void DenseBatchInc(RowId row_id, const void *updates,
                             int32_t index_st,
                             int32_t num_updates) = 0;
  virtual void Clock() = 0;

  virtual int32_t get_row_type() const = 0;
};

}  // namespace petuum
