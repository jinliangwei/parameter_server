/*
 * Least-squares Matrix Factorization application built on Petuum Parameter Server.
 *
 * Author: qho
 */
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
#include <gflags/gflags.h>
#include <glog/logging.h>

// Command-line flags
DEFINE_string(hostfile, "", "Path to Petuum PS server configuration file");
DEFINE_string(datafile, "", "Input sparse matrix");
DEFINE_string(output_prefix, "", "Output results with this prefix. "
    "If the prefix is X, the output will be called X.L and X.R");
DEFINE_double(init_step_size, 0.5, "Initial stochastic gradient descent "
    "step size");
DEFINE_double(step_dec, 0.9, "Step size is "
    "init_step_size * (step_dec)^iter.");
DEFINE_bool(use_step_dec, false, "False to use sqrt instead of "
    "multiplicative decay.");
DEFINE_double(lambda, 0.001, "L2 regularization strength.");
DEFINE_int32(num_clocks_per_eval, 1, "# of clocks between "
    "each objective evaluation");
DEFINE_int32(num_clients, 1, "Total number of clients");
DEFINE_int32(num_worker_threads, 1, "Number of worker threads per client");
DEFINE_int32(num_comm_channels_per_client, 1,
    "number of comm channels per client");
DEFINE_int32(client_id, 0, "This client's ID");
DEFINE_int32(K, 100, "Factorization rank");
DEFINE_int32(num_iterations, 100, "Number of iterations");
DEFINE_int32(staleness, 0, "Staleness");
DEFINE_int32(N_cache_size, 10000000, "Process cache size for the L table.");
DEFINE_int32(M_cache_size, 10000000, "Process cache size for the M table.");
DEFINE_int32(num_clocks_per_iter, 1,
    "clock num_clocks_per_iter in each SGD sweep.");
DEFINE_string(ssp_mode, "SSPPush", "SSPAggr/SSPPush/SSP");
DEFINE_string(data_format, "list", "list, mmt or libsvm");
DEFINE_string(stats_path, "", "Statistics output file");
DEFINE_bool(output_LR, false, "Save L and R matrices to disk or not.");

// SSPAggr parameters
DEFINE_int32(bandwidth_mbps, 40, "Bandwidth in Megabits per second"
    " per comm thread.");
DEFINE_int32(bg_idle_milli, 500, "Bg idle millisecond");
DEFINE_int32(oplog_push_upper_bound_kb, 100,
             "oplog push upper bound in Kilobytes per comm thread.");
DEFINE_int32(oplog_push_staleness_tolerance, 2,
             "oplog push staleness tolerance");
DEFINE_int32(thread_oplog_batch_size, 100*1000*1000,
             "Thread OpLog Batch Size");
DEFINE_int32(server_push_row_threshold, 100, "Server push row threshold");
DEFINE_int32(server_idle_milli, 10, "server idle time out in millisec");
DEFINE_string(update_sort_policy, "Random", "Update sort policy");
DEFINE_int32(row_oplog_type, petuum::RowOpLogType::kDenseRowOpLog, "row oplog type");
DEFINE_bool(oplog_dense_serialized, true, "dense serialized oplog");

DEFINE_string(oplog_type, "Sparse", "use append only oplog?");
DEFINE_uint64(append_only_buffer_capacity, 1024*1024, "buffer capacity in bytes");
DEFINE_uint64(append_only_buffer_pool_size, 3, "append_ only buffer pool size");
DEFINE_int32(bg_apply_append_oplog_freq, 4, "bg apply append oplog freq");

DEFINE_string(process_storage_type, "BoundedSparse", "process storage type");

DEFINE_bool(no_oplog_replay, false, "oplog replay?");

// Data variables
int N_, M_; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)
std::vector<int> X_row; // Row index of each nonzero entry in the data matrix
std::vector<int> X_col; // Column index of each nonzero entry in the data matrix
std::vector<float> X_val; // Value of each nonzero entry in the data matrix
int kLossTableColIdxClock = 0;
int kLossTableColIdxComputeTime = 1;
int kLossTableColIdxL2Loss = 2;
int kLossTableColIdxL2RegLoss = 3;
int kLossTableColIdxComputeEvalTime = 4;
int kLossTableColIdxIter = 5;

// Returns the number of workers threads across all clients
int get_total_num_workers() {
  return FLAGS_num_clients * FLAGS_num_worker_threads;
}
// Returns the global thread ID of this worker
int get_global_worker_id(int thread_id) {
  return (FLAGS_client_id * FLAGS_num_worker_threads) + thread_id;
}

// Read the full data set in libsvm format.
void read_libsvm_data(const std::string& file) {
  petuum::HighResolutionTimer timer;
  char *line = NULL, *ptr = NULL, *endptr = NULL;
  size_t num_bytes;
  FILE *data_stream = fopen(file.c_str(), "r");
  CHECK_NOTNULL(data_stream);
  int base = 10;

  // Read first line: #-rows/users #-columns/movies
  CHECK_NE(-1, getline(&line, &num_bytes, data_stream));
  N_ = strtol(line, &endptr, base);
  ptr = endptr + 1;   // +1 to skip the space.
  M_ = strtol(ptr, &endptr, base);
  LOG_IF(INFO, FLAGS_client_id == 0) << "(#-rows, #-cols) = ("
    << N_ << ", " << M_ << ")";
  int nnz = 0;  // number of non-zero entries.
  while (getline(&line, &num_bytes, data_stream) != -1) {
    int row_id = strtol(line, &endptr, base);   // Read the row id.
    ptr = endptr;
    while (*ptr != '\n') {
      // read a word_id:count pair
      int col_id = strtol(ptr, &endptr, base);
      ptr = endptr; // *ptr = colon
      CHECK_EQ(':', *ptr);

      float val = strtof(++ptr, &endptr);
      ptr = endptr;
      X_row.push_back(row_id);
      X_col.push_back(col_id);
      X_val.push_back(val);
      ++nnz;
      while (*ptr == ' ') ++ptr; // goto next non-space char
    }
  }
  fclose(data_stream);
  LOG(INFO) << "Done reading " << nnz << " non-zero entries in "
    << timer.elapsed() << " seconds.";
}

// Read sparse data matrix into X_row, X_col and X_val. Each line of the matrix
// is a whitespace-separated triple (row,col,value), where row>=0 and col>=0.
// For example:
//
// 0 0 0.5
// 1 2 1.5
// 2 1 2.5
//
// This specifies a 3x3 matrix with 3 nonzero elements: 0.5 at (0,0), 1.5 at
// (1,2) and 2.5 at (2,1).
void read_sparse_matrix(std::string inputfile) {
  X_row.clear();
  X_col.clear();
  X_val.clear();
  N_ = 0;
  M_ = 0;
  std::ifstream inputstream(inputfile.c_str());
  CHECK(inputstream) << "Failed to read " << inputfile;
  while(true) {
    int row, col;
    float val;
    inputstream >> row >> col >> val;
    if (!inputstream) {
      break;
    }
    X_row.push_back(row);
    X_col.push_back(col);
    X_val.push_back(val);
    N_ = row+1 > N_ ? row+1 : N_;
    M_ = col+1 > M_ ? col+1 : M_;
  }
  inputstream.close();
}

// read a MMT matrix in coordinate format.
void read_mmt_data(const std::string &inputfile) {
  X_row.clear();
  X_col.clear();
  X_val.clear();
  char *line = NULL, *ptr = NULL, *endptr = NULL;
  size_t num_bytes;
  FILE *data_stream = fopen(inputfile.c_str(), "r");
  CHECK_NOTNULL(data_stream);
  int base = 10, nnz;

  int suc = getline(&line, &num_bytes, data_stream);
  CHECK_NE(suc, -1);

  // skip the banner
  while (line[0] == '%') {
    suc = getline(&line, &num_bytes, data_stream);
    CHECK_NE(suc, -1);
  }

  N_ = strtol(line, &endptr, base);
  ptr = endptr;
  M_ = strtol(ptr, &endptr, base);
  ptr = endptr;
  nnz = strtol(ptr, &endptr, base);

  LOG(INFO) << "M_ = " << M_
            << " N_ = " << N_
            << " nnz = " << nnz;

  suc = getline(&line, &num_bytes, data_stream);

  int cnt = 0;

  while (suc != -1) {
    int row_id = strtol(line, &endptr, base);   // Read the row id.
    ptr = endptr;
    int col_id = strtol(ptr, &endptr, base);
    ptr = endptr;
    float val = strtof(++ptr, &endptr);

    X_row.push_back(row_id);
    X_col.push_back(col_id);
    X_val.push_back(val);

    suc = getline(&line, &num_bytes, data_stream);
    ++cnt;
  }
  fclose(data_stream);
  LOG(INFO) << "Read " << cnt << " lines";
}

size_t CountLocalNumRows(int32_t element_begin, int32_t element_end) {
  std::set<int> row_ids;
  for (int32_t i = element_begin; i < element_end; ++i) {
    row_ids.insert(X_row[i]);
  }
  return row_ids.size();
}

size_t CountLocalNumColumns(int32_t element_begin, int32_t element_end) {
  std::set<int> col_ids;
  for (int32_t i = element_begin; i < element_end; ++i) {
    col_ids.insert(X_col[i]);
  }
  return col_ids.size();
}

// Performs stochastic gradient descent on X_row[a], X_col[a], X_val[a].
void sgd_element(int a, int iter, float step_size, int global_worker_id,
    petuum::Table<float>& L_table,
    petuum::Table<float>& R_table,
    petuum::Table<float>& loss_table,
    std::vector<float>* Li_cache,
    std::vector<float>* Rj_cache) {
  // Let i = X_row[a], j = X_col[a], and X(i,j) = X_val[a]
  const int i = X_row[a];
  const int j = X_col[a];
  const float Xij = X_val[a];
  // Read L(i,:) and R(:,j) from Petuum PS
  {
    petuum::RowAccessor Li_acc;
    petuum::RowAccessor Rj_acc;
    const auto& Li_row = L_table.Get<petuum::DenseRow<float> >(i, &Li_acc);
    const auto& Rj_row = R_table.Get<petuum::DenseRow<float> >(j, &Rj_acc);
    Li_row.CopyToVector(Li_cache);
    Rj_row.CopyToVector(Rj_cache);
    // Release the accessor.
  }
  std::vector<float>& Li = *Li_cache;
  std::vector<float>& Rj = *Rj_cache;

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
  petuum::DenseUpdateBatch<float> Li_update(0, FLAGS_K);
  petuum::DenseUpdateBatch<float> Rj_update(0, FLAGS_K);
  float grad_coeff = -2 * (Xij - LiRj);
  float regularization_coeff = FLAGS_lambda * 2;
  for (int k = 0; k < FLAGS_K; ++k) {
    float gradient = 0.0;
    // Compute update for L(i,k)
    gradient = grad_coeff * Rj[k] + regularization_coeff * Li[k];
    Li_update[k] = -gradient * step_size;
    // Compute update for R(k,j)
    gradient = grad_coeff * Li[k] + regularization_coeff * Rj[k];
    Rj_update[k] = -gradient * step_size;
  }
  L_table.DenseBatchInc(i, Li_update);
  R_table.DenseBatchInc(j, Rj_update);
}

void init_mf_lt(petuum::Table<float>& L_table, petuum::Table<float>& R_table) {
  // Create a uniform RNG in the range (-1,1)
  std::mt19937 gen(12345);
  std::uniform_real_distribution<float> dist(-1., 1.);
  const int num_workers = get_total_num_workers();

  // Add a random initialization in [0,1)/num_workers to each element of L and R
  for (int i = 0; i < N_; ++i) {
    petuum::DenseUpdateBatch<float> L_updates(0, FLAGS_K);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen) / float(num_workers);
      L_updates[k] = init_val;
    }
    L_table.DenseBatchInc(i, L_updates);
  }
  for (int j = 0; j < M_; ++j) {
    petuum::DenseUpdateBatch<float> R_updates(0, FLAGS_K);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen) / float(num_workers);
      R_updates[k] = init_val;
    }
    R_table.DenseBatchInc(j, R_updates);
  }
}

void init_mf(petuum::Table<float>& L_table, petuum::Table<float>& R_table,
    int N_begin, int N_end, int M_begin, int M_end) {
  // Create a uniform RNG in the range (-1,1)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dist(0, 0.1);

  // Add a random initialization in [0,1)/num_workers to each element of L and R
  for (int i = N_begin; i < N_end; ++i) {
    petuum::DenseUpdateBatch<float> L_updates(0, FLAGS_K);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen);
      L_updates[k] = init_val;
    }
    L_table.DenseBatchInc(i, L_updates);
  }
  for (int j = M_begin; j < M_end; ++j) {
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
    << "Matrix dimensions: " << N_ << " by " << M_ << std::endl
    << "# non-missing entries: " << X_row.size() << std::endl
    << "num_iterations = " << FLAGS_num_iterations << std::endl
    << "num_clients = " << FLAGS_num_clients << std::endl
    << "num_worker_threads = " << FLAGS_num_worker_threads << std::endl
    << "num_comm_channels_per_client = "
    << FLAGS_num_comm_channels_per_client << std::endl
    << "num_clocks_per_iter = " << FLAGS_num_clocks_per_iter << std::endl
    << "num_clocks_per_eval = " << FLAGS_num_clocks_per_eval << std::endl
    << "N_cache_size = " << FLAGS_N_cache_size << std::endl
    << "M_cache_size = " << FLAGS_M_cache_size << std::endl
    << "staleness = " << FLAGS_staleness << std::endl
    << "ssp_mode = " << FLAGS_ssp_mode << std::endl
    << "init_step_size = " << FLAGS_init_step_size << std::endl
    << "step_dec = " << FLAGS_step_dec << std::endl
    << "use_step_dec = " << ((FLAGS_use_step_dec) ? "True" : "False") << std::endl
    << "lambda = " << FLAGS_lambda << std::endl
    << "data file = " << FLAGS_datafile << std::endl;
  return ss.str();
}

// Outputs L_table and R_table to disk
void output_to_disk(petuum::Table<float>& L_table,
    petuum::Table<float>& R_table, petuum::Table<float>& loss_table,
    int num_obj_evals, bool final) {
  static int record_counter = 0;
  if (FLAGS_output_LR) {
    // L_table
    std::string L_file = FLAGS_output_prefix + ".L";
    std::ofstream L_stream(L_file.c_str());
    for (uint32_t i = 0; i < N_; ++i) {
      petuum::RowAccessor Li_acc;
      const petuum::DenseRow<float>& Li = L_table.Get<petuum::DenseRow<float> >(i, &Li_acc);
      for (uint32_t k = 0; k < FLAGS_K; ++k) {
        L_stream << Li[k] << " ";
      }
      L_stream << "\n";
    }
    L_stream.close();

    // R_table
    std::string R_file = FLAGS_output_prefix + ".R";
    std::ofstream R_stream(R_file.c_str());
    for (uint32_t j = 0; j < M_; ++j) {
      petuum::RowAccessor Rj_acc;
      const petuum::DenseRow<float>& Rj = R_table.Get<petuum::DenseRow<float> >(j, &Rj_acc);
      for (uint32_t k = 0; k < FLAGS_K; ++k) {
        R_stream << Rj[k] << " ";
      }
      R_stream << "\n";
    }
    R_stream.close();
  }

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

void record_loss(int obj_eval_counter, int iter, int clock,
    petuum::Table<float>& L_table,
    petuum::Table<float>& R_table, petuum::Table<float>& loss_table,
    int N_begin, int N_end, int M_begin, int M_end, int global_worker_id,
    int total_num_workers, int element_begin, int element_end,
    std::vector<float>* Li_cache, std::vector<float>* Rj_cache) {
  double squared_loss = 0.;
  // for (int a = global_worker_id; a < X_row.size(); a += total_num_workers) {
  for (int a = element_begin; a < element_end; ++a) {
    // Let i = X_row[a], j = X_col[a], and X(i,j) = X_val[a]
    const int i = X_row[a];
    const int j = X_col[a];
    const float Xij = X_val[a];
    // Read L(i,:) and R(:,j) from Petuum PS
    {
      petuum::RowAccessor Li_acc;
      petuum::RowAccessor Rj_acc;
      const auto& Li = L_table.Get<petuum::DenseRow<float> >(i, &Li_acc);
      const auto& Rj = R_table.Get<petuum::DenseRow<float> >(j, &Rj_acc);
      Li.CopyToVector(Li_cache);
      Rj.CopyToVector(Rj_cache);
    }

    // Compute L(i,:) * R(:,j)
    float LiRj = 0.0;
    for (int k = 0; k < FLAGS_K; ++k) {
      LiRj += (*Li_cache)[k] * (*Rj_cache)[k];
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
  for (int ii = N_begin; ii < N_end; ++ii) {
    {
      petuum::RowAccessor Li_acc;
      const auto& Li = L_table.Get<petuum::DenseRow<float> >(ii, &Li_acc);
      Li.CopyToVector(Li_cache);
    }
    for (int k = 0; k < FLAGS_K; ++k) {
      L2_reg_loss += (*Li_cache)[k] * (*Li_cache)[k];
    }
  }
  for (int ii = M_begin; ii < M_end; ++ii) {
    {
      petuum::RowAccessor Rj_acc;
      const auto& Rj = R_table.Get<petuum::DenseRow<float> >(ii, &Rj_acc);
      Rj.CopyToVector(Rj_cache);
    }
    for (int k = 0; k < FLAGS_K; ++k) {
      L2_reg_loss += (*Rj_cache)[k] * (*Rj_cache)[k];
    }
  }
  L2_reg_loss *= FLAGS_lambda;
  loss_table.Inc(obj_eval_counter, kLossTableColIdxL2RegLoss,
      L2_reg_loss + squared_loss);
}

// Main Matrix Factorization routine, called by pthread_create
void solve_mf(int32_t thread_id, boost::barrier* process_barrier) {
  // Register this thread with Petuum PS
  petuum::PSTableGroup::RegisterThread();
  // Get tables
  petuum::Table<float> L_table = petuum::PSTableGroup::GetTableOrDie<float>(0);
  petuum::Table<float> R_table = petuum::PSTableGroup::GetTableOrDie<float>(1);
  petuum::Table<float> loss_table =
      petuum::PSTableGroup::GetTableOrDie<float>(2);

  // Algebra for dividing up L and R tables.
  int total_num_worker_threads = FLAGS_num_clients * FLAGS_num_worker_threads;
  int num_M_rows_per_thread = M_ / total_num_worker_threads;
  int M_begin = (FLAGS_client_id * FLAGS_num_worker_threads
      + thread_id) * num_M_rows_per_thread;
  int M_end = (FLAGS_client_id == (FLAGS_num_clients - 1)
      && thread_id == (FLAGS_num_worker_threads - 1)) ?
    (M_ + 1) : M_begin + num_M_rows_per_thread;
  int num_N_rows_per_thread = N_ / total_num_worker_threads;
  int N_begin = (FLAGS_client_id * FLAGS_num_worker_threads
      + thread_id) * num_N_rows_per_thread;
  int N_end = (FLAGS_client_id == (FLAGS_num_clients - 1)
      && thread_id == (FLAGS_num_worker_threads - 1)) ?
    (N_ + 1) : N_begin + num_N_rows_per_thread;
  const int total_num_workers = get_total_num_workers();

  // Cache for DenseRow bulk-read
  std::vector<float> Li_cache(FLAGS_K);
  std::vector<float> Rj_cache(FLAGS_K);

  STATS_APP_INIT_BEGIN();
  // Initialize MF solver
  init_mf(L_table, R_table, N_begin, N_end, M_begin, M_end);
  //init_mf_lt(L_table, R_table);
  // Run additional iterations to let stale values finish propagating
  petuum::PSTableGroup::GlobalBarrier();
  STATS_APP_INIT_END();

  // global_worker_id lies in the range [0,total_num_workers)
  const int global_worker_id = get_global_worker_id(thread_id);

  // Run MF solver
  int clock = 0;
  petuum::HighResolutionTimer total_timer;
  double total_eval_time = 0.;
  int obj_eval_counter = 0;  // ith objective evaluation

  int num_elements_per_worker = X_row.size() / total_num_workers;
  int element_begin = global_worker_id * num_elements_per_worker;
  int element_end = (global_worker_id == (total_num_workers - 1)) ?
    X_row.size() : element_begin + num_elements_per_worker;

  // round down in order to have same # of clocks per iteration.
  int work_per_clock = num_elements_per_worker / FLAGS_num_clocks_per_iter;
  CHECK(work_per_clock > 0) << "work_per_clock is less than 1"
                            << " reduce num_clocks_per_iter "
                            << " X_row.size() = " << X_row.size()
                            << " total_num_workers = " << total_num_workers;

  // Bootstrap.
  if (thread_id == 0) {
    petuum::HighResolutionTimer bootstrap_timer;
    STATS_APP_BOOTSTRAP_BEGIN();
    std::set<int32_t> L_rows;
    std::set<int32_t> R_rows;
    // Last element on this machine.
    int process_element_end = (FLAGS_client_id == FLAGS_num_clients - 1) ?
      X_row.size() :
      element_begin + FLAGS_num_worker_threads * num_elements_per_worker;
    for (int a = element_begin; a < process_element_end; ++a) {
      L_rows.insert(X_row[a]);
      R_rows.insert(X_col[a]);
    }
    for (const auto& e : L_rows) {
      L_table.GetAsyncForced(e);
    }
    for (const auto& e : R_rows) {
      R_table.GetAsyncForced(e);
    }
    L_table.WaitPendingAsyncGet();
    R_table.WaitPendingAsyncGet();
    STATS_APP_BOOTSTRAP_END();
    LOG(INFO) << "Bootstrap finished in " << bootstrap_timer.elapsed()
      << " seconds.";
  }
  process_barrier->wait();

  for (int iter = 0; iter < FLAGS_num_iterations; ++iter) {
    if (global_worker_id == 0) {
      LOG(INFO) << "Iteration " << iter+1 << "/" <<
        FLAGS_num_iterations << "... ";
    }
    int element_counter = 0;
    float step_size = 0.;
    if (FLAGS_use_step_dec) {
      // Use Multiplicative step size to compare with GraphLab.
      step_size = FLAGS_init_step_size * pow(FLAGS_step_dec, iter);
    } else {
      step_size = FLAGS_init_step_size * pow(100.0 + iter, -0.5);
    }

    // Overall computation (no eval / communication time)
    STATS_APP_ACCUM_COMP_BEGIN();
    for (int a = element_begin; a < element_end; ++a) {
      sgd_element(a, iter, step_size, global_worker_id, L_table,
          R_table,loss_table, &Li_cache, &Rj_cache);
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
          record_loss(obj_eval_counter, iter+1, clock, L_table, R_table,
              loss_table, N_begin,
              N_end, M_begin, M_end, global_worker_id, total_num_workers,
              element_begin, element_end, &Li_cache, &Rj_cache);
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
            output_to_disk(L_table, R_table, loss_table, obj_eval_counter,
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
          // sgd_element(a, iter, step_size, global_worker_id, L_table,
          //    R_table,loss_table, &Li_cache, &Rj_cache);
          const int i = X_row[a];
          petuum::RowAccessor Li_acc;
          L_table.Get(i, &Li_acc);
        }
        // Overall computation (no eval / communication time)
        STATS_APP_ACCUM_COMP_BEGIN();
      }
    }
    CHECK_EQ((iter+1)*FLAGS_num_clocks_per_iter, clock);
    CHECK_EQ(clock / FLAGS_num_clocks_per_eval, obj_eval_counter);
    LOG_IF(INFO, FLAGS_client_id == 0 && thread_id == 0) << "Iter " << iter+1
      << " finished. Time: " << total_timer.elapsed();
  }

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

  petuum::PSTableGroup::GlobalBarrier();

  // Output results to disk
  if (global_worker_id == 0) {
    LOG(INFO) << "Outputting results to prefix " << FLAGS_output_prefix
      << " ... ";
    output_to_disk(L_table, R_table, loss_table, obj_eval_counter, true);
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

  LOG(INFO) << "staleness = " << FLAGS_staleness;

  // Configure Petuum PS
  petuum::TableGroupConfig table_group_config;
  table_group_config.num_comm_channels_per_client
    = FLAGS_num_comm_channels_per_client;
  // Global params
  table_group_config.num_total_clients = FLAGS_num_clients;
  // L_table, R_table, loss_table.
  table_group_config.num_tables = 3;
  // +1 for main() thread
  table_group_config.num_local_app_threads = FLAGS_num_worker_threads + 1;
  petuum::GetHostInfos(FLAGS_hostfile, &table_group_config.host_map);
  table_group_config.client_id = FLAGS_client_id;
  if (FLAGS_ssp_mode == "SSPPush") {
    table_group_config.consistency_model = petuum::SSPPush;
  } else if (FLAGS_ssp_mode == "SSP") {
    table_group_config.consistency_model = petuum::SSP;
  } else if (FLAGS_ssp_mode == "SSPAggr") {
    table_group_config.consistency_model = petuum::SSPAggr;
  } else {
    LOG(FATAL) << "Unsupported ssp mode " << FLAGS_ssp_mode;
  }
  table_group_config.stats_path = FLAGS_stats_path;

  // SSPAggr parameters.
  table_group_config.bandwidth_mbps = FLAGS_bandwidth_mbps;
  table_group_config.bg_idle_milli = FLAGS_bg_idle_milli;
  table_group_config.oplog_push_upper_bound_kb
    = FLAGS_oplog_push_upper_bound_kb;
  table_group_config.oplog_push_staleness_tolerance
    = FLAGS_oplog_push_staleness_tolerance;
  table_group_config.thread_oplog_batch_size = FLAGS_thread_oplog_batch_size;
  table_group_config.server_push_row_threshold
    = FLAGS_server_push_row_threshold;
  table_group_config.server_idle_milli
    = FLAGS_server_idle_milli;

  // Configure PS row types
  // Register dense float rows as ID 0
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<float> >(0);
  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<int64_t> >(1);
  // Start PS
  // IMPORTANT: This command starts up the name node service on client 0.
  //            We therefore do it ASAP, before other lengthy actions like
  //            loading data.
  // Initializing thread does not need table access
  petuum::PSTableGroup::Init(table_group_config, false);

  LOG(INFO) << "TableGroupInit is done";

  // Load Data
  if (FLAGS_client_id == 0) {
    LOG(INFO) << std::endl << "Loading data... ";
  }

  if (FLAGS_data_format == "libsvm") {
    read_libsvm_data(FLAGS_datafile);
  } else if (FLAGS_data_format == "list") {
    read_sparse_matrix(FLAGS_datafile);
  } else if (FLAGS_data_format == "mmt") {
    read_mmt_data(FLAGS_datafile);
  } else {
    LOG(FATAL) << "Unknown data format " << FLAGS_data_format;
  }

  //  for (int i = 0; i < 8; ++i) {
  //int num_elements_per_client = X_row.size() / 8;
  //int element_begin = i * num_elements_per_client;
  //int element_end = (i == (8 - 1)) ?
  //X_row.size() : element_begin + num_elements_per_client;
  //size_t num_rows = CountLocalNumRows(element_begin, element_end);
  //  size_t num_cols = CountLocalNumColumns(element_begin, element_end);
  //  LOG(INFO) << "client = " << i
  //          << " num_rows = " << num_rows
  //           << " num_cols = " << num_cols;
  //}

  //exit(0);

  if (FLAGS_client_id == 0) {
    LOG(INFO) << std::endl << GetExperimentInfo();
  }

  petuum::OpLogType oplog_type;

  if (FLAGS_oplog_type == "Sparse") {
    oplog_type = petuum::Sparse;
  } else if (FLAGS_oplog_type == "AppendOnly"){
    oplog_type = petuum::AppendOnly;
  } else if (FLAGS_oplog_type == "Dense") {
    oplog_type = petuum::Dense;
  } else {
    LOG(FATAL) << "Unknown oplog type = " << FLAGS_oplog_type;
  }

  petuum::ProcessStorageType process_storage_type;

  if (FLAGS_process_storage_type == "BoundedDense") {
    process_storage_type = petuum::BoundedDense;
  } else if (FLAGS_process_storage_type == "BoundedSparse") {
    process_storage_type = petuum::BoundedSparse;
  } else {
    LOG(FATAL) << "Unknown process storage type " << FLAGS_process_storage_type;
  }

  // Configure PS tables
  petuum::ClientTableConfig table_config;
  table_config.table_info.row_type = 0; // Dense float rows
  table_config.table_info.row_oplog_type = FLAGS_row_oplog_type;
  table_config.table_info.oplog_dense_serialized = FLAGS_oplog_dense_serialized;
  table_config.oplog_type = oplog_type;
  table_config.append_only_oplog_type = petuum::DenseBatchInc;
  table_config.append_only_buff_capacity = FLAGS_append_only_buffer_capacity;
  table_config.per_thread_append_only_buff_pool_size
      = FLAGS_append_only_buffer_pool_size;
  table_config.bg_apply_append_oplog_freq = FLAGS_bg_apply_append_oplog_freq;
  table_config.process_storage_type = process_storage_type;
  table_config.no_oplog_replay = FLAGS_no_oplog_replay;

  // L_table (N by K)
  table_config.table_info.table_staleness = FLAGS_staleness;
  table_config.table_info.row_capacity = FLAGS_K;
  table_config.table_info.dense_row_oplog_capacity = FLAGS_K;
  //table_config.process_cache_capacity = N_;
  table_config.process_cache_capacity = FLAGS_N_cache_size;
  table_config.oplog_capacity = FLAGS_N_cache_size;
  petuum::PSTableGroup::CreateTable(0,table_config);

  // R_table (M by K)
  table_config.table_info.table_staleness = FLAGS_staleness;
  table_config.table_info.row_capacity = FLAGS_K;
  table_config.table_info.dense_row_oplog_capacity = FLAGS_K;
  //table_config.process_cache_capacity = M_;
  table_config.process_cache_capacity = FLAGS_M_cache_size;
  table_config.oplog_capacity = FLAGS_M_cache_size;
  petuum::PSTableGroup::CreateTable(1,table_config);

  table_config.no_oplog_replay = false;
  table_config.oplog_type = petuum::Sparse;
  table_config.process_storage_type = petuum::BoundedSparse;
  // loss_table (1 by total # workers)
  table_config.table_info.table_staleness = FLAGS_staleness;  // No staleness for loss table
  // <iter, clock, compute-time, compute+eval_time, L2_loss, L2_regularized_loss> (each row is an iter).
  table_config.table_info.row_capacity = 6;
  table_config.process_cache_capacity = 1;
  table_config.oplog_capacity = 100;
  petuum::PSTableGroup::CreateTable(2,table_config);

  // Finished creating tables
  petuum::PSTableGroup::CreateTableDone();

  LOG(INFO) << "Created all tables";

  // Run Petuum PS-based MF solver
  std::vector<std::thread> threads(FLAGS_num_worker_threads);
  boost::barrier process_barrier(FLAGS_num_worker_threads);
  for (int t = 0; t < FLAGS_num_worker_threads; ++t) {
    threads[t] = std::thread(solve_mf, t, &process_barrier);
  }
  for (auto& thr : threads) {
    thr.join();
  }

  // Cleanup and output runtime
  petuum::PSTableGroup::ShutDown();
  if (FLAGS_client_id == 0) {
    LOG(INFO) << "total runtime = " << total_timer.elapsed() << "s";
  }
  return 0;
}
