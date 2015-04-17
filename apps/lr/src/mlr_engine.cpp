// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.10.04

#include "mlr_engine.hpp"
#include "mlr_sgd_solver.hpp"
#include "lr_sgd_solver.hpp"
#include "abstract_mlr_sgd_solver.hpp"
#include "common.hpp"
#include <string>
#include <cmath>
#include <vector>
#include <cstdio>
#include <glog/logging.h>
#include <ml/include/ml.hpp>
#include <cstdint>
#include <fstream>

#include <petuum_ps_common/include/petuum_ps.hpp>
#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>

namespace mlr {

MLREngine::MLREngine() :
    num_train_data_(FLAGS_num_train_data),
    feature_dim_(FLAGS_feature_dim),
    num_labels_(FLAGS_num_labels),
    num_train_eval_(FLAGS_num_train_eval),
    perform_test_(FLAGS_perform_test),
    thread_counter_(0),
    process_barrier_(new boost::barrier(FLAGS_num_table_threads)) {

  LOG(INFO) << "num_train_data = " << num_train_data_;
}

MLREngine::~MLREngine() {
  for (auto p : train_features_) {
    delete p;
  }
  for (auto p : test_features_) {
    delete p;
  }
}

void MLREngine::ReadData() {
  std::string read_format = FLAGS_data_format;
  bool feature_one_based = FLAGS_feature_one_based;
  bool label_one_based = FLAGS_label_one_based;
  bool snappy_compressed = FLAGS_snappy_compressed;

  std::string train_file
      = FLAGS_train_file
      + (FLAGS_global_data ? "" :
         "." + std::to_string(FLAGS_client_id));

  LOG(INFO) << "Reading train file: " << train_file;
  if (read_format == "bin") {
    petuum::ml::ReadDataLabelBinary(
        train_file, feature_dim_, num_train_data_,
        &train_features_, &train_labels_);
    if (perform_test_) {
      LOG(INFO) << "Reading test file: " << FLAGS_test_file;
      petuum::ml::ReadDataLabelBinary(
          FLAGS_test_file, feature_dim_,
          num_test_data_, &test_features_, &test_labels_);
    }
  } else if (read_format == "libsvm") {
    LOG(INFO) << "train_file = " << train_file
              << " feature_one_based = " << feature_one_based
              << " label_one_based = " << label_one_based
              << " snappy_compressed = " << snappy_compressed
              << " num_data = " << num_train_data_;

    petuum::ml::ReadDataLabelLibSVM(
        train_file, feature_dim_, num_train_data_,
        &train_features_, &train_labels_,
        feature_one_based, label_one_based, snappy_compressed);

    if (perform_test_) {
      LOG(INFO) << "Reading test file: " << FLAGS_test_file;
      petuum::ml::ReadDataLabelLibSVM(
          FLAGS_test_file, feature_dim_,
          num_test_data_, &test_features_, &test_labels_,
          feature_one_based, label_one_based, snappy_compressed);
    }
  } else if (read_format == "sparse_feature_binary") {
    petuum::ml::ReadDataLabelSparseFeatureBinary(
        train_file, feature_dim_, num_train_data_,
        &train_features_, &train_labels_,
        feature_one_based, label_one_based, snappy_compressed);
    if (perform_test_) {
      LOG(INFO) << "Reading test file: " << FLAGS_test_file;
      petuum::ml::ReadDataLabelSparseFeatureBinary(
          FLAGS_test_file, feature_dim_,
          num_test_data_, &test_features_, &test_labels_,
          feature_one_based, label_one_based, snappy_compressed);
    }
  }
}

void MLREngine::Start() {
  petuum::PSTableGroup::RegisterThread();

  // Initialize local thread data structures.
  int thread_id = thread_counter_++;

  int client_id = FLAGS_client_id;
  int num_clients = FLAGS_num_clients;
  int num_threads = FLAGS_num_table_threads;
  int num_epochs = FLAGS_num_epochs;
  int num_batches_per_epoch = FLAGS_num_batches_per_epoch;
  int loss_table_staleness = FLAGS_table_staleness;
  int num_epochs_per_eval = FLAGS_num_epochs_per_eval;
  bool global_data = FLAGS_global_data;
  int num_test_eval = FLAGS_num_test_eval;

  std::vector<float> predict_buff(num_labels_);

  if (num_test_eval == 0) {
    num_test_eval = num_test_data_;
  }

  if (thread_id == 0) {
    loss_table_ =
      petuum::PSTableGroup::GetTableOrDie<float>(kLossTableID);
    w_table_ =
      petuum::PSTableGroup::GetTableOrDie<float>(kWTableID);

    int num_rows = num_labels_;

    if (num_labels_ == 2) {
      LOG(INFO) << "num_labels = " << num_labels_;
      num_rows = std::ceil(float(feature_dim_) / FLAGS_w_table_num_cols);
    }

    LOG(INFO) << "num_rows = " << num_rows;

    for (int i = 0; i < num_rows; ++i) {
      w_table_.GetAsyncForced(i);
    }

    w_table_.WaitPendingAsyncGet();
    LOG(INFO) << "Bootstrap done!";
  }

  // Barrier to ensure w_table_ and loss_table_ is initialized.
  process_barrier_->wait();

  std::unique_ptr<AbstractMLRSGDSolver> mlr_solver;
  if (num_labels_ == 2) {
    // Create LR sgd solver.
    LRSGDSolverConfig solver_config;
    solver_config.feature_dim = feature_dim_;
    solver_config.sparse_data =
        (FLAGS_data_format == "libsvm"
         || FLAGS_data_format == "sparse_feature_binary");
    solver_config.w_table = w_table_;
    solver_config.lambda = FLAGS_lambda;
    solver_config.w_table_num_cols = FLAGS_w_table_num_cols;
    mlr_solver.reset(new LRSGDSolver(solver_config));
  } else {
    // Create MLR sgd solver.
    MLRSGDSolverConfig solver_config;
    solver_config.feature_dim = feature_dim_;
    solver_config.num_labels = num_labels_;
    solver_config.sparse_data =
        (FLAGS_data_format == "libsvm"
        || FLAGS_data_format == "sparse_feature_binary");
    solver_config.sparse_weight = FLAGS_sparse_weight;
    solver_config.w_table = w_table_;
    mlr_solver.reset(new MLRSGDSolver(solver_config));
  }
  mlr_solver->ReadFreshParams();

  petuum::HighResolutionTimer total_timer;
  LOG(INFO) << "clients = " << num_clients
            << " threads = " << num_threads
            << " num_batches_per_epochs = " << num_batches_per_epoch
            << " num_data = " << num_train_data_;
  petuum::ml::WorkloadManagerConfig workload_mgr_config;
  workload_mgr_config.thread_id = thread_id;
  workload_mgr_config.client_id = client_id;
  workload_mgr_config.num_clients = num_clients;
  workload_mgr_config.num_threads = num_threads;
  workload_mgr_config.num_batches_per_epoch = num_batches_per_epoch;
  workload_mgr_config.num_data = num_train_data_;
  workload_mgr_config.global_data = global_data;
  petuum::ml::WorkloadManager workload_mgr(workload_mgr_config);
  // For training error.
  petuum::ml::WorkloadManager workload_mgr_train_error(workload_mgr_config);

  LOG_IF(INFO, client_id == 0 && thread_id == 0)
    << "Batch size: " << workload_mgr.GetBatchSize()
    << " num_train_data = " << num_train_data_;

  petuum::ml::WorkloadManagerConfig test_workload_mgr_config;
  test_workload_mgr_config.thread_id = thread_id;
  test_workload_mgr_config.client_id = client_id;
  test_workload_mgr_config.num_clients = num_clients;
  test_workload_mgr_config.num_threads = num_threads;
  test_workload_mgr_config.num_batches_per_epoch = 1;
  // Need to set num_data to non-zero to avoid problem in WorkloadManager.
  test_workload_mgr_config.num_data = perform_test_ ? num_test_data_ : 10000;
  // test set is always globa (duplicated on all clients).
  test_workload_mgr_config.global_data = true;
  petuum::ml::WorkloadManager test_workload_mgr(test_workload_mgr_config);

  // It's reset after every check-pointing (saving to disk).
  petuum::HighResolutionTimer checkpoint_timer;

  int32_t eval_counter = 0;
  int32_t batch_counter = 0;
  STATS_APP_ACCUM_COMP_BEGIN();

  petuum::PSTableGroup::TurnOnEarlyComm();
  for (int epoch = 0; epoch < num_epochs; ++epoch) {
    if (client_id == 0 && thread_id == 0)
      LOG(INFO) << "epoch ... " << epoch;

    // refresh cache if it is not refreshed from computing obj value
    if (epoch == 0
        || ((epoch - 1) % num_epochs_per_eval != 0)) {
      STATS_APP_ACCUM_COMP_END();
      mlr_solver->ReadFreshParams();
      STATS_APP_ACCUM_COMP_BEGIN();
    }

    workload_mgr.Restart();
    while (!workload_mgr.IsEnd()) {
      int32_t data_idx = workload_mgr.GetDataIdxAndAdvance();
      if (client_id == 0 && thread_id == 0)
        LOG_EVERY_N(INFO, 10000) << "processed " << google::COUNTER;

      mlr_solver->SingleDataSGD(
        *train_features_[data_idx],
        train_labels_[data_idx], 0);

      if (workload_mgr.IsEndOfBatch()) {
        if (!workload_mgr.IsEnd()) {
          mlr_solver->ApplyUpdates();
          if (FLAGS_clock_per_minibatch) {
            petuum::PSTableGroup::Clock();
          }
          STATS_APP_ACCUM_COMP_END();
          mlr_solver->ReadFreshParams();
          STATS_APP_ACCUM_COMP_BEGIN();
        }
        ++batch_counter;
      }
    }
    CHECK_EQ(0, batch_counter % num_batches_per_epoch);
    LOG(INFO) << "CK1";
    mlr_solver->ApplyUpdates();
    petuum::PSTableGroup::Clock();
    LOG(INFO) << "CK2";

    if (epoch % num_epochs_per_eval == 0) {
      LOG(INFO) << "CK3";
      STATS_APP_ACCUM_COMP_END();
      mlr_solver->ReadFreshParams();
      LOG(INFO) << "CK4";

      petuum::HighResolutionTimer eval_timer;
      ComputeTrainError(mlr_solver.get(), &workload_mgr_train_error,
                        num_train_eval_, eval_counter, &predict_buff);

      if (thread_id == 0 && client_id == 0) {
        // Add reg loss.
        loss_table_.Inc(eval_counter, kColIdxLossTableRegLoss,
            mlr_solver->EvaluateL2RegLoss());
      }

      if (perform_test_) {
        ComputeTestError(mlr_solver.get(), &test_workload_mgr,
            num_test_eval, eval_counter, &predict_buff);
      }
      if (client_id == 0 && thread_id == 0) {
        petuum::UpdateBatch<float> update_batch(3);
        update_batch.UpdateSet(0, kColIdxLossTableEpoch, epoch + 1);
        update_batch.UpdateSet(1, kColIdxLossTableBatch, batch_counter);
        update_batch.UpdateSet(2, kColIdxLossTableTime,
                               total_timer.elapsed() - eval_timer.elapsed());
        loss_table_.BatchInc(eval_counter, update_batch);

        if (eval_counter > loss_table_staleness) {
          // Print the last eval info to overcome staleness.
          LOG(INFO) << PrintOneEval(eval_counter - loss_table_staleness - 1);
        }
        LOG(INFO) << "Eval #" << eval_counter << " finished in "
          << eval_timer.elapsed();
      }
      ++eval_counter;
    }
    STATS_APP_ACCUM_COMP_BEGIN();
  }
  petuum::PSTableGroup::TurnOffEarlyComm();
  STATS_APP_ACCUM_COMP_END();
  petuum::PSTableGroup::GlobalBarrier();

  // Use all the train data in the last training error eval.
  ComputeTrainError(mlr_solver.get(), &workload_mgr_train_error,
                    num_train_data_, eval_counter, &predict_buff);

  if (thread_id == 0 && client_id == 0) {
    // Add reg loss.
    loss_table_.Inc(eval_counter, kColIdxLossTableRegLoss,
                    mlr_solver->EvaluateL2RegLoss());
  }

  if (perform_test_) {
    // Use the whole test set in the end.
    ComputeTestError(mlr_solver.get(), &test_workload_mgr,
                     num_test_data_, eval_counter, &predict_buff);
  }

  petuum::PSTableGroup::GlobalBarrier();
  if (client_id == 0 && thread_id == 0) {
    loss_table_.Inc(eval_counter, kColIdxLossTableEpoch, num_epochs);
    loss_table_.Inc(eval_counter, kColIdxLossTableBatch,
                    batch_counter);
    loss_table_.Inc(eval_counter, kColIdxLossTableTime,
                    total_timer.elapsed());

    LOG(INFO) << std::endl << PrintAllEval(eval_counter);
    LOG(INFO) << "Final eval: " << PrintOneEval(eval_counter);
    SaveLoss(eval_counter);
  }
  petuum::PSTableGroup::DeregisterThread();
}

void MLREngine::ComputeTrainError(
    AbstractMLRSGDSolver* mlr_solver,
    petuum::ml::WorkloadManager* workload_mgr,
    int32_t num_data_to_use,
    int32_t ith_eval,
    std::vector<float> *predict_buff) {
  float total_zero_one_loss = 0.;
  float total_entropy_loss = 0.;

  workload_mgr->Restart();
  int num_total = 0;
  while (!workload_mgr->IsEnd() && num_total < num_data_to_use) {
    int32_t data_idx = workload_mgr->GetDataIdxAndAdvance();
    mlr_solver->Predict(*(train_features_[data_idx]), predict_buff);
    total_zero_one_loss += mlr_solver->ZeroOneLoss(*predict_buff,
        train_labels_[data_idx]);
    total_entropy_loss += mlr_solver->CrossEntropyLoss(*predict_buff,
        train_labels_[data_idx]);
    ++num_total;
  }

  loss_table_.Inc(ith_eval, kColIdxLossTableZeroOneLoss,
                  total_zero_one_loss);
  loss_table_.Inc(ith_eval, kColIdxLossTableEntropyLoss,
                  total_entropy_loss);
  loss_table_.Inc(ith_eval, kColIdxLossTableNumEvalTrain,
                  static_cast<float>(num_total));
}

void MLREngine::ComputeTestError(
    AbstractMLRSGDSolver* mlr_solver,
    petuum::ml::WorkloadManager* test_workload_mgr,
    int32_t num_data_to_use, int32_t ith_eval,
    std::vector<float> *predict_buff) {
  test_workload_mgr->Restart();
  int32_t num_error = 0;
  int32_t num_total = 0;
  int i = 0;
  while (!test_workload_mgr->IsEnd() && i < num_data_to_use) {
    int32_t data_idx = test_workload_mgr->GetDataIdxAndAdvance();
    mlr_solver->Predict(*test_features_[data_idx], predict_buff);
    num_error += mlr_solver->ZeroOneLoss(*predict_buff, test_labels_[data_idx]);
    ++num_total;
    ++i;
  }
  loss_table_.Inc(ith_eval, kColIdxLossTableTestZeroOneLoss,
                  static_cast<float>(num_error));
  loss_table_.Inc(ith_eval, kColIdxLossTableNumEvalTest,
                  static_cast<float>(num_total));
}

std::string MLREngine::PrintOneEval(int32_t ith_eval) {
  std::stringstream output;
  petuum::RowAccessor row_acc;
  const auto& loss_row = loss_table_.Get<petuum::DenseRow<float> >(ith_eval, &row_acc);
  std::string test_info;
  if (perform_test_) {
    CHECK_LT(0, static_cast<int>(loss_row[kColIdxLossTableNumEvalTest]));
    std::string test_zero_one_str =
      std::to_string(loss_row[kColIdxLossTableTestZeroOneLoss] /
          loss_row[kColIdxLossTableNumEvalTest]);
    std::string num_test_str =
      std::to_string(static_cast<int>(loss_row[kColIdxLossTableNumEvalTest]));
    test_info += "test-0-1: " + test_zero_one_str
      + " num-test-used: " + num_test_str;
  }
  CHECK_LT(0, static_cast<int>(loss_row[kColIdxLossTableNumEvalTrain]));
  output << loss_row[kColIdxLossTableEpoch] << " "
    << loss_row[kColIdxLossTableBatch] << " "
    << "train-0-1: " << loss_row[kColIdxLossTableZeroOneLoss] /
    loss_row[kColIdxLossTableNumEvalTrain] << " "
    << "train-entropy: " << loss_row[kColIdxLossTableEntropyLoss] /
    loss_row[kColIdxLossTableNumEvalTrain] << " "
    << "train-obj: " << loss_row[kColIdxLossTableEntropyLoss] /
    loss_row[kColIdxLossTableNumEvalTrain]
    + loss_row[kColIdxLossTableRegLoss] << " "
    << "num-train-used: " << loss_row[kColIdxLossTableNumEvalTrain] << " "
    << test_info << " "
    << "time: " << loss_row[kColIdxLossTableTime] << std::endl;
  return output.str();
}

std::string MLREngine::PrintAllEval(int32_t up_to_ith_eval) {
  std::stringstream output;
  if (perform_test_) {
    output << "Epoch Batch Train-0-1 Train-Entropy Train-Obj Num-Train-Used Test-0-1 "
      << "Num-Test-Used Time" << std::endl;
  } else {
    output << "Epoch Batch Train-0-1 Train-Entropy Train-Obj Num-Train-Used "
      << "Time" << std::endl;
  }
  for (int i = 0; i <= up_to_ith_eval; ++i) {
    petuum::RowAccessor row_acc;
    const auto &loss_row = loss_table_.Get<petuum::DenseRow<float> >(i, &row_acc);
    std::string test_info;
    if (perform_test_) {
      CHECK_LT(0, static_cast<int>(loss_row[kColIdxLossTableNumEvalTest]));
      std::string test_zero_one_str =
        std::to_string(loss_row[kColIdxLossTableTestZeroOneLoss] /
            loss_row[kColIdxLossTableNumEvalTest]);
      std::string num_test_str =
        std::to_string(static_cast<int>(loss_row[kColIdxLossTableNumEvalTest]));
      test_info += test_zero_one_str + " " + num_test_str;
    }
    CHECK_LT(0, static_cast<int>(loss_row[kColIdxLossTableNumEvalTrain]));
    output << loss_row[kColIdxLossTableEpoch] << " "
      << loss_row[kColIdxLossTableBatch] << " "
      << loss_row[kColIdxLossTableZeroOneLoss] /
      loss_row[kColIdxLossTableNumEvalTrain] << " "
      << loss_row[kColIdxLossTableEntropyLoss] /
      loss_row[kColIdxLossTableNumEvalTrain] << " "
      << loss_row[kColIdxLossTableEntropyLoss] /
      loss_row[kColIdxLossTableNumEvalTrain]
      + loss_row[kColIdxLossTableRegLoss] << " "
      << loss_row[kColIdxLossTableNumEvalTrain] << " "
      << test_info << " "
      << loss_row[kColIdxLossTableTime] << std::endl;
  }
  return output.str();
}

void MLREngine::SaveLoss(int32_t up_to_ith_eval) {
  CHECK(!FLAGS_output_file_prefix.empty());
  std::string output_filename = FLAGS_output_file_prefix + ".loss";
  petuum::HighResolutionTimer disk_output_timer;
  std::ofstream out_stream(output_filename);
  out_stream << GetExperimentInfo();
  out_stream << PrintAllEval(up_to_ith_eval);
  out_stream.close();
  LOG(INFO) << "Loss up to " << up_to_ith_eval << " (exclusive) is saved to "
    << output_filename << " in " << disk_output_timer.elapsed();
}

std::string MLREngine::GetExperimentInfo() const {
  std::stringstream ss;
  ss << "Train set: " << FLAGS_train_file << std::endl
    << "feature_dim: " << feature_dim_ << std::endl
    << "num_labels: " << num_labels_ << std::endl
    << "num_train_data: " << num_train_data_ << std::endl
    << "num_test_data: " << num_test_data_ << std::endl
    << "num_epochs: " << FLAGS_num_epochs << std::endl
    << "num_batches_per_epoch: " << FLAGS_num_batches_per_epoch << std::endl
    << "lambda: " << FLAGS_lambda << std::endl
    << "num_epochs_per_eval: " << FLAGS_num_epochs_per_eval << std::endl
    << "staleness: " << FLAGS_table_staleness << std::endl
    << "num_clients: " << FLAGS_num_clients << std::endl
    << "num_table_threads: " << FLAGS_num_table_threads << std::endl
    << "num_comm_channels_per_client: "
    << FLAGS_num_comm_channels_per_client << std::endl;
  return ss.str();
}


}  // namespace mlr
