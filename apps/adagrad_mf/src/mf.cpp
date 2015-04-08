#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <limits>
#include <random>
#include <set>
#include <math.h>
#include <mutex>
#include <boost/thread/barrier.hpp>

#include <petuum_ps_common/util/high_resolution_timer.hpp>
#include <petuum_ps_common/util/thread.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

// Command-line flags
DEFINE_double(init_step_size, 1e-2, "initial step size");
DEFINE_double(step_size_decay, 0.995, "initial step size");
DEFINE_double(lambda, 0.001, "L2 regularization strength.");
DEFINE_int32(K, 100, "factorization rank");
DEFINE_int32(num_iters, 100, "number of iterations");
DEFINE_int32(num_iters_per_eval, 100, "number of iterations per eval");
DEFINE_int32(num_workers, 1, "number of worker threads per client");
DEFINE_int64(refresh_freq, -1, "refresh rate");
DEFINE_string(datafile, "", "data file");
DEFINE_string(loss_file, "", "loss file");
DEFINE_string(output_textfile, "", "output text");

class AdaGradMF {
public:
  AdaGradMF();
  ~AdaGradMF();

  void Train(const std::string &train_file,
             int num_iters);
private:
  void LoadData(const std::string &train_file, int32_t partition_id);
  void PartitionWorkLoad(size_t num_local_workers);
  void InitModel();
  void TakeSnapShot(const std::string &path);

  friend class AdaGradMFWorker;

  // model
  // no need for concurrency control as it is shared
  std::vector<std::vector<float> > L_mat_;

  std::mutex R_mtx_;
  std::vector<std::vector<float> > R_mat_;

  // data
  size_t X_num_rows_, X_num_cols_;
  std::vector<int> X_row_;
  std::vector<int> X_col_;
  std::vector<float> X_val_;
  std::vector<uint64_t> X_partition_starts_;

  // loss
  std::mutex loss_mtx_;
  std::vector<float> squared_loss_;
  std::vector<float> reg_loss_;
};

AdaGradMF::AdaGradMF() { }

AdaGradMF::~AdaGradMF() { }

void AdaGradMF::LoadData(const std::string &filename, int32_t partition_id) {
  std::string bin_file = (partition_id >= 0) ?
                         (filename + "." + std::to_string(partition_id))
                         : filename;

  FILE *bin_input = fopen(bin_file.c_str(), "rb");
  CHECK(bin_input != 0) << "failed to read " << bin_file;

  size_t num_nnz_this_partition = 0,
        num_rows_this_partition = 0,
        num_cols_this_partition = 0;
  size_t read_size = fread(&num_nnz_this_partition, sizeof(size_t), 1, bin_input);
  CHECK_EQ(read_size, 1);
  read_size = fread(&num_rows_this_partition, sizeof(size_t), 1, bin_input);
  CHECK_EQ(read_size, 1);
  read_size = fread(&num_cols_this_partition, sizeof(size_t), 1, bin_input);
  CHECK_EQ(read_size, 1);

  X_row_.resize(num_nnz_this_partition);
  X_col_.resize(num_nnz_this_partition);
  X_val_.resize(num_nnz_this_partition);

  read_size = fread(X_row_.data(), sizeof(int), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);
  read_size = fread(X_col_.data(), sizeof(int), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);
  read_size = fread(X_val_.data(), sizeof(float), num_nnz_this_partition, bin_input);
  CHECK_EQ(read_size, num_nnz_this_partition);

  X_num_rows_ = num_rows_this_partition;
  X_num_cols_ = num_cols_this_partition;
  LOG(INFO) << "partition = " << partition_id
            << " #row = " << X_num_rows_
            << " #cols = " << X_num_cols_
            << " nnz = " << X_row_.size();
}

void AdaGradMF::PartitionWorkLoad(size_t num_workers) {
  size_t num_nnz = X_val_.size();
  size_t num_nnz_per_partition = num_nnz / num_workers;

  X_partition_starts_.resize(num_workers);
  uint64_t partition_start = 0;
  for (int32_t i = 0; i < num_workers; ++i) {
    X_partition_starts_[i] = partition_start;

    uint64_t partition_end = (i == num_workers - 1) ?
                             num_nnz - 1 :
                             partition_start + num_nnz_per_partition;

    uint64_t end_row_id = X_row_[partition_end];
    CHECK(partition_end < num_nnz) << "i = " << i;

    if (i != num_workers - 1) {
      while (partition_end < num_nnz
             && end_row_id == X_row_[partition_end]) {
        ++partition_end;
      }

      CHECK(partition_end < num_nnz) << "There's empty bin " << i;
      partition_end--;
      partition_start = partition_end + 1;
    }
    LOG(INFO) << "partition start = " << partition_start
              << " partition end = " << partition_end;
  }
}

void AdaGradMF::InitModel() {
  L_mat_.resize(X_num_rows_);
  R_mat_.resize(X_num_cols_);

  // Create a uniform RNG in the range (-1,1)
  //std::random_device rd;
  //std::mt19937 gen(rd());
  std::mt19937 gen(1234);
  std::normal_distribution<float> dist(0, 0.1);

  for (auto &L_row : L_mat_) {
    L_row.resize(FLAGS_K, 0);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen);
      L_row[k] = init_val;
    }
  }

  for (auto &R_row : R_mat_) {
    R_row.resize(FLAGS_K, 0);
    for (int k = 0; k < FLAGS_K; ++k) {
      double init_val = dist(gen);
      R_row[k] = init_val;
    }
  }
}

class AdaGradMFWorker : public petuum::Thread {
public:
  AdaGradMFWorker(
      size_t num_workers,
      int32_t my_id,
      int num_iters,
      AdaGradMF &adagrad_mf,
      boost::barrier &process_barrier);
  ~AdaGradMFWorker();

  void *operator() ();

private:
  void SgdElement(int64_t a, float step_size);
  void ApplyUpdates();
  void RefreshWeights();

  const size_t num_workers_;
  const int32_t my_id_;
  const int num_iters_;
  AdaGradMF &adagrad_mf_;

  boost::barrier &process_barrier_;
  std::vector<std::vector<float> > &L_mat_partitioned_;

  std::mutex &R_mtx_shared_;
  std::vector<std::vector<float> > &R_mat_shared_;

  std::vector<std::vector<float> > R_mat_;
  std::vector<std::vector<float> > R_updates_;

  // data
  const size_t &X_num_rows_, &X_num_cols_;
  const std::vector<int> &X_row_;
  const std::vector<int> &X_col_;
  const std::vector<float> &X_val_;

  uint64_t X_partition_start_;
  uint64_t X_partition_end_;
};

void AdaGradMF::Train(const std::string &train_file,
                      int num_iters) {
  boost::barrier process_barrier(FLAGS_num_workers);
  LoadData(train_file, 0);
  PartitionWorkLoad(FLAGS_num_workers);
  LOG(INFO) << "Workload is partitioned";
  InitModel();
  LOG(INFO) << "Init model";

  squared_loss_.resize(num_iters, 0);
  reg_loss_.resize(num_iters, 0);

  AdaGradMFWorker *worker[FLAGS_num_workers];

  for (int i = 0; i < FLAGS_num_workers; ++i) {
    worker[i] = new AdaGradMFWorker(FLAGS_num_workers,
                                    i,
                                    FLAGS_num_iters,
                                    *this,
                                    process_barrier);
  }

  int n = 0;
  for (auto &wptr : worker) {
    LOG(INFO) << "starting " << n++;
    wptr->Start();
  }

  for (auto &wptr : worker) {
    wptr->Join();
  }

  if (FLAGS_output_textfile != "")
    TakeSnapShot(FLAGS_output_textfile);

  for (auto &wptr : worker) {
    delete wptr;
  }
}

void RowToString(
    const std::vector<float> &row,
    std::string *str) {
  *str = "";
  for (int i = 0; i < row.size(); ++i) {
    *str += std::to_string(row[i]);
    *str += " ";
  }
}

void AdaGradMF::TakeSnapShot(const std::string &path) {
  {
    std::ofstream text_file;
    text_file.open(path + ".R");
    CHECK(text_file.good());
    std::string str;
    for (int i = 0; i < R_mat_.size(); ++i) {
      const auto &row_vec = R_mat_[i];
      RowToString(row_vec, &str);
      text_file << i
                << " " << str << "\n";
    }
    text_file.close();
  }
  {
    std::ofstream text_file;
    text_file.open(path + ".L");
    CHECK(text_file.good());
    std::string str;
    for (int i = 0; i < L_mat_.size(); ++i) {
      const auto &row_vec = L_mat_[i];
      RowToString(row_vec, &str);
      text_file << i
                << " " << str << "\n";
    }
    text_file.close();
  }
}

AdaGradMFWorker::AdaGradMFWorker(
    size_t num_workers,
    int32_t my_id,
    int num_iters,
    AdaGradMF &adagrad_mf,
    boost::barrier &process_barrier):
    num_workers_(num_workers),
    my_id_(my_id),
    num_iters_(num_iters),
    adagrad_mf_(adagrad_mf),
    process_barrier_(process_barrier),
    L_mat_partitioned_(adagrad_mf.L_mat_),
    R_mtx_shared_(adagrad_mf.R_mtx_),
    R_mat_shared_(adagrad_mf.R_mat_),
    R_mat_(adagrad_mf.X_num_cols_),
    R_updates_(adagrad_mf.X_num_cols_),
    X_num_rows_(adagrad_mf.X_num_rows_),
    X_num_cols_(adagrad_mf.X_num_cols_),
    X_row_(adagrad_mf.X_row_),
    X_col_(adagrad_mf.X_col_),
    X_val_(adagrad_mf.X_val_),
  X_partition_start_(adagrad_mf.X_partition_starts_[my_id]) {

  if (my_id == (num_workers - 1)) {
    X_partition_end_ = X_row_.size();
  } else {
    X_partition_end_ = adagrad_mf.X_partition_starts_[my_id + 1];
  }

  for (auto &row : R_mat_) {
    row.resize(FLAGS_K, 0);
  }

  for (auto &row : R_updates_) {
    row.resize(FLAGS_K, 0);
  }
}

AdaGradMFWorker::~AdaGradMFWorker() { }

void *AdaGradMFWorker::operator() () {
  petuum::HighResolutionTimer train_timer;
  float step_size = FLAGS_init_step_size;
  for (int iter = 0; iter < num_iters_; ++iter) {
    size_t element_counter = 0;
    RefreshWeights();
    for (int64_t a = X_partition_start_; a < X_partition_end_; ++a) {
      SgdElement(a, step_size);
      ++element_counter;

      if (FLAGS_refresh_freq > 0
          && (element_counter % FLAGS_refresh_freq == 0)) {
        ApplyUpdates();
        RefreshWeights();
        element_counter = 0;
      }
    }
    step_size *= FLAGS_step_size_decay;
    ApplyUpdates();
    process_barrier_.wait();

    if (iter % FLAGS_num_iters_per_eval == 0) {
      RefreshWeights();
      float squared_loss = 0;
      LOG(INFO) << "begin = " << X_partition_start_
                << " end = " << X_partition_end_;
      for (int64_t a = X_partition_start_; a < X_partition_end_; ++a) {
        const int i = X_row_[a];
        const int j = X_col_[a];
        const float Xij = X_val_[a];
        auto& Li = L_mat_partitioned_[i];
        auto& Rj = R_mat_[j];
        float LiRj = 0.0;
        for (int k = 0; k < FLAGS_K; ++k) {
          LiRj += Li[k] * Rj[k];
        }
        squared_loss += pow(Xij - LiRj, 2);
        //LOG(INFO) << Xij << " " << LiRj << " " << squared_loss;

        //LOG(INFO) << "sl = " << squared_loss;
      }

      LOG(INFO) << "squared loss = " << squared_loss;

      float reg_loss = 0;
      int row_id_start = X_row_[X_partition_start_];
      int row_id_end = X_row_[X_partition_end_ - 1];
      for (int i = row_id_start; i <= row_id_end; ++i) {
        for (int j = 0; j < FLAGS_K; ++j) {
          reg_loss += pow(L_mat_partitioned_[i][j], 2);
          CHECK(reg_loss == reg_loss)
              << "i = " << i
              << " j = " << j
              << " loss = " << reg_loss
              << " val = " << L_mat_partitioned_[i][j];
        }
      }

      //LOG(INFO) << "reg_loss = " << reg_loss;

      if (iter % num_workers_ == my_id_) {
        for (int i = 0; i < X_num_cols_; ++i) {
          for (int j = 0; j < FLAGS_K; ++j) {
            reg_loss += pow(R_mat_[i][j], 2);
          }
        }
      }
      {
        std::lock_guard<std::mutex> lock(adagrad_mf_.loss_mtx_);
        adagrad_mf_.squared_loss_[iter] += squared_loss;
        adagrad_mf_.reg_loss_[iter] += FLAGS_lambda*reg_loss + squared_loss;
      }
      process_barrier_.wait();
      if (iter % num_workers_ == my_id_) {
        std::lock_guard<std::mutex> lock(adagrad_mf_.loss_mtx_);
        LOG(INFO) << "iter = " << iter
                  << " sec = " << train_timer.elapsed()
                  << " squred loss = " << adagrad_mf_.squared_loss_[iter]
                  << " reg loss = " << adagrad_mf_.reg_loss_[iter];
      }
      process_barrier_.wait();
    }
  }

  process_barrier_.wait();

  if (FLAGS_loss_file != ""
      && my_id_ == 0) {
    std::ofstream loss_stream(FLAGS_loss_file, std::ios_base::app);
    CHECK(loss_stream.is_open());
    for (int i = 0; i < num_iters_; ++i) {
      loss_stream << "iter: " << i
                  << " squred loss: " << adagrad_mf_.squared_loss_[i]
                  << " reg loss: " << adagrad_mf_.reg_loss_[i] << std::endl;
    }
    loss_stream.close();
  }
  return 0;
}

void AdaGradMFWorker::ApplyUpdates() {
  {
    std::lock_guard<std::mutex> lock(R_mtx_shared_);
    for (int i = 0; i < R_updates_.size(); ++i) {
      for (int j = 0; j < R_updates_[i].size(); ++j) {
        R_mat_shared_[i][j] += R_updates_[i][j];
      }
      R_updates_[i].assign(FLAGS_K, 0);
    }
  }
}

void AdaGradMFWorker::RefreshWeights() {
  {
    std::lock_guard<std::mutex> lock(R_mtx_shared_);
    for (int i = 0; i < X_num_cols_; ++i) {
      memcpy(R_mat_[i].data(), R_mat_shared_[i].data(),
             sizeof(float)*FLAGS_K);
    }
  }
}

// Performs stochastic gradient descent on X_row[a], X_col[a], X_val[a].
void AdaGradMFWorker::SgdElement(int64_t a, float step_size) {
  // Let i = X_row[a], j = X_col[a], and X(i,j) = X_val[a]
  const int i = X_row_[a];
  const int j = X_col_[a];
  const float Xij = X_val_[a];

  auto& Li = L_mat_partitioned_[i];
  auto& Rj = R_mat_[j];

  // Compute L(i,:) * R(:,j)
  float LiRj = 0.0;
  for (int k = 0; k < FLAGS_K; ++k) {
    LiRj += Li[k] * Rj[k];
  }

  float grad_coeff = -(Xij - LiRj);
  float regularization_coeff = FLAGS_lambda;
  for (int k = 0; k < FLAGS_K; ++k) {
    // Compute update for L(i,k)
    float L_gradient = 2*(grad_coeff * Rj[k] + regularization_coeff * Li[k]);
    Li[k] -= step_size * L_gradient;
    CHECK(Li[k] == Li[k]) << a;
    // Compute update for R(k,j)
    float R_gradient = 2*(grad_coeff * Li[k] + regularization_coeff * Rj[k]);
    Rj[k] -= step_size * R_gradient;
    CHECK(Rj[k] == Rj[k]) << a;
    R_updates_[j][k] -= step_size * R_gradient;
  }
}

// Main function
int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;

  AdaGradMF adagrad_mf;
  adagrad_mf.Train(FLAGS_datafile, FLAGS_num_iters);

  LOG(INFO) << "Total time = " << total_timer.elapsed();
  return 0;
}
