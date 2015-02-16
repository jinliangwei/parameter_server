// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.03.25

#include <petuum_ps_common/include/petuum_ps.hpp>
#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>
#include <petuum_ps_common/include/init_table_group_config.hpp>
#include <petuum_ps_common/include/init_table_config.hpp>
#include "lda_engine.hpp"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thread>
#include <vector>

// Petuum Parameters


DEFINE_uint64(word_topic_table_process_cache_capacity, 100000,
              "Word topic table process cache capacity");

// LDA Parameters
DEFINE_string(doc_file, "",
    "File containing document in LibSVM format. Each document is a line.");
DEFINE_int32(num_vocabs, -1, "Number of vocabs.");
DEFINE_int32(max_vocab_id, -1, "Maximum word index, which could be different "
    "from num_vocabs if there are unused vocab indices.");
DEFINE_double(alpha, 1, "Dirichlet prior on document-topic vectors.");
DEFINE_double(beta, 0.1, "Dirichlet prior on vocab-topic vectors.");
DEFINE_int32(num_topics, 100, "Number of topics.");
DEFINE_int32(num_work_units, 1, "Number of work units");
DEFINE_int32(compute_ll_interval, 1,
    "Copmute log likelihood over local dataset on every N iterations");

// Misc
DEFINE_string(output_file_prefix, "", "LDA results.");

// No need to change the following.
DEFINE_int32(word_topic_table_id, 1, "ID within Petuum-PS");
DEFINE_int32(summary_table_id, 2, "ID within Petuum-PS");
DEFINE_int32(llh_table_id, 3, "ID within Petuum-PS");

DEFINE_int32(num_iters_per_work_unit, 1, "number of iterations per work unit");
DEFINE_int32(num_clocks_per_work_unit, 1, "number of clocks per work unit");

int32_t kSortedVectorMapRowTypeID = 1;
int32_t kDenseRowIntTypeID = 2;
int32_t kDenseRowDoubleTypeID = 3;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "LDA starts here! dense serialize = " << FLAGS_oplog_dense_serialized;

  // Read in data first to get # of vocabs in this partition.
  petuum::TableGroupConfig table_group_config;
  // doc-topic table, summary table, llh table.
  petuum::InitTableGroupConfig(&table_group_config, 3);

  petuum::PSTableGroup::RegisterRow<petuum::SortedVectorMapRow<int32_t> >
    (kSortedVectorMapRowTypeID);
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<int32_t> >
    (kDenseRowIntTypeID);
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<double> >
    (kDenseRowDoubleTypeID);

  LOG(INFO) << "Starting to Init PS";

  // Don't let main thread access table API.
  petuum::PSTableGroup::Init(table_group_config, false);

  STATS_SET_APP_DEFINED_ACCUM_SEC_NAME("sole_compute_sec");
  STATS_SET_APP_DEFINED_VEC_NAME("work_unit_sec");
  STATS_SET_APP_DEFINED_ACCUM_VAL_NAME("nonzero_entries");

  lda::LDAEngine lda_engine;
  LOG(INFO) << "Loading data";
  STATS_APP_LOAD_DATA_BEGIN();
  lda_engine.ReadData(FLAGS_doc_file);
  STATS_APP_LOAD_DATA_END();

  LOG(INFO) << "Read data done!";
  LOG(INFO) << "LDA starts here! dense serialize = " << FLAGS_oplog_dense_serialized;

  petuum::ClientTableConfig wt_table_config;
  petuum::InitTableConfig(&wt_table_config);
  wt_table_config.table_info.row_capacity = FLAGS_num_topics;
  wt_table_config.table_info.dense_row_oplog_capacity = FLAGS_num_topics;
  wt_table_config.process_cache_capacity =
      FLAGS_word_topic_table_process_cache_capacity;
  wt_table_config.thread_cache_capacity = 1;
  wt_table_config.oplog_capacity = FLAGS_word_topic_table_process_cache_capacity;
  wt_table_config.table_info.row_type = kSortedVectorMapRowTypeID;
  CHECK(petuum::PSTableGroup::CreateTable(
      FLAGS_word_topic_table_id, wt_table_config)) << "Failed to create word-topic table";

  LOG(INFO) << "Created word-topic table";

  // Summary row table (single_row).
  petuum::ClientTableConfig summary_table_config;
  petuum::InitTableConfig(&summary_table_config);
  summary_table_config.table_info.row_capacity = FLAGS_num_topics;
  summary_table_config.table_info.dense_row_oplog_capacity = FLAGS_num_topics;
  summary_table_config.process_cache_capacity = 1;
  summary_table_config.thread_cache_capacity = 1;
  summary_table_config.oplog_capacity = 1;
  summary_table_config.table_info.row_type = kDenseRowIntTypeID;
  CHECK(petuum::PSTableGroup::CreateTable(
      FLAGS_summary_table_id, summary_table_config)) << "Failed to create summary table";

  // Log-likelihood (llh) table. Single column; each column is a complete-llh.
  petuum::ClientTableConfig llh_table_config;
  petuum::InitTableConfig(&llh_table_config);
  llh_table_config.process_cache_capacity = FLAGS_num_work_units*FLAGS_num_iters_per_work_unit;
  llh_table_config.thread_cache_capacity = 1;
  llh_table_config.oplog_capacity = FLAGS_num_work_units*FLAGS_num_iters_per_work_unit;
  llh_table_config.table_info.row_capacity = 3;   // 3 columns: "iter-# llh time".
  llh_table_config.table_info.dense_row_oplog_capacity
      = llh_table_config.table_info.row_capacity;
  llh_table_config.table_info.row_type = kDenseRowDoubleTypeID;
  llh_table_config.process_cache_capacity = petuum::BoundedDense;
  llh_table_config.oplog_type = petuum::Sparse;
  CHECK(petuum::PSTableGroup::CreateTable(
      FLAGS_llh_table_id, llh_table_config)) << "Failed to create summary table";

  petuum::PSTableGroup::CreateTableDone();

  // Start LDA
  LOG(INFO) << "Starting LDA with " << FLAGS_num_table_threads << " threads "
            << "on client " << FLAGS_client_id;

  std::vector<std::thread> threads(FLAGS_num_table_threads);
  for (auto& thr : threads) {
    thr = std::thread(&lda::LDAEngine::Start, std::ref(lda_engine));
  }
  for (auto& thr : threads) {
    thr.join();
  }

  LOG(INFO) << "LDA finished!";
  petuum::PSTableGroup::ShutDown();
  LOG(INFO) << "LDA shut down!";
  return 0;
}
