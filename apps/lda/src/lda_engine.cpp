#include "lda_engine.hpp"
#include "utils.hpp"
#include "context.hpp"
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <time.h>
#include <glog/logging.h>
#include <mutex>
#include <set>
#include <dirent.h>

namespace lda {

LDAEngine::LDAEngine() :
  thread_counter_(0) {
  Context& context = Context::get_instance();
  num_threads_ = context.get_int32("num_table_threads");
  compute_ll_interval_ = context.get_int32("compute_ll_interval");
  process_barrier_.reset(new boost::barrier(num_threads_));
}

void LDAEngine::Start() {
  petuum::PSTableGroup::RegisterThread();

  // Initialize local thread data structures.
  int thread_id = thread_counter_++;

  Context& context = Context::get_instance();
  int client_id = context.get_int32("client_id");

  int32_t summary_table_id = context.get_int32("summary_table_id");
  int32_t word_topic_table_id = context.get_int32("word_topic_table_id");
  petuum::Table<int> summary_table =
    petuum::PSTableGroup::GetTableOrDie<int>(summary_table_id);
  petuum::Table<int> word_topic_table =
    petuum::PSTableGroup::GetTableOrDie<int>(word_topic_table_id);

  if (thread_id == 0)
    corpus_.RestartWorkUnit(1);
  LOG(INFO) << "Restarted work unit " << client_id;
  process_barrier_->wait();

  // Tally the word-topic counts for this thread. Use batch to reduce row
  // contentions.
  STATS_APP_INIT_BEGIN();
  petuum::UpdateBatch<int> summary_updates;
  std::unordered_map<petuum::RowId, petuum::UpdateBatch<int> > word_topic_updates;
  std::set<petuum::RowId> local_vocabs;

  auto doc_iter = corpus_.GetOneDoc();

  while (!corpus_.EndOfWorkUnit(doc_iter)) {
    for (WordOccurrenceIterator it(&(*doc_iter)); !it.End(); it.Next()) {
      //LOG(INFO) << "word = " << it.Word() << " topic = " << it.Topic();
      local_vocabs.insert(it.Word());
      word_topic_updates[it.Word()].Update(it.Topic(), 1);
      summary_updates.Update(it.Topic(), 1);
    }
    doc_iter = corpus_.GetOneDoc();
  }

  {
    std::unique_lock<std::mutex> ulock(local_vocabs_mtx_);
    for(auto it = local_vocabs.begin(); it != local_vocabs.end(); ++it) {
      local_vocabs_.insert(*it);
    }
  }

  STATS_APP_INIT_END();

  // need barrier sync to make sure all workers have added their local vocabs to
  // the shared vocab set
  process_barrier_->wait();

  if (thread_id == 0) {
    LOG(INFO) << "Done initializing topic assignments (uniform random).";
    STATS_APP_BOOTSTRAP_BEGIN();
    word_topic_table.RegisterRowSet(local_vocabs_);
    STATS_APP_BOOTSTRAP_END();
  }
  word_topic_table.WaitForBulkInit();
  LOG(INFO) << "bulk init done!";
  int num_clients = context.get_int32("num_clients");
  std::vector<petuum::RowId> row_id_vec;
  size_t total_num_rows = 0;
  word_topic_table.GetLocalRowIdSet(&row_id_vec,
                                    num_clients,
                                    num_threads_,
                                    thread_id,
                                    &total_num_rows);
  LOG(INFO) << "get local row id set done";
  FastDocSampler sampler(total_num_rows);
  if (thread_id == 0) {
    lda_stats_.reset(new LDAStats(total_num_rows));
  }
  summary_table.BatchInc(0, summary_updates);

  for (auto it = word_topic_updates.begin();
      it != word_topic_updates.end(); ++it) {
    LOG(INFO) << "batch in for " << it->first;
    word_topic_table.BatchInc(it->first, it->second);
  }
  petuum::PSTableGroup::GlobalBarrier();

  process_barrier_->wait();

  if (thread_id == 0)
    LOG(INFO) << "Bootstrap done, starts computing";

  int32_t llh_table_id = context.get_int32("llh_table_id");
  petuum::Table<double> llh_table =
      petuum::PSTableGroup::GetTableOrDie<double>(llh_table_id);
  int32_t num_work_units = context.get_int32("num_work_units");
  int32_t num_clocks_per_work_unit = context.get_int32("num_clocks_per_work_unit");
  int32_t num_iters_per_work_unit = context.get_int32("num_iters_per_work_unit");

  int num_docs = corpus_.GetNumDocs();    // # of docs in this machine.

  // ceil to ensure (num_docs_this_work_unit / num_dos_per_clock) does not
  // exceed num_clocks_per_work_unit
  int num_docs_per_clock
    = std::ceil(static_cast<float>(num_docs) * num_iters_per_work_unit
      / num_clocks_per_work_unit);
  petuum::HighResolutionTimer total_timer;  // times the computation.

  if (thread_id == 0)
    petuum::PSTableGroup::TurnOnEarlyComm();

  for (int work_unit = 1; work_unit <= num_work_units; ++work_unit) {

    STATS_APP_ACCUM_COMP_BEGIN();

    petuum::HighResolutionTimer work_unit_timer;

    if (thread_id == 0) {
      corpus_.RestartWorkUnit(num_iters_per_work_unit);
      num_tokens_this_work_unit_ = 0;
      num_docs_this_work_unit_ = 0;
    }

    process_barrier_->wait();

    // Refresh summary row cache every iteration.
    sampler.RefreshCachedSummaryRow();

    STATS_APP_DEFINED_ACCUM_SEC_BEGIN();

    int32_t num_clocks_this_work_unit = 0;
    int32_t num_iters_this_work_unit;

    int32_t local_num_tokens = 0;
    int32_t local_num_docs = 0;
    int32_t local_last_doc_ntokens = 0;

    auto doc_iter = corpus_.GetOneDoc(&num_iters_this_work_unit);

    while (!corpus_.EndOfWorkUnit(doc_iter)) {
      sampler.SampleOneDoc(&(*doc_iter));
      ++local_num_docs;
      //LOG(INFO) << "Sampled # docs = " << local_num_docs;
      local_last_doc_ntokens = doc_iter->NumTokens();

      local_num_tokens += local_last_doc_ntokens;
      int32_t num_docs_this_work_unit = ++num_docs_this_work_unit_;

      num_tokens_this_work_unit_ += doc_iter->NumTokens();

      // Manage clocking.
      int num_clocks_behind = (num_docs_this_work_unit / num_docs_per_clock)
        - num_clocks_this_work_unit;
      if (num_clocks_behind > 0) {
        for (int i = 0; i < num_clocks_behind; ++i) {
          petuum::PSTableGroup::Clock();
          STATS_APP_DEFINED_ACCUM_SEC_END();
          petuum::HighResolutionTimer refresh_timer;
          sampler.RefreshCachedSummaryRow();
          //double refresh_sec = refresh_timer.elapsed();
          //LOG(INFO) << " thread id = " << thread_id
          //        << " refresh sec = " << refresh_sec;
          STATS_APP_DEFINED_ACCUM_SEC_BEGIN();
        }
        num_clocks_this_work_unit += num_clocks_behind;
      }

      if (num_iters_this_work_unit >= 0) {
	double seconds_this_work_unit = work_unit_timer.elapsed();
	LOG(INFO)
	  << "work_unit: " << work_unit
	  << "\titer: " << num_iters_this_work_unit
	  << "\tclient " << client_id
	  << "\tthread_id " << thread_id
	  << "\ttook: " << seconds_this_work_unit << " sec"
	  << "\tthroughput: "
	  << num_tokens_this_work_unit_ / num_threads_ / seconds_this_work_unit
	  << " token/(thread*sec)";
      }
      doc_iter = corpus_.GetOneDoc(&num_iters_this_work_unit);
    }

    int num_clocks_behind = num_clocks_per_work_unit - num_clocks_this_work_unit;

    for (int i = 0; i < num_clocks_behind; ++i) {
      petuum::PSTableGroup::Clock();
    }

    num_clocks_this_work_unit += num_clocks_behind;

    process_barrier_->wait();
    STATS_APP_DEFINED_ACCUM_SEC_END();

    STATS_APP_ACCUM_OBJ_COMP_BEGIN();
    // Compute LLH.
    int ith_llh;
    if (compute_ll_interval_ != -1 && work_unit % compute_ll_interval_ == 0) {
      // Every thread compute doc-topic LLH component.
      ith_llh = work_unit / compute_ll_interval_;

      if (thread_id == 0)
	corpus_.RestartWorkUnit(1);
      process_barrier_->wait();

      auto doc_iter = corpus_.GetOneDoc();
      int doc_llh = 0.;
      while (!corpus_.EndOfWorkUnit(doc_iter)) {
        doc_llh += lda_stats_->ComputeOneDocLLH(&(*doc_iter));
        doc_iter = corpus_.GetOneDoc();
      }
      // TODO: could use thread inc
      llh_table.Inc(ith_llh, 1, doc_llh);

      // Each client takes turn to compute the last bit of word llh and
      // print LLH from to last iteration to get correct llh (llh_table should
      // have 0 staleness.
      if (ith_llh % num_clients == client_id && thread_id == 0) {
        petuum::HighResolutionTimer word_llh_timer;
        lda_stats_->ComputeWordLLH(ith_llh, row_id_vec);
        lda_stats_->ComputeWordLLHSummary(ith_llh, work_unit*num_iters_per_work_unit);
        LOG(INFO) << "word llh compute seconds = " << word_llh_timer.elapsed();
        LOG(INFO) << " LLH (work_unit " << (ith_llh - 1) * compute_ll_interval_
          << "): " << lda_stats_->PrintOneLLH(ith_llh - 1);
        lda_stats_->SetTime(ith_llh, total_timer.elapsed());
      } else {
        lda_stats_->ComputeWordLLH(ith_llh, row_id_vec);
      }
      // Keep the approximate time (the time when head thread finishes).
      process_barrier_->wait();
    }
    STATS_APP_ACCUM_OBJ_COMP_END();

    if (thread_id == 0) {
      STATS_APPEND_APP_DEFINED_VEC(work_unit_timer.elapsed());
    }

    STATS_APP_ACCUM_COMP_END();
  }   // for iter.
  if (thread_id == 0)
    petuum::PSTableGroup::TurnOffEarlyComm();

  petuum::PSTableGroup::GlobalBarrier();
  // Head thread print out the LLH.
  if (client_id == 0 && thread_id == 0) {
    int num_llh = num_work_units / compute_ll_interval_;
    LOG(INFO) << "Total time for " << num_work_units << " work units : "
      << total_timer.elapsed() << " sec.";
    LOG(INFO) << "\n" << lda_stats_->PrintLLH(num_llh);

    // Save llh to disk.
    std::string output_file_prefix = context.get_string("output_file_prefix");
    if (!output_file_prefix.empty()) {
      std::string output_file = output_file_prefix + ".llh";
      std::ofstream out_stream(output_file.c_str());
      out_stream << lda_stats_->PrintLLH(num_llh);
      out_stream.close();
    }
  }

  petuum::PSTableGroup::DeregisterThread();
}

void LDAEngine::ReadData(const char* data_path,
                         int32_t client_index,
                         size_t num_clients,
                         size_t max_num_files) {
  std::vector<std::string> file_list;
  DIR *dir = opendir(data_path);
  CHECK(dir != nullptr);
  struct dirent *ent = nullptr;
  /* print all the files and directories within directory */
  while ((ent = readdir(dir)) != nullptr) {
    auto *d_name = ent->d_name;
    if (strlen(d_name) == 0 || d_name[0] == '.' || d_name[0] == '_') continue;
    file_list.emplace_back(d_name);
    //LOG(INFO) << "got file " << d_name;
  }
  std::sort(file_list.begin(), file_list.end());
  size_t num_files = (max_num_files > 0) ? std::min(max_num_files, file_list.size()) : file_list.size();
  size_t num_files_per_client = (num_files + num_clients - 1) / num_clients;
  size_t my_num_files = ((client_index >= (num_files % num_clients)) && (num_files % num_clients > 0)) ?
                        num_files_per_client - 1 : num_files_per_client;
  size_t my_file_index_start = (client_index >= (num_files % num_clients)) ?
                               num_files_per_client * (num_files % num_clients) +
                               (num_files_per_client - 1) * (client_index - (num_files % num_clients))
                               : num_files_per_client * client_index;
  LOG(INFO) << "index start = " << my_file_index_start
            << " my_num_files = " << my_num_files;
  size_t num_docs = 0;
  size_t num_words = 0;
  for (size_t i = my_file_index_start; i < my_file_index_start + my_num_files; i++) {
    std::string file_path = std::string(data_path) + "/" + file_list[i];
    LOG(INFO) << "read file " << file_path;
    FILE *data_file = fopen(file_path.c_str(), "r");
    CHECK(data_file != nullptr);
    fseek(data_file, 0, SEEK_END);
    size_t file_size = ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    std::vector<char> char_buff(file_size + 1);
    fread(char_buff.data(), 1, file_size, data_file);
    char_buff[file_size] = '\0';
    fclose(data_file);
    char *line = strtok(char_buff.data(), "\n");
    while (line != nullptr) {
      auto *doc = corpus_.CreateAndGet();
      doc->InitAppendWord();
      char *cursor = line;
      while (*cursor != '\0') {
        int64_t word_id = strtol(cursor, &cursor, 16);
        CHECK_EQ(*cursor, ':') << "word id = " << word_id
                               << " [" << *cursor << "] "
                               << cursor;
        size_t count = strtol(cursor + 1, &cursor, 10);
        //LOG(INFO) << word_id << " " << count << "[" << *cursor << "]";
        doc->AppendWord(word_id, count);
        num_words++;
        while(isspace(*cursor) && *cursor != '\0') cursor++;
      }
      corpus_.RandomInit(doc);
      line = strtok(nullptr, "\n");
      num_docs++;
      if (num_docs % 100 == 0) {
        LOG(INFO) << "num_docs = " << num_docs;
        break;
      }
    }
  }
  LOG(INFO) << "total num_docs = " << num_docs
            << " num words = " << num_words;
}

}   // namespace lda
