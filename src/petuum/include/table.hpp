// table.hpp
// author: jinliang

#pragma once

#include <boost/utility.hpp>
#include <vector>

#include <petuum/ps_common/client/abstract_client_table.hpp>

namespace petuum {

// UPDATE can be V = {int, double, ...} when the entry is simple numerical
// type. If the entry type is a struct, UPDATE can be id:val pair, like {1:5}
// which increment field 1 of struct by 5.
template<typename UPDATE>
class UpdateBatch {
public:
  UpdateBatch():
    col_ids_(0),
    updates_(0) { }

  UpdateBatch(size_t num_updates):
      col_ids_(num_updates),
      updates_(num_updates) { }

  ~UpdateBatch() { }

  UpdateBatch (const UpdateBatch & other) :
      col_ids_(other.col_ids_),
      updates_(other.updates_) { }

  UpdateBatch & operator = (const UpdateBatch &other) {
    col_ids_ = other.col_ids_;
    updates_ = other.updates_;
    return *this;
  }

  void Update(int32_t column_id, const UPDATE& update) {
    col_ids_.push_back(column_id);
    updates_.push_back(update);
  }

  void UpdateSet(int32_t idx, int32_t column_id, const UPDATE& update) {
    col_ids_[idx] = column_id;
    updates_[idx] = update;
  }

  const std::vector<int32_t> &GetColIDs() const {
    return col_ids_;
  }

  const UPDATE* GetUpdates() const {
    return updates_.data();
  }

  int32_t GetBatchSize() const {
    return updates_.size();
  }

private:
  std::vector<int32_t> col_ids_;
  std::vector<UPDATE> updates_;
};

// This class is provided mainly for convience than complete functionality.
// It has no dependency to other classes in petuum ps.
template<typename UPDATE>
class DenseUpdateBatch {
public:
  // Underlying updates are not initialized
  DenseUpdateBatch(int32_t index_st,
                   int32_t num_updates):
      index_st_(index_st),
      num_updates_(num_updates),
      updates_(num_updates) { }

  UPDATE & operator[] (int32_t index) {
    int32_t idx = index - index_st_;
    return updates_[idx];
  }

  void *get_mem() {
    return updates_.data();
  }

  const void *get_mem_const() const {
    return updates_.data();
  }

  int32_t get_index_st() const {
    return index_st_;
  }

  int32_t get_num_updates() const {
    return num_updates_;
  }

private:
  int32_t index_st_;
  int32_t num_updates_;
  std::vector<UPDATE> updates_;
};

// User table is stores a lightweight pointer to ClientTable.
template<typename UPDATE>
class Table {
public:
  Table(AbstractClientTable* system_table):
    system_table_(system_table) { }

  Table(const Table &table):
    system_table_(table.system_table_) { }

  Table & operator = (const Table &table){
    system_table_ = table.system_table_;
    return *this;
  }

  void Get(int32_t row_id, int32_t col_id, AbstractRow *to,
           int32_t data_clock = -1) {
    system_table_->Get(row_id, col_id, to, data_clock);
  }

  void CopyToVector(int32_t row_id, void *vec,
                    int32_t data_clock = -1) {
    system_table_->CopyToVector(row_id, vec, data_clock);
  }

  void Inc(int32_t row_id, int32_t column_id, UPDATE update){
    system_table_->Inc(row_id, column_id, &update);
  }

  void BatchInc(int32_t row_id, const UpdateBatch<UPDATE>& update_batch){
    system_table_->BatchInc(row_id, update_batch.GetColIDs().data(),
      update_batch.GetUpdates(), update_batch.GetBatchSize());
  }

  void DenseBatchInc(int32_t row_id,
                     const DenseUpdateBatch<UPDATE> &update_batch) {
    system_table_->DenseBatchInc(row_id, update_batch.get_mem_const(),
                                 update_batch.get_index_st(),
                                 update_batch.get_num_updates());
  }

  int32_t get_row_type() const {
    return 0;
  }

private:
  AbstractClientTable* system_table_;
};
}   // namespace petuum
