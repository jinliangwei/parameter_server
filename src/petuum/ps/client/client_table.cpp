#include <glog/logging.h>

#include <petuum/ps/client/client_table.hpp>
#include <petuum/ps_common/util/stats.hpp>
#include <petuum/ps_common/util/class_register.hpp>
#include <petuum/ps_common/index/row_id_set.hpp>

#include <cmath>

namespace petuum {

ClientTable::ClientTable(int32_t table_id, const ClientTableConfig &config):
    table_id_(table_id),
    sample_row_(
        ClassRegistry<AbstractRow>::GetRegistry().CreateObject(
            config.table_info.row_type)),
    client_table_config_(config),
    parameter_cache_(config.cache_capacity,
                     [&config]() {
                       auto &registry = ClassRegistry<AbstractRow>::GetRegistry();
                       auto row = registry.CreateObject(config.table_info.row_type);
                       row->Init(config.table_info.row_capacity);
                       return row;
                     },
                     [](AbstractRow* const &obj) {
                         delete obj;
                     }),
    update_buffer_(config.cache_capacity,
                   [&config](int n) {
                     auto &registry = ClassRegistry<AbstractRowOpLog>::GetRegistry();
                     auto row_oplog = registry.CreateObject(
                         config.table_info.row_oplog_type);
                     row_oplog->Init(config.table_info.row_capacity);
                     int32_t row_id = RowIDSetParser::get_nth_id(
                         n, config.row_id_set);
                     RowMeta meta = {
                       row_id,
                       0,
                       0.0,
                       0
                     };
                     return std::make_pair(meta, row_oplog);
                   },
                   [](const std::pair<RowMeta, AbstractRowOpLog*> &obj){
                     delete obj.second;
                   }),
  lock_engine_(config.cache_capacity) { }

ClientTable::~ClientTable() {
  delete index_;
}

void ClientTable::Get(int32_t row_id, int32_t col_id, AbstractRow *to,
                      int32_t data_clock) {

}

void ClientTable::CopyToVector(int32_t row_id, void *vec,
                               int32_t data_clock) {

}

void ClientTable::Inc(int32_t row_id, int32_t column_id, const void *update) {
  STATS_APP_SAMPLE_INC_BEGIN(table_id_);

  STATS_APP_SAMPLE_INC_END(table_id_);
}

void ClientTable::BatchInc(
    int32_t row_id, const int32_t* column_ids,
    const void* updates, size_t num_updates) {
  STATS_APP_SAMPLE_BATCH_INC_BEGIN(table_id_);

  STATS_APP_SAMPLE_BATCH_INC_END(table_id_);
}

void ClientTable::DenseBatchInc(
    int32_t row_id, const void *updates, int32_t index_st,
    size_t num_updates) {
  STATS_APP_SAMPLE_BATCH_INC_BEGIN(table_id_);

  STATS_APP_SAMPLE_BATCH_INC_END(table_id_);
}

void ClientTable::Clock() {
  STATS_APP_SAMPLE_CLOCK_BEGIN(table_id_);

  STATS_APP_SAMPLE_CLOCK_END(table_id_);
}

}  // namespace petuum
