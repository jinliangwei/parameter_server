#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <limits>
#include <random>
#include <set>
#include <thread>
#include <boost/thread/barrier.hpp>

#include <petuum_ps_common/include/petuum_ps.hpp>
#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>
#include <petuum_ps_common/include/init_table_config.hpp>
#include <petuum_ps_common/include/init_table_group_config.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

// Command-line flags
DEFINE_double(init_step_size, 0.5, "Initial stochastic gradient descent "
    "step size");
DEFINE_double(step_dec, 0.9, "Step size is "
    "init_step_size * (step_dec)^iter.");
DEFINE_bool(use_step_dec, false, "False to use sqrt instead of "
    "multiplicative decay.");
DEFINE_double(lambda, 0.001, "L2 regularization strength.");
DEFINE_int32(K, 100, "Factorization rank");

DEFINE_string(datafile, "", "Input sparse matrix");
DEFINE_string(output_prefix, "", "Output results with this prefix. "
    "If the prefix is X, the output will be called X.L and X.R");
DEFINE_int32(num_iterations, 100, "Number of iterations");
DEFINE_int32(num_clocks_per_iter, 1,
             "clock num_clocks_per_iter in each SGD sweep.");
DEFINE_int32(num_clocks_per_eval, 1, "# of clocks between "
             "each objective evaluation");
DEFINE_int32(num_worker_threads, 1, "Number of worker threads per client");

DEFINE_bool(output_LR, false, "Save L and R matrices to disk or not.");

DEFINE_uint64(M_cache_size, 10000000, "Process cache size for the R table.");
DEFINE_uint64(M_client_send_oplog_upper_bound, 100, "M client upper bound");

// Data variables
size_t X_num_rows, X_num_cols; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)
std::vector<int32_t> X_row; // Row index of each nonzero entry in the data matrix
std::vector<int32_t> X_col; // Column index of each nonzero entry in the data matrix
std::vector<float> X_val; // Value of each nonzero entry in the data matrix
std::vector<int32_t> X_partition_starts;

int kLossTableColIdxClock = 0;
int kLossTableColIdxComputeTime = 1;
int kLossTableColIdxL2Loss = 2;
int kLossTableColIdxL2RegLoss = 3;
int kLossTableColIdxComputeEvalTime = 4;
int kLossTableColIdxIter = 5;

void ReadBinaryMatrix(const std::string &filename, int32_t partition_id) {
  std::string bin_file = filename + "." + std::to_string(partition_id);

  FILE *bin_input = fopen(bin_file.c_str(), "rb");
  CHECK(bin_input != 0) << "failed to read " << bin_file;

  uint64_t num_nnz_this_partition = 0,
        num_rows_this_partition = 0,
        num_cols_this_partition = 0;
  size_t read_size = fread(&num_nnz_this_partition, sizeof(uint64_t), 1, bin_input);
  CHECK_EQ(read_size, 1);
  read_size = fread(&num_rows_this_partition, sizeof(uint64_t), 1, bin_input);
  CHECK_EQ(read_size, 1);
  read_size = fread(&num_cols_this_partition, sizeof(uint64_t), 1, bin_input);
  CHECK_EQ(read_size, 1);
  LOG(INFO) << "num_nnz_this_partition: " << num_nnz_this_partition
    << " num_rows_this_partition: " << num_rows_this_partition
    << " num_cols_this_partition: " << num_cols_this_partition;

  X_row.resize(num_nnz_this_partition);
  X_col.resize(num_nnz_this_partition);
  X_val.resize(num_nnz_this_partition);

  read_size = fread(X_row.data(), sizeof(int32_t), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);
  read_size = fread(X_col.data(), sizeof(int32_t), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);
  read_size = fread(X_val.data(), sizeof(float), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);

  X_num_rows = num_rows_this_partition;
  X_num_cols = num_cols_this_partition;
  LOG(INFO) << "partition = " << partition_id
            << " #row = " << X_num_rows
            << " #cols = " << X_num_cols;
}

void PartitionWorkLoad(int32_t num_local_workers) {
  size_t num_nnz = X_val.size();
  size_t num_nnz_per_partition = num_nnz / num_local_workers;

  X_partition_starts.resize(num_local_workers);
  int64_t partition_start = 0;
  for (int32_t i = 0; i < num_local_workers; ++i) {
    X_partition_starts[i] = partition_start;

    int64_t partition_end = (i == num_local_workers - 1) ?
                            num_nnz - 1 :
                            partition_start + num_nnz_per_partition;

    int end_row_id = X_row[partition_end];
    CHECK(partition_end < num_nnz) << "i = " << i;

    if (i != num_local_workers - 1) {
      while (partition_end < num_nnz
             && end_row_id == X_row[partition_end]) {
        ++partition_end;
      }

      CHECK(partition_end < num_nnz) << "There's empty bin " << i;
      partition_end--;
      partition_start = partition_end + 1;
    }
  }
}

// Returns the number of workers threads across all clients
int get_total_num_workers() {
  return FLAGS_num_clients * FLAGS_num_worker_threads;
}
// Returns the global thread ID of this worker
int get_global_worker_id(int thread_id) {
  return (FLAGS_client_id * FLAGS_num_worker_threads) + thread_id;
}

void InitLTable(size_t row_capacity,
                int32_t local_worker_id,
                std::vector<std::vector<float> > *L_table,
                size_t *row_id_offset) {
  int row_id_start = X_row[X_partition_starts[local_worker_id]];
  int row_id_end = (local_worker_id == X_partition_starts.size() - 1) ?
                   X_row.back() :
                   X_row[X_partition_starts[local_worker_id + 1] - 1];

  size_t L_table_size = row_id_end - row_id_start + 1;
  L_table->resize(L_table_size, std::vector<float>(row_capacity, 0.0));
  *row_id_offset = row_id_start;
}

// Performs stochastic gradient descent on X_row[a], X_col[a], X_val[a].
void SgdElement(
    int64_t a, int iter, float step_size, int global_worker_id,
    std::vector<std::vector<float> > &L_table,
    size_t L_row_id_offset,
    petuum::Table<float>& R_table,
    petuum::Table<float>& loss_table,
    std::vector<float>* Rj_cache) {
  // Let i = X_row[a], j = X_col[a], and X(i,j) = X_val[a]
  const int i = X_row[a];
  const int j = X_col[a];
  const float Xij = X_val[a];
  // Read R(:,j) from Petuum PS
  {
    petuum::RowAccessor Rj_acc;
    const auto& Rj_row = R_table.Get<petuum::DenseRow<float> >(j, &Rj_acc);
    Rj_row.CopyToVector(Rj_cache);
    // Release the accessor.
  }
  auto& Rj = *Rj_cache;
  auto& Li = L_table[i - L_row_id_offset];

  // Compute L(i,:) * R(:,j)
  float LiRj = 0.0;
  for (int k = 0; k < FLAGS_K; ++k) {
    LiRj += Li[k] * Rj[k];
  }

  // Now update L(i,:) and R(:,j) based on the loss function at X(i,j).
  // The loss function at X(i,j) is ( X(i,j) - L(i,:)*R(:,j) )^2.
  //
  // The gradient w.r.t. L(i,k) is -2*X(i,j)R(k,j) + 2*L(i,:)*R(:,j)*R(k,j).
  // The gradient w.r.t. R(k,j) is -2*X(i,j)L(i,k) + 2*L(i,:)*R(:,j)*L(i,k).
  petuum::DenseUpdateBatch<float> Rj_update(0, FLAGS_K);
  float grad_coeff = -2 * (Xij - LiRj);
  float regularization_coeff = FLAGS_lambda * 2;
  for (int k = 0; k < FLAGS_K; ++k) {
    float gradient = 0.0;
    // Compute update for L(i,k)
    gradient = grad_coeff * Rj[k] + regularization_coeff * Li[k];
    Li[k] += -gradient * step_size;
    // Compute update for R(k,j)
    gradient = grad_coeff * Li[k] + regularization_coeff * Rj[k];
    Rj_update[k] = -gradient * step_size;
  }
  R_table.DenseBatchInc(j, Rj_update);
}

void InitMF(
    std::vector<std::vector<float> >& L_table,
    petuum::Table<float>& R_table,
    int col_begin, int col_end) {
  // Create a uniform RNG in the range (-1,1)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dist(0, 0.1);

  for (auto &L_row : L_table) {
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen);
      L_row[k] = init_val;
    }
  }

  for (int j = col_begin; j < col_end; ++j) {
    petuum::DenseUpdateBatch<float> R_updates(0, FLAGS_K);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen);
      R_updates[k] = init_val;
    }
    R_table.DenseBatchInc(j, R_updates);
  }
}

std::string GetExperimentInfo() {
  std::stringstream ss;
  ss << "Rank(K) = " << FLAGS_K << std::endl
    << "Matrix dimensions: " << X_num_rows << " by " << X_num_cols << std::endl
    << "# non-missing entries: " << X_row.size() << std::endl
    << "num_iterations = " << FLAGS_num_iterations << std::endl
    << "num_clients = " << FLAGS_num_clients << std::endl
    << "num_worker_threads = " << FLAGS_num_worker_threads << std::endl
    << "num_comm_channels_per_client = "
    << FLAGS_num_comm_channels_per_client << std::endl
    << "num_clocks_per_iter = " << FLAGS_num_clocks_per_iter << std::endl
    << "num_clocks_per_eval = " << FLAGS_num_clocks_per_eval << std::endl
    << "M_cache_size = " << FLAGS_M_cache_size << std::endl
    << "staleness = " << FLAGS_table_staleness << std::endl
    << "ssp_mode = " << FLAGS_consistency_model << std::endl
    << "init_step_size = " << FLAGS_init_step_size << std::endl
    << "step_dec = " << FLAGS_step_dec << std::endl
    << "use_step_dec = " << ((FLAGS_use_step_dec) ? "True" : "False") << std::endl
    << "lambda = " << FLAGS_lambda << std::endl
    << "data file = " << FLAGS_datafile << std::endl;
  return ss.str();
}

void OutputToDisk(petuum::Table<float> &R_table,
                  petuum::Table<float>& loss_table,
                  int num_obj_evals, bool final) {
  static int record_counter = 0;
  // loss_table
  std::string loss_file;
  if (final) {
    loss_file = FLAGS_output_prefix + ".loss";
  } else {
    loss_file = FLAGS_output_prefix + ".loss" + std::to_string(record_counter);
  }
  LOG(INFO) << "Writing to loss file: " << loss_file;
  std::ofstream loss_stream(loss_file.c_str());
  loss_stream << GetExperimentInfo();
  loss_stream << "Iter Clock Compute-Time Compute-Eval-Time L2_loss "
    << "L2_regularized_loss" << std::endl;
  for (int obj_eval_counter = 0; obj_eval_counter < num_obj_evals;
      ++obj_eval_counter) {
    petuum::RowAccessor loss_acc;
    loss_table.Get(obj_eval_counter, &loss_acc);
    const auto& loss_row = loss_acc.Get<petuum::DenseRow<float> >();
    loss_stream << loss_row[kLossTableColIdxIter] << " "
      << loss_row[kLossTableColIdxClock] << " "
      << loss_row[kLossTableColIdxComputeTime] << " "
      << loss_row[kLossTableColIdxComputeEvalTime] << " "
      << loss_row[kLossTableColIdxL2Loss] << " "
      << loss_row[kLossTableColIdxL2RegLoss] << std::endl;
  }
  loss_stream.close();
  ++record_counter;
}

void RecordLoss(
    int obj_eval_counter, int iter, int clock,
    std::vector<std::vector<float> > &L_table,
    size_t L_row_id_offset,
    petuum::Table<float>& R_table,
    petuum::Table<float>& loss_table,
    int col_begin, int col_end,
    int global_worker_id, int total_num_workers,
    int element_begin, int element_end,
    std::vector<float>* Rj_cache) {
  double squared_loss = 0.;
  // for (int a = global_worker_id; a < X_row.size(); a += total_num_workers) {
  for (int a = element_begin; a < element_end; ++a) {
    // Let i = X_row[a], j = X_col[a], and X(i,j) = X_val[a]
    const int i = X_row[a];
    const int j = X_col[a];
    const float Xij = X_val[a];
    // Read L(i,:) and R(:,j) from Petuum PS
    {
      petuum::RowAccessor Rj_acc;
      const auto& Rj_row = R_table.Get<petuum::DenseRow<float> >(j, &Rj_acc);
      Rj_row.CopyToVector(Rj_cache);
    }
    auto& Li = L_table[i - L_row_id_offset];
    auto& Rj = *Rj_cache;

    // Compute L(i,:) * R(:,j)
    float LiRj = 0.0;
    for (int k = 0; k < FLAGS_K; ++k) {
      LiRj += Li[k] * Rj[k];
    }
    // Update the L2 (non-regularized version) loss function
    squared_loss += pow(Xij - LiRj, 2);
  }
  loss_table.Inc(obj_eval_counter, kLossTableColIdxL2Loss, squared_loss);

  if (global_worker_id == 0) {
    loss_table.Inc(obj_eval_counter, kLossTableColIdxClock, clock);
    loss_table.Inc(obj_eval_counter, kLossTableColIdxIter, iter);
  }

  // Compute loss.
  double L2_reg_loss = 0.;
  for (auto &Li : L_table) {
    for (int k = 0; k < FLAGS_K; ++k) {
      L2_reg_loss += Li[k] * Li[k];
    }
  }

  for (int ii = col_begin; ii < col_end; ++ii) {
    {
      petuum::RowAccessor Rj_acc;
      const auto& Rj_row
          = R_table.Get<petuum::DenseRow<float> >(ii, &Rj_acc);
      Rj_row.CopyToVector(Rj_cache);
    }
    auto &Rj = *Rj_cache;

    for (int k = 0; k < FLAGS_K; ++k) {
      L2_reg_loss += Rj[k] * Rj[k];
    }
  }

  L2_reg_loss *= FLAGS_lambda;
  loss_table.Inc(obj_eval_counter, kLossTableColIdxL2RegLoss,
                 L2_reg_loss + squared_loss);
}

// Main Matrix Factorization routine, called by pthread_create
void SolveMF(int32_t thread_id, boost::barrier* process_barrier) {
  // Register this thread with Petuum PS
  petuum::PSTableGroup::RegisterThread();
  // Get tables
  petuum::Table<float> R_table = petuum::PSTableGroup::GetTableOrDie<float>(1);
  petuum::Table<float> loss_table =
      petuum::PSTableGroup::GetTableOrDie<float>(2);

  std::vector<std::vector<float> > L_table;
  size_t row_id_offset = 0;
  InitLTable(FLAGS_K, thread_id, &L_table, &row_id_offset);

  const int total_num_workers = get_total_num_workers();
  // global_worker_id lies in the range [0,total_num_workers)
  const int global_worker_id = get_global_worker_id(thread_id);

  int num_cols_per_thread = X_num_cols / total_num_workers;
  int col_begin = global_worker_id * num_cols_per_thread;
  int col_end = (global_worker_id == total_num_workers - 1) ?
                (X_num_cols - 1) : col_begin + num_cols_per_thread;

  // Cache for DenseRow bulk-read
  std::vector<float> Rj_cache(FLAGS_K);

  STATS_APP_INIT_BEGIN();
  // Initialize MF solver
  InitMF(L_table, R_table, col_begin, col_end);

  // Run additional iterations to let stale values finish propagating
  petuum::PSTableGroup::GlobalBarrier();
  STATS_APP_INIT_END();

  // Run MF solver
  int clock = 0;
  petuum::HighResolutionTimer total_timer;
  double total_eval_time = 0.;
  int obj_eval_counter = 0;  // ith objective evaluation

  int64_t element_begin = X_partition_starts[thread_id];
  int64_t element_end = (thread_id == FLAGS_num_worker_threads - 1) ?
                    X_row.size() : X_partition_starts[thread_id + 1];

  // round down in order to have same # of clocks per iteration.
  int64_t work_per_clock = (element_end - element_begin) / FLAGS_num_clocks_per_iter;
  CHECK(work_per_clock > 0) << "work_per_clock is less than 1"
                            << " reduce num_clocks_per_iter "
                            << " X_row.size() = " << X_row.size()
                            << " total_num_workers = " << total_num_workers;

  // Bootstrap.
  if (thread_id == 0) {
    LOG(INFO) << "Bootstrap starting";
    petuum::HighResolutionTimer bootstrap_timer;
    STATS_APP_BOOTSTRAP_BEGIN();
    std::set<int32_t> R_rows;
    // Last element on this machine.
    int64_t process_element_end = X_row.size();

    for (int64_t a = 0; a < process_element_end; ++a) {
      R_rows.insert(X_col[a]);
    }

    for (const auto& e : R_rows) {
      R_table.GetAsyncForced(e);
    }

    R_table.WaitPendingAsyncGet();
    STATS_APP_BOOTSTRAP_END();
    LOG(INFO) << "Bootstrap finished in " << bootstrap_timer.elapsed()
              << " seconds.";
  }
  process_barrier->wait();

  petuum::PSTableGroup::GlobalBarrier();

  petuum::PSTableGroup::TurnOnEarlyComm();

  for (int iter = 0; iter < FLAGS_num_iterations; ++iter) {
    if (global_worker_id == 0) {
      LOG(INFO) << "Iteration " << iter+1 << "/"
                << FLAGS_num_iterations << "... ";
    }

    size_t element_counter = 0;
    float step_size = 0.;
    if (FLAGS_use_step_dec) {
      // Use Multiplicative step size to compare with GraphLab.
      step_size = FLAGS_init_step_size * pow(FLAGS_step_dec, iter);
    } else {
      step_size = FLAGS_init_step_size * pow(100.0 + iter, -0.5);
    }

    // Overall computation (no eval / communication time)
    STATS_APP_ACCUM_COMP_BEGIN();
    for (int64_t a = element_begin; a < element_end; ++a) {
      SgdElement(a, iter, step_size, global_worker_id, L_table,
                 row_id_offset, R_table, loss_table, &Rj_cache);
      ++element_counter;

      if ((element_counter % work_per_clock == 0
           && clock < (iter + 1)*FLAGS_num_clocks_per_iter - 1)
          || (element_counter == element_end - element_begin)) {
        petuum::PSTableGroup::Clock();
        ++clock;
        STATS_APP_ACCUM_COMP_END();

        // Evaluate (if needed) before clocking.
        if (clock % FLAGS_num_clocks_per_eval == 0) {
          STATS_APP_ACCUM_OBJ_COMP_BEGIN();
          petuum::HighResolutionTimer eval_timer;
          // Record objective at each clock
          RecordLoss(obj_eval_counter, iter+1, clock, L_table,
                     row_id_offset, R_table,
                     loss_table, col_begin, col_end,
                     global_worker_id, total_num_workers,
                     element_begin, element_end, &Rj_cache);
          float eval_cost = eval_timer.elapsed();
          total_eval_time += eval_cost;
          STATS_APP_ACCUM_OBJ_COMP_END();

          if (global_worker_id == 0 && obj_eval_counter > 0) {
            // Read the obj value from last recording (which is fresh as loss
            // table has 0 staleness).
            {
              petuum::RowAccessor loss_acc;
              const petuum::DenseRow<float>& loss_row =
                  loss_table.Get<petuum::DenseRow<float> >(obj_eval_counter - 1, &loss_acc);
              LOG(INFO) << "End of clock " << clock
                << " L2_loss = " << loss_row[kLossTableColIdxL2Loss]
                << " L2_reg_loss = " << loss_row[kLossTableColIdxL2RegLoss];
              // Release loss_acc.
            }

            // Output to disk in case of failure.
            OutputToDisk(R_table, loss_table, obj_eval_counter,
                false);
            float total_time = total_timer.elapsed();
            float compute_time = total_time - total_eval_time;
            LOG(INFO) << " Evaluation cost = " << eval_cost
              << " || Total compute time = " << compute_time
              << " || Total time = " << total_time;
            // Record approximate end-of-this-clock time (using head thread's
            // finish time).
            loss_table.Inc(obj_eval_counter, kLossTableColIdxComputeTime,
                compute_time);
            loss_table.Inc(obj_eval_counter, kLossTableColIdxComputeEvalTime,
                total_time);
          }
          ++obj_eval_counter;
        } else {
          // Do a fake get to avoid initial block time.
          const int i = X_col[a];
          petuum::RowAccessor Ri_acc;
          R_table.Get(i, &Ri_acc);
        }
        // Overall computation (no eval / communication time)
        STATS_APP_ACCUM_COMP_BEGIN();
      }
    }
    CHECK_EQ((iter+1)*FLAGS_num_clocks_per_iter, clock);
    CHECK_EQ(clock / FLAGS_num_clocks_per_eval, obj_eval_counter);
    LOG_IF(INFO, global_worker_id == 0) << "Iter " << iter+1
      << " finished. Time: " << total_timer.elapsed();
  }

  petuum::PSTableGroup::TurnOffEarlyComm();

  // Finish propagation
  petuum::PSTableGroup::GlobalBarrier();

  // Output loss function
  if (global_worker_id == 0) {
    std::stringstream ss;
    ss << "Iter Clock Compute-Time Compute-Eval-Time L2_loss L2_reg_loss"
      << std::endl;
    for (int c = 0; c < obj_eval_counter; ++c) {
      petuum::RowAccessor loss_acc;
      const auto& loss_row = loss_table.Get<petuum::DenseRow<float> >(c, &loss_acc);
      ss << loss_row[kLossTableColIdxIter] << " "
        << loss_row[kLossTableColIdxClock] << " "
        << loss_row[kLossTableColIdxComputeTime] << " "
        << loss_row[kLossTableColIdxComputeEvalTime] << " "
        << loss_row[kLossTableColIdxL2Loss] << " "
        << loss_row[kLossTableColIdxL2RegLoss] << std::endl;
    }
    LOG(INFO) << "Summary Stats = \n" << ss.str();
  }

  // Output results to disk
  if (global_worker_id == 0) {
    LOG(INFO) << "Outputting results to prefix " << FLAGS_output_prefix
      << " ... ";
    OutputToDisk(R_table, loss_table, obj_eval_counter, true);
    LOG(INFO) << "done";
  }
  // Deregister this thread with Petuum PS
  petuum::PSTableGroup::DeregisterThread();
}

// Main function
int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;

  LOG(INFO) << "staleness = " << FLAGS_table_staleness;

  // Configure Petuum PS
  petuum::TableGroupConfig table_group_config;
  petuum::InitTableGroupConfig(&table_group_config, 2);

  // Configure PS row types
  // Register dense float rows as ID 0
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<float> >(0);
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<int64_t> >(1);

  // Initializing thread does not need table access
  petuum::PSTableGroup::Init(table_group_config, false);

  LOG(INFO) << "TableGroupInit is done";

  // Load Data
  if (FLAGS_client_id == 0) {
    LOG(INFO) << std::endl << "Loading data... ";
  }

  LOG(INFO) << "Read data " << FLAGS_datafile;

  ReadBinaryMatrix(FLAGS_datafile, FLAGS_client_id);
  PartitionWorkLoad(FLAGS_num_worker_threads);

  if (FLAGS_client_id == 0) {
    LOG(INFO) << std::endl << GetExperimentInfo();
  }

  // Configure PS tables
  petuum::ClientTableConfig table_config;
  petuum::InitTableConfig(&table_config);

  table_config.table_info.server_push_row_upper_bound
      = FLAGS_server_push_row_upper_bound;

  // R_table (M by K)
  table_config.table_info.row_capacity = FLAGS_K;
  table_config.table_info.dense_row_oplog_capacity = FLAGS_K;
  table_config.process_cache_capacity = FLAGS_M_cache_size;
  table_config.thread_cache_capacity = 1;
  table_config.oplog_capacity = FLAGS_M_cache_size;
  table_config.client_send_oplog_upper_bound
      = FLAGS_M_client_send_oplog_upper_bound;
  petuum::PSTableGroup::CreateTable(1, table_config);

  table_config.table_info.oplog_dense_serialized = true;
  table_config.no_oplog_replay = false;
  table_config.oplog_type = petuum::Sparse;
  table_config.process_storage_type = petuum::BoundedSparse;
  table_config.table_info.table_staleness = FLAGS_table_staleness;

  table_config.table_info.row_capacity = 6;
  table_config.table_info.dense_row_oplog_capacity = 6;
  table_config.table_info.row_oplog_type = 0;
  table_config.process_cache_capacity = 100;
  table_config.thread_cache_capacity = 1;
  table_config.oplog_capacity = 100;
  table_config.client_send_oplog_upper_bound = 1;
  petuum::PSTableGroup::CreateTable(2, table_config);

  // Finished creating tables
  petuum::PSTableGroup::CreateTableDone();

  LOG(INFO) << "Created all tables";

  // Run Petuum PS-based MF solver
  std::vector<std::thread> threads(FLAGS_num_worker_threads);
  boost::barrier process_barrier(FLAGS_num_worker_threads);
  for (int t = 0; t < FLAGS_num_worker_threads; ++t) {
    threads[t] = std::thread(SolveMF, t, &process_barrier);
  }
  for (auto& thr : threads) {
    thr.join();
  }

  // Cleanup and output runtime
  petuum::PSTableGroup::ShutDown();
  if (FLAGS_client_id == 0) {
    LOG(INFO) << "total runtime = " << total_timer.elapsed() << "s";
  }

  LOG(INFO) << "exiting " << FLAGS_client_id;
  return 0;
}
