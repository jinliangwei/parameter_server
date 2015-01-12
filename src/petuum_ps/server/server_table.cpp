#include <petuum_ps/server/server_table.hpp>
#include <petuum_ps_common/util/stats.hpp>
#include <iterator>
#include <vector>
#include <sstream>
#include <leveldb/db.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <time.h>

namespace petuum {

bool ServerTable::AppendTableToBuffs(
    int32_t client_id_st,
    boost::unordered_map<int32_t, RecordBuff> *buffs,
    int32_t *failed_client_id, bool resume) {

  if (resume) {
    bool append_row_suc
        = row_iter_->second.AppendRowToBuffs(
            client_id_st,
            buffs, tmp_row_buff_, curr_row_size_, row_iter_->first,
            failed_client_id);
    if (!append_row_suc)
      return false;
    ++row_iter_;
    client_id_st = 0;
  }

  size_t num_no_subscription = 0;
  for (; row_iter_ != storage_.end(); ++row_iter_) {
    if (row_iter_->second.NoClientSubscribed()) {
      num_no_subscription++;
      continue;
    }

    if (!row_iter_->second.IsDirty())
      continue;

    //LOG(INFO) << table_id_ << " " << row_iter_->first << " " << row_iter_->second.get_importance();

    STATS_SERVER_ACCUM_IMPORTANCE(table_id_, row_iter_->second.get_importance(), true);

    row_iter_->second.ResetDirty();
    ResetImportance_(&(row_iter_->second));

    curr_row_size_ = row_iter_->second.SerializedSize();
    if (curr_row_size_ > tmp_row_buff_size_) {
      delete[] tmp_row_buff_;
      tmp_row_buff_size_ = curr_row_size_;
      tmp_row_buff_ = new uint8_t[curr_row_size_];
    }
    curr_row_size_ = row_iter_->second.Serialize(tmp_row_buff_);

    bool append_row_suc = row_iter_->second.AppendRowToBuffs(
        client_id_st,
        buffs, tmp_row_buff_, curr_row_size_, row_iter_->first,
        failed_client_id);

    if (!append_row_suc) {
      return false;
    }
  }
  delete[] tmp_row_buff_;

  return true;
}

void ServerTable::SortCandidateVectorRandom(
    std::vector<CandidateServerRow> *candidate_row_vector) {
  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle((*candidate_row_vector).begin(),
               (*candidate_row_vector).end(), g);
}

void ServerTable::SortCandidateVectorImportance(
    std::vector<CandidateServerRow> *candidate_row_vector) {

  std::sort((*candidate_row_vector).begin(),
            (*candidate_row_vector).end(),
            [] (const CandidateServerRow &row1, const CandidateServerRow &row2)
            {
              if (row1.server_row_ptr->get_importance() ==
                  row2.server_row_ptr->get_importance()) {
                return row1.row_id < row2.row_id;
              } else {
                return (row1.server_row_ptr->get_importance() >
                    row2.server_row_ptr->get_importance());
              }
            });
}

void ServerTable::GetPartialTableToSend(
    std::vector<std::pair<int32_t, ServerRow*> > *rows_to_send,
    boost::unordered_map<int32_t, size_t> *client_size_map) {

  size_t num_rows_threshold = table_info_.server_push_row_upper_bound;

  size_t num_candidate_rows
      = num_rows_threshold * GlobalContext::get_row_candidate_factor();

  size_t storage_size = storage_.size();

  if (num_candidate_rows > storage_size)
    num_candidate_rows = storage_size;

  double select_prob = double(num_candidate_rows) / double(storage_size);
  std::mt19937 generator(time(NULL)); // max 4.2 billion
  std::uniform_real_distribution<> uniform_dist(0, 1);

  std::vector<CandidateServerRow> candidate_row_vector;

    for (auto &row_pair : storage_) {
      if (row_pair.second.NoClientSubscribed()
          || !row_pair.second.IsDirty())
        continue;

      double prob = uniform_dist(generator);
      if (prob <= select_prob)
        candidate_row_vector.push_back(CandidateServerRow(row_pair.first, &(row_pair.second)));
      if (candidate_row_vector.size() == num_candidate_rows) break;
  }

  SortCandidateVector_(&candidate_row_vector);

  for (auto vec_iter = candidate_row_vector.begin();
       vec_iter != candidate_row_vector.end(); vec_iter++) {

    (*rows_to_send).push_back(
        std::make_pair(vec_iter->row_id, vec_iter->server_row_ptr));

    vec_iter->server_row_ptr->AccumSerializedSizePerClient(client_size_map);

    if ((*rows_to_send).size() >= num_rows_threshold)
      break;
  }
}

void ServerTable::AppendRowsToBuffsPartial(
    boost::unordered_map<int32_t, RecordBuff> *buffs,
    const std::vector<std::pair<int32_t, ServerRow*> > &rows_to_send) {

  tmp_row_buff_ = new uint8_t[tmp_row_buff_size_];

  for (const auto &row_pair : rows_to_send) {
    int32_t row_id = row_pair.first;
    ServerRow *row = row_pair.second;
    if (row->NoClientSubscribed()
        || !row->IsDirty()) {
      LOG(FATAL) << "row " << row_id << " should not be sent!";
    }

    STATS_SERVER_ACCUM_IMPORTANCE(table_id_, row->get_importance(), true);

    row->ResetDirty();
    ResetImportance_(row);

    curr_row_size_ = row->SerializedSize();
    if (curr_row_size_ > tmp_row_buff_size_) {
      delete[] tmp_row_buff_;
      tmp_row_buff_size_ = curr_row_size_;
      tmp_row_buff_ = new uint8_t[curr_row_size_];
    }

    curr_row_size_ = row->Serialize(tmp_row_buff_);

    row->AppendRowToBuffs(
        buffs, tmp_row_buff_, curr_row_size_, row_id);
  }

  delete[] tmp_row_buff_;
}

void ServerTable::MakeSnapShotFileName(
    const std::string &snapshot_dir,
    int32_t server_id, int32_t table_id, int32_t clock,
    std::string *filename) const {

  std::stringstream ss;
  ss << snapshot_dir << "/server_table" << ".server-" << server_id
     << ".table-" << table_id << ".clock-" << clock
     << ".db";
  *filename = ss.str();
}

void ServerTable::TakeSnapShot(
    const std::string &snapshot_dir,
    int32_t server_id, int32_t table_id, int32_t clock) const {

  std::string db_name;
  MakeSnapShotFileName(snapshot_dir, server_id, table_id, clock, &db_name);

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, db_name, &db);

  uint8_t *mem_buff = new uint8_t[512];
  int32_t buff_size = 512;
  for (auto row_iter = storage_.begin(); row_iter != storage_.end();
       row_iter++) {
    int32_t serialized_size = row_iter->second.SerializedSize();
    if (buff_size < serialized_size) {
      delete[] mem_buff;
      buff_size = serialized_size;
      mem_buff = new uint8_t[buff_size];
    }
    row_iter->second.Serialize(mem_buff);
    leveldb::Slice key(reinterpret_cast<const char*>(&(row_iter->first)),
                       sizeof(int32_t));
    leveldb::Slice value(reinterpret_cast<const char*>(mem_buff),
                         serialized_size);
    db->Put(leveldb::WriteOptions(), key, value);
  }
  delete[] mem_buff;
  delete db;
}

void ServerTable::ReadSnapShot(const std::string &resume_dir,
                               int32_t server_id, int32_t table_id, int32_t clock) {

  std::string db_name;
  MakeSnapShotFileName(resume_dir, server_id, table_id, clock, &db_name);

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, db_name, &db);

  leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    int32_t row_id = *(reinterpret_cast<const int32_t*>(it->key().data()));
        const uint8_t *row_data
            = reinterpret_cast<const uint8_t*>(it->value().data());
        size_t row_data_size = it->value().size();

        int32_t row_type = table_info_.row_type;
            AbstractRow *abstract_row
                = ClassRegistry<AbstractRow>::GetRegistry().CreateObject(row_type);
            abstract_row->Deserialize(row_data, row_data_size);

            storage_.insert(std::make_pair(row_id, ServerRow(abstract_row)));
            VLOG(0) << "ReadSnapShot, row_id = " << row_id;
  }
  delete it;
  delete db;
}
}
