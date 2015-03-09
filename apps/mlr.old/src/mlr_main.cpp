// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.10.04

#include "mlr_engine.hpp"
#include "common.hpp"
#include <petuum_ps_common/include/petuum_ps.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thread>
#include <vector>
#include <cstdint>

#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>
#include <petuum_ps_common/include/init_table_config.hpp>
#include <petuum_ps_common/include/init_table_group_config.hpp>

// Data Parameters
DEFINE_int32(num_train_data, 0, "Number of training data. Cannot exceed the "
    "number of data in train_file. 0 to use all train data.");
DEFINE_string(train_file, "", "The program expects 2 files: train_file, "
    "train_file.meta. If global_data = false, then it looks for train_file.X, "
    "train_file.X.meta, where X is the client_id.");
DEFINE_bool(global_data, false, "If true, all workers read from the same "
    "train_file. If false, append X. See train_file.");
DEFINE_string(test_file, "", "The program expects 2 files: test_file, "
    "test_file.meta, test_file must have format specified in read_format "
    "flag. All clients read test file if FLAGS_perform_test == true.");
DEFINE_int32(num_train_eval, 20, "Use the next num_train_eval train data "
    "(per thread) for intermediate eval.");
DEFINE_int32(num_test_eval, 20, "Use the first num_test_eval test data for "
    "intermediate eval. 0 for using all. The final eval will always use all "
    "test data.");
DEFINE_bool(perform_test, false, "Ignore test_file if true.");
DEFINE_bool(use_weight_file, false, "True to use init_weight_file as init");
DEFINE_string(weight_file, "", "Use this file to initialize weight. "
  "Format of the file is libsvm (see SaveWeight in MLRSGDSolver).");

// MLR Parameters
DEFINE_int32(num_epochs, 1, "Number of data sweeps.");
DEFINE_int32(num_batches_per_epoch, 10, "Since we Clock() at the end of each batch, "
    "num_batches_per_epoch is effectively the number of clocks per epoch (iteration)");
DEFINE_double(learning_rate, 0.1, "Initial step size");
DEFINE_double(decay_rate, 1, "multiplicative decay");
DEFINE_int32(num_epochs_per_eval, 10, "Number of batches per evaluation");
DEFINE_bool(sparse_weight, false, "Use sparse feature for model parameters");

// Misc
DEFINE_string(output_file_prefix, "", "Results go here.");
DEFINE_int32(num_secs_per_checkpoint, 600,
               "# of seconds between each saving to disk");

const int32_t kDenseRowFloatTypeID = 0;
//const int32_t kSparseFeatureRowFloatTypeID = 1;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  mlr::MLREngine mlr_engine;
  //STATS_APP_LOAD_DATA_BEGIN();
  mlr_engine.ReadData();
  //STATS_APP_LOAD_DATA_END();

  int32_t feature_dim = mlr_engine.GetFeatureDim();
  int32_t num_labels = mlr_engine.GetNumLabels();

  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<float> >
    (kDenseRowFloatTypeID);
  //petuum::PSTableGroup::RegisterRow<petuum::SparseFeatureRow<float> >
  //  (kSparseFeatureRowFloatTypeID);

  petuum::TableGroupConfig table_group_config;
  petuum::InitTableGroupConfig(&table_group_config, 2);

  // Use false to disallow main thread to access table API.
  petuum::PSTableGroup::Init(table_group_config, false);

  CHECK(!FLAGS_sparse_weight) << "Not yet supported!";

  // Creating weight table: one row per label.
  petuum::ClientTableConfig table_config;
  petuum::InitTableConfig(&table_config);

  table_config.table_info.row_type = kDenseRowFloatTypeID;
  table_config.table_info.row_capacity = feature_dim;
  table_config.table_info.dense_row_oplog_capacity = feature_dim;
  table_config.process_cache_capacity = num_labels;
  table_config.oplog_capacity = table_config.process_cache_capacity;
  petuum::PSTableGroup::CreateTable(kWTableID, table_config);

  // loss table.
  table_config.process_storage_type = petuum::BoundedSparse;
  table_config.table_info.row_type = kDenseRowFloatTypeID;
  table_config.table_info.row_capacity = mlr::kNumColumnsLossTable;
  table_config.table_info.dense_row_oplog_capacity
      = mlr::kNumColumnsLossTable;
  table_config.process_cache_capacity = 1000;
  table_config.oplog_capacity = table_config.process_cache_capacity;
  petuum::PSTableGroup::CreateTable(kLossTableID, table_config);

  petuum::PSTableGroup::CreateTableDone();

  LOG(INFO) << "Starting MLR with " << FLAGS_num_table_threads << " threads "
    << "on client " << FLAGS_client_id;

  std::vector<std::thread> threads(FLAGS_num_table_threads);
  for (auto& thr : threads) {
    thr = std::thread(&mlr::MLREngine::Start, std::ref(mlr_engine));
  }
  for (auto& thr : threads) {
    thr.join();
  }

  petuum::PSTableGroup::ShutDown();
  LOG(INFO) << "MLR finished and shut down!";
  return 0;
}
