// author: jinliang

#include <petuum/ps/server/abstract_server_row.hpp>

#pragma once

namespace petuum {

// Disallow copy to avoid shared ownership of row_data.
// Allow move sematic for it to be stored in STL containers.
class ServerRow : public AbstractServerRow {
public:
  ServerRow():
      dirty_(false) { }

  explicit ServerRow(AbstractRow *row_data):
      row_data_(row_data),
      num_clients_subscribed_(0),
      dirty_(false) { }

  ~ServerRow() {
    if(row_data_ != 0)
      delete row_data_;
  }

  ServerRow(ServerRow && other):
      row_data_(other.row_data_),
      num_clients_subscribed_(other.num_clients_subscribed_),
      dirty_(other.dirty_) {
    other.row_data_ = 0;
  }

  ServerRow & operator = (ServerRow & other) = delete;

  virtual void ApplyBatchInc(
      const int32_t *column_ids,
      const void *update_batch, int32_t num_updates) {
  }

  virtual void ApplyBatchIncAccumImportance(
      const int32_t *column_ids,
      const void *update_batch, int32_t num_updates) {
  }

  virtual void ApplyDenseBatchInc(const void *update_batch, int32_t num_updates) {

  }

  virtual void ApplyDenseBatchIncAccumImportance(const void *update_batch,
                                         int32_t num_updates) {
  }

  virtual size_t SerializedSize() const {
    return 0;
  }

  virtual size_t Serialize(void *bytes) const {
    return 0;
  }

  void Subscribe(int32_t client_id) {
    if (callback_subs_.Subscribe(client_id))
      ++num_clients_subscribed_;
  }

  bool NoClientSubscribed() const {
    return (num_clients_subscribed_ == 0);
  }

  void Unsubscribe(int32_t client_id) {
    if (callback_subs_.Unsubscribe(client_id))
      --num_clients_subscribed_;
  }

  bool AppendRowToBuffs(
      int32_t client_id_st,
      boost::unordered_map<int32_t, RecordBuff> *buffs,
      const void *row_data, size_t row_size, int32_t row_id,
      int32_t *failed_client_id, size_t *num_clients) {
    return callback_subs_.AppendRowToBuffs(
        client_id_st, buffs, row_data,
        row_size, row_id, failed_client_id, num_clients);
  }

  bool IsDirty() const {
    return dirty_;
  }

  void ResetDirty() {
    dirty_ = false;
  }

  void AccumSerializedSizePerClient(
      boost::unordered_map<int32_t, size_t> *client_size_map) {
  }

  void AppendRowToBuffs(
      boost::unordered_map<int32_t, RecordBuff> *buffs,
      const void *row_data, size_t row_size, int32_t row_id,
      size_t *num_clients) {
  }

  double get_importance() {
    return importance_;
  }

  void AccumImportance(double importance) {
    importance_ += importance;
  }

  void ResetImportance() {
    importance_ = 0;
  }

  AbstractRow *get_row_data() const {
    return row_data_;
  }

protected:
  CallBackSubs callback_subs_;
  AbstractRow *row_data_;
  size_t num_clients_subscribed_;

  bool dirty_;

  double importance_;
};
}
