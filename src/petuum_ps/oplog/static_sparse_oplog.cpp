#include <petuum_ps/oplog/static_sparse_oplog.hpp>
#include <petuum_ps/thread/context.hpp>
#include <petuum_ps_common/include/constants.hpp>
#include <petuum_ps_common/include/configs.hpp>
#include <petuum_ps/oplog/meta_row_oplog.hpp>
#include <petuum_ps_common/oplog/dense_row_oplog.hpp>
#include <petuum_ps_common/oplog/sparse_row_oplog.hpp>

#include <functional>

namespace petuum {

StaticSparseOpLog::StaticSparseOpLog(const AbstractRow *sample_row,
                                     size_t dense_row_oplog_capacity,
                                     int32_t row_oplog_type,
                                     bool version_maintain):
  update_size_(sample_row->get_update_size()),
  locks_(GlobalContext::GetLockPoolSize()),
  sample_row_(sample_row),
  dense_row_oplog_capacity_(dense_row_oplog_capacity) {
  if (GlobalContext::get_consistency_model() == SSPAggr) {
    if (row_oplog_type == RowOpLogType::kDenseRowOpLog) {
      if (!version_maintain)
        CreateRowOpLog_ = CreateRowOpLog::CreateDenseMetaRowOpLog;
      else
        CreateRowOpLog_ = CreateRowOpLog::CreateVersionDenseMetaRowOpLog;
    } else if (row_oplog_type == RowOpLogType::kSparseRowOpLog)
      CreateRowOpLog_ = CreateRowOpLog::CreateSparseMetaRowOpLog;
    else if(row_oplog_type == RowOpLogType::kDenseRowOpLogFloat16)
      CreateRowOpLog_ = CreateRowOpLog::CreateDenseMetaRowOpLogFloat16;
    else
      CreateRowOpLog_ = CreateRowOpLog::CreateSparseVectorMetaRowOpLog;
  } else {
    if (row_oplog_type == RowOpLogType::kDenseRowOpLog) {
      if (!version_maintain)
        CreateRowOpLog_ = CreateRowOpLog::CreateDenseRowOpLog;
      else
        CreateRowOpLog_ = CreateRowOpLog::CreateVersionDenseRowOpLog;
    } else if (row_oplog_type == RowOpLogType::kSparseRowOpLog)
      CreateRowOpLog_ = CreateRowOpLog::CreateSparseRowOpLog;
    else if(row_oplog_type == RowOpLogType::kDenseRowOpLogFloat16)
      CreateRowOpLog_ = CreateRowOpLog::CreateDenseRowOpLogFloat16;
    else
      CreateRowOpLog_ = CreateRowOpLog::CreateSparseVectorRowOpLog;
  }
}

StaticSparseOpLog::~StaticSparseOpLog() {
  for (auto &row_pair : oplog_map_) {
    if (row_pair.second != nullptr) {
      delete row_pair.second;
    }
  }
}

int32_t StaticSparseOpLog::Inc(RowId row_id, int32_t column_id, const void *delta) {
  Unlocker<> unlocker;
  locks_.Lock(row_id, &unlocker);

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);

  if (row_oplog == nullptr)
    row_oplog = CreateAndInsertRowOpLog(row_id);

  void *oplog_delta = row_oplog->FindCreate(column_id);
  sample_row_->AddUpdates(column_id, oplog_delta, delta);

  return 0;
}

int32_t StaticSparseOpLog::BatchInc(RowId row_id, const int32_t *column_ids,
                                    const void *deltas, int32_t num_updates) {
  Unlocker<> unlocker;
  locks_.Lock(row_id, &unlocker);

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);

  if (row_oplog == nullptr)
    row_oplog = CreateAndInsertRowOpLog(row_id);

  const uint8_t* deltas_uint8 = reinterpret_cast<const uint8_t*>(deltas);

  for (int i = 0; i < num_updates; ++i) {
    void *oplog_delta = row_oplog->FindCreate(column_ids[i]);
    sample_row_->AddUpdates(column_ids[i], oplog_delta, deltas_uint8
                            + update_size_*i);
  }

  return 0;
}

int32_t StaticSparseOpLog::DenseBatchInc(RowId row_id, const void *updates,
                                         int32_t index_st, int32_t num_updates) {
  LOG(FATAL) << "Unsupported operation";
  return 0;
}

bool StaticSparseOpLog::FindOpLog(RowId row_id, OpLogAccessor *oplog_accessor) {
  locks_.Lock(row_id, oplog_accessor->get_unlock_ptr());

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog != 0) {
    oplog_accessor->set_row_oplog(row_oplog);
    return true;
  }

  (oplog_accessor->get_unlock_ptr())->Release();
  locks_.Unlock(row_id);
  return false;
}

bool StaticSparseOpLog::FindInsertOpLog(RowId row_id,
                                        OpLogAccessor *oplog_accessor) {
  bool new_create = false;
  locks_.Lock(row_id, oplog_accessor->get_unlock_ptr());

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);

  if (row_oplog == nullptr) {
    row_oplog = CreateAndInsertRowOpLog(row_id);
    new_create = true;
  }

  oplog_accessor->set_row_oplog(row_oplog);
  return new_create;
}

bool StaticSparseOpLog::FindAndLock(RowId row_id,
                                    OpLogAccessor *oplog_accessor) {
  locks_.Lock(row_id, oplog_accessor->get_unlock_ptr());

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog != nullptr) {
    oplog_accessor->set_row_oplog(row_oplog);
    return true;
  }

  return false;
}

AbstractRowOpLog *StaticSparseOpLog::FindOpLog(RowId row_id) {
  return FindRowOpLog(row_id);
}

AbstractRowOpLog *StaticSparseOpLog::FindInsertOpLog(RowId row_id) {

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog == 0)
    row_oplog = CreateAndInsertRowOpLog(row_id);

  return row_oplog;
}

bool StaticSparseOpLog::GetEraseOpLog(RowId row_id,
                                      AbstractRowOpLog **row_oplog_ptr) {
  Unlocker<> unlocker;
  locks_.Lock(row_id, &unlocker);

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog == nullptr)
    return false;

  auto iter = oplog_map_.find(row_id);
  CHECK(iter != oplog_map_.end());
  iter->second = nullptr;
  *row_oplog_ptr = row_oplog;
  return true;
}

bool StaticSparseOpLog::GetEraseOpLogIf(RowId row_id,
                                        GetOpLogTestFunc test,
                                        void *test_args,
                                        AbstractRowOpLog **row_oplog_ptr) {
  Unlocker<> unlocker;
  locks_.Lock(row_id, &unlocker);

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog == nullptr)
    return false;

  if (!test(row_oplog, test_args)) {
    *row_oplog_ptr = nullptr;
    return true;
  }

  auto iter = oplog_map_.find(row_id);
  CHECK(iter != oplog_map_.end());
  iter->second = nullptr;
  *row_oplog_ptr = row_oplog;
  return true;
}

bool StaticSparseOpLog::GetInvalidateOpLogMeta(RowId row_id,
                                               RowOpLogMeta *row_oplog_meta) {
  Unlocker<> unlocker;
  locks_.Lock(row_id, &unlocker);

  AbstractRowOpLog *row_oplog = FindRowOpLog(row_id);
  if (row_oplog == nullptr)
    return false;

  MetaRowOpLog *meta_row_oplog = dynamic_cast<MetaRowOpLog*>(row_oplog);

  *row_oplog_meta = meta_row_oplog->GetMeta();
  meta_row_oplog->InvalidateMeta();
  meta_row_oplog->ResetImportance();
  return true;
}

AbstractAppendOnlyBuffer *StaticSparseOpLog::GetAppendOnlyBuffer(int32_t comm_channel_idx) {
  LOG(FATAL) << "Unsupported operation for OpLogType";
  return 0;
}

void StaticSparseOpLog::PutBackBuffer(int32_t comm_channel_idx, AbstractAppendOnlyBuffer *buff) {
  LOG(FATAL) << "Unsupported operation for OpLogType";
}

void StaticSparseOpLog::BulkInit(const RowId* row_id_set,
                                 size_t num_rows) {
  std::unique_lock<std::mutex> lock(bulk_init_mtx_);
  for (size_t i = 0; i < num_rows; i++) {
    oplog_map_.emplace(row_id_set[i], nullptr);
  }
}


AbstractRowOpLog *StaticSparseOpLog::FindRowOpLog(RowId row_id) {
   auto iter = oplog_map_.find(row_id);
   CHECK(iter != oplog_map_.end()) << " cant't find " << row_id;
   return iter->second;
}

AbstractRowOpLog *StaticSparseOpLog::CreateAndInsertRowOpLog(RowId row_id) {
   AbstractRowOpLog* row_oplog = CreateRowOpLog_(update_size_, sample_row_,
                                                dense_row_oplog_capacity_);
   auto iter = oplog_map_.find(row_id);
   CHECK(iter != oplog_map_.end());
   iter->second = row_oplog;
   return row_oplog;
}

}
