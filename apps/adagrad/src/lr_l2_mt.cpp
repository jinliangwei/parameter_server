#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/noncopyable.hpp>
#include <string>
#include <fstream>
#include <math.h>
#include <mutex>
#include <boost/thread/barrier.hpp>

#include <petuum_ps_common/util/high_resolution_timer.hpp>
#include <petuum_ps_common/util/thread.hpp>
#include <ml/include/ml.hpp>
#include <ml/feature/sparse_feature.hpp>

DEFINE_string(train_file, "", "train");
DEFINE_string(test_file, "", "test");
DEFINE_int32(num_epochs, 1, "num epochs");
DEFINE_int32(num_epochs_per_eval, 1, "num epochs");
DEFINE_double(init_lr, 1, "init lr");
DEFINE_double(lr_decay, 0.99, "decay rate");
DEFINE_string(loss_file, "", "");
DEFINE_int32(num_worker_threads, 1, "thrs");
DEFINE_double(lambda, 0.000000000001, "lambda");

class AdaGradLR : public boost::noncopyable {
public:
  AdaGradLR();
  ~AdaGradLR();

  void Train(const std::string &train_file, int num_epochs,
             const std::string &test_file);
private:

  void LoadData(const std::string &train_file,
                const std::string &test_file);
  void DumpModel();
  void CleanFeatures();

  friend class AdaGradLRWorker;

  //data
  size_t feature_dim_;
  size_t num_labels_;
  size_t num_data_;
  size_t num_test_data_;
  std::vector<int32_t> labels_;
  std::vector<petuum::ml::AbstractFeature<float>* > features_;
  std::vector<int32_t> test_labels_;
  std::vector<petuum::ml::AbstractFeature<float>* > test_features_;

  std::mutex mtx_;
  petuum::ml::DenseFeature<float> *weights_;

  std::mutex loss_mtx_;
  std::vector<float> entropy_loss_;
  std::vector<float> zero_one_loss_;
  std::vector<float> reg_loss_;
  std::vector<float> test_entropy_loss_;
  std::vector<float> test_zero_one_loss_;
};

class AdaGradLRWorker : public petuum::Thread {
public:
  AdaGradLRWorker(
      size_t num_workers,
      int32_t my_id,
      int num_epochs,
      AdaGradLR &ada_grad,
      boost::barrier &process_barrier);
  ~AdaGradLRWorker();

  void *operator() ();

private:
  void Predict(
    const petuum::ml::AbstractFeature<float>& feature,
    std::vector<float> *result) const;

  static int32_t ZeroOneLoss(
      const std::vector<float>& prediction,
      int32_t label);

  static float CrossEntropyLoss(
      const std::vector<float>& prediction,
      int32_t label);

  float EvaluateL2RegLoss() const;

  void LoadData(const AdaGradLR &ada_grad);

  void ApplyUpdates();
  void RefreshWeights();

  const size_t num_workers_;
  const int32_t my_id_;
  const int num_epochs_;

  AdaGradLR &ada_grad_;
  boost::barrier &process_barrier_;

  // data
  size_t feature_dim_;
  size_t num_labels_;
  size_t num_total_data_;
  size_t num_data_;
  size_t num_total_test_data_;
  size_t num_test_data_;
  std::vector<std::pair<
                int32_t,
                petuum::ml::AbstractFeature<float>*> > data_;

  std::vector<std::pair<
                int32_t,
                petuum::ml::AbstractFeature<float>*> > test_data_;

  petuum::ml::DenseFeature<float> *weights_;
  std::vector<float> updates_;
  std::vector<float> hist_gradients_;
};

AdaGradLRWorker::AdaGradLRWorker(
      size_t num_workers,
      int32_t my_id,
      int num_epochs,
      AdaGradLR &ada_grad,
      boost::barrier &process_barrier):
    num_workers_(num_workers),
    my_id_(my_id),
    num_epochs_(num_epochs),
    ada_grad_(ada_grad),
    process_barrier_(process_barrier) {
  LoadData(ada_grad);
  weights_ = new petuum::ml::DenseFeature<float>(feature_dim_);
  updates_.resize(feature_dim_);
  hist_gradients_.resize(feature_dim_);
}

AdaGradLRWorker::~AdaGradLRWorker() {
  delete weights_;
}

void AdaGradLRWorker::LoadData(const AdaGradLR &ada_grad) {
  LOG(INFO) << "Loading data start";
  feature_dim_ = ada_grad.feature_dim_;
  num_labels_ = ada_grad.num_labels_;

  size_t num_data = ada_grad.num_data_;
  num_total_data_ = num_data;

  size_t num_data_per_thread = (num_data + num_workers_ - 1) / num_workers_;
  size_t total_data_assigned = num_data_per_thread * num_workers_;
  int32_t cutoff_id = num_workers_ - 1 - (total_data_assigned - num_data);

  int32_t data_idx_begin = 0;
  int32_t data_idx_end = 0;

  if (my_id_ <= cutoff_id) {
    data_idx_begin = num_data_per_thread * my_id_;
    data_idx_end = data_idx_begin + num_data_per_thread;
  } else if (my_id_ == cutoff_id + 1) {
    data_idx_begin = num_data_per_thread * my_id_;
    data_idx_end = data_idx_begin + num_data_per_thread - 1;
  } else {
    data_idx_begin = num_data_per_thread * cutoff_id
                     + (num_data_per_thread - 1) * (my_id_ - cutoff_id);
    data_idx_end = data_idx_begin + num_data_per_thread - 1;
  }

  num_data_ = data_idx_end - data_idx_begin;
  data_.resize(num_data_);

  for (int i = data_idx_begin; i < data_idx_end; ++i) {
    data_[i - data_idx_begin]
        = std::make_pair(ada_grad.labels_[i], ada_grad.features_[i]);
  }

  LOG(INFO) << "load data done, size = " << num_data_;

  size_t num_test_data = ada_grad.num_test_data_;
  num_total_test_data_ = num_test_data;

  LOG(INFO) << "num_test_data = " << num_test_data;

  if (num_test_data > 0) {
    size_t num_data_per_thread = (num_test_data + num_workers_ - 1) / num_workers_;
    size_t total_data_assigned = num_data_per_thread * num_workers_;
    int32_t cutoff_id = num_workers_ - 1 - (total_data_assigned - num_test_data);

    int32_t data_idx_begin = 0;
    int32_t data_idx_end = 0;

    if (my_id_ <= cutoff_id) {
      data_idx_begin = num_data_per_thread * my_id_;
      data_idx_end = data_idx_begin + num_data_per_thread;
    } else if (my_id_ == cutoff_id + 1) {
      data_idx_begin = num_data_per_thread * my_id_;
      data_idx_end = data_idx_begin + num_data_per_thread - 1;
    } else {
      data_idx_begin = num_data_per_thread * cutoff_id
                       + (num_data_per_thread - 1) * (my_id_ - cutoff_id);
      data_idx_end = data_idx_begin + num_data_per_thread - 1;
    }
    num_test_data_ = data_idx_end - data_idx_begin;
    LOG(INFO) << "num_test_data = " << num_test_data_
              << " begin = " << data_idx_begin
              << " end = " << data_idx_end;
    test_data_.resize(num_test_data_);

    for (int i = data_idx_begin; i < data_idx_end; ++i) {
      test_data_[i - data_idx_begin]
          = std::make_pair(ada_grad.test_labels_[i], ada_grad.test_features_[i]);
    }
    LOG(INFO) << "load test data done, size = " << num_test_data_;
  }
}

void AdaGradLRWorker::ApplyUpdates() {
  std::lock_guard<std::mutex> lock(ada_grad_.mtx_);

  for (int i = 0; i < updates_.size(); ++i) {
    (*ada_grad_.weights_)[i] += updates_[i];
  }
  updates_.assign(feature_dim_, 0);
}

void AdaGradLRWorker::RefreshWeights() {
  std::lock_guard<std::mutex> lock(ada_grad_.mtx_);
  for (int i = 0; i < feature_dim_; ++i) {
    (*weights_)[i] = (*ada_grad_.weights_)[i];
  }
}

void *AdaGradLRWorker::operator() () {
  petuum::HighResolutionTimer train_timer;

  std::vector<float> predict_buff(num_labels_);
  float lr = FLAGS_init_lr;
  for (int epoch = 0; epoch < num_epochs_; ++epoch) {
    lr *= FLAGS_lr_decay;
    RefreshWeights();
    for (const auto &datum : data_) {
      const petuum::ml::AbstractFeature<float>& feature
          = *(datum.second);
      int32_t label = datum.first;
      Predict(feature, &predict_buff);

      {
        std::vector<float>& weights_vec = weights_->GetVector();
        float diff = predict_buff[0] - label;
        for (int i = 0; i < feature.GetNumEntries(); ++i) {
          int32_t fid = feature.GetFeatureId(i);
          float fval = feature.GetFeatureVal(i);
          if (fval == 0) continue;
          float gradient = diff * fval + FLAGS_lambda * weights_vec[fid];
          if (gradient == 0) continue;
          float update = lr * gradient;
          weights_vec[fid] -= update;
          updates_[fid] -= update;
        }
      }
    }
    ApplyUpdates();

    if (epoch % FLAGS_num_epochs_per_eval == 0) {
      RefreshWeights();
      float total_zero_one_loss = 0.;
      float total_entropy_loss = 0.;

      {
        for (const auto &datum : data_) {
          const petuum::ml::AbstractFeature<float>& feature
              = *(datum.second);
          int32_t label = datum.first;
          Predict(feature, &predict_buff);

          total_zero_one_loss += ZeroOneLoss(
              predict_buff, label);
          total_entropy_loss += CrossEntropyLoss(
              predict_buff, label);
        }
      }

      float test_total_zero_one_loss = 0.;
      float test_total_entropy_loss = 0.;
      if (num_test_data_ > 0) {
        for (const auto &datum : test_data_) {
          //LOG(INFO) << datum.second;
          const petuum::ml::AbstractFeature<float>& feature
              = *(datum.second);
          int32_t label = datum.first;
          Predict(feature, &predict_buff);

          test_total_zero_one_loss += ZeroOneLoss(
              predict_buff, label);
          test_total_entropy_loss += CrossEntropyLoss(
              predict_buff, label);
        }
      }

      {
        std::lock_guard<std::mutex> lock(ada_grad_.loss_mtx_);
        ada_grad_.entropy_loss_[epoch] += total_entropy_loss;
        ada_grad_.zero_one_loss_[epoch] += total_zero_one_loss;
        ada_grad_.test_entropy_loss_[epoch] += test_total_entropy_loss;
        ada_grad_.test_zero_one_loss_[epoch] += test_total_zero_one_loss;
      }

      if (epoch % num_workers_ == my_id_) {
        ada_grad_.reg_loss_[epoch] += EvaluateL2RegLoss();
      }

      process_barrier_.wait();

      if (epoch % num_workers_ == my_id_) {
        std::ofstream loss_stream(FLAGS_loss_file, std::ios_base::app);
        CHECK(loss_stream.is_open());
        loss_stream << "Train "
                    << epoch << " " << train_timer.elapsed() << " "
                    << ada_grad_.zero_one_loss_[epoch] / num_total_data_ << " "
                    << ada_grad_.entropy_loss_[epoch] / num_total_data_ << " "
                    << ada_grad_.entropy_loss_[epoch] / num_total_data_ + ada_grad_.reg_loss_[epoch]
                    << " "
                    << "Test "
                    << ada_grad_.test_zero_one_loss_[epoch] / num_total_test_data_ << " "
                    << ada_grad_.test_entropy_loss_[epoch] / num_total_test_data_ << " "
                    << std::endl;
        loss_stream.close();

        LOG(INFO) << "Train "
                  << epoch << " " << train_timer.elapsed() << " "
                  << ada_grad_.zero_one_loss_[epoch] / num_total_data_ << " "
                  << ada_grad_.entropy_loss_[epoch] / num_total_data_ << " "
                  << ada_grad_.entropy_loss_[epoch] / num_total_data_ + ada_grad_.reg_loss_[epoch]
                  << " "
                  << "Test "
                  << ada_grad_.test_zero_one_loss_[epoch] / num_total_test_data_ << " "
                  << ada_grad_.test_entropy_loss_[epoch] / num_total_test_data_ << " "
                  << std::endl;
      }

    }
  }
  return 0;
}

void AdaGradLRWorker::Predict(
    const petuum::ml::AbstractFeature<float>& feature,
    std::vector<float> *result) const {
  std::vector<float> &y_vec = *result;
  // fastsigmoid is numerically unstable for input <-88.
  y_vec[0] = petuum::ml::Sigmoid(
      petuum::ml::SparseDenseFeatureDotProduct(
          feature, *weights_));
  y_vec[1] = 1 - y_vec[0];
}

int32_t AdaGradLRWorker::ZeroOneLoss(
    const std::vector<float>& prediction, int32_t label) {
    // prediction[0] is the probability of being in class 1
    return prediction[label] >= 0.5 ? 1 : 0;
}

float AdaGradLRWorker::CrossEntropyLoss(
    const std::vector<float>& prediction, int32_t label) {
  CHECK_LE(prediction[label], 1);
  CHECK_GE(prediction[label], 0);
  // prediction[0] is the probability of being in class 1
  float prob = (label == 0) ? prediction[1] : prediction[0];
  return -petuum::ml::SafeLog(prob);
}

float AdaGradLRWorker::EvaluateL2RegLoss() const {
  double l2_norm = 0.;
  std::vector<float> w = weights_->GetVector();
  for (int i = 0; i < feature_dim_; ++i) {
    l2_norm += w[i] * w[i];
  }

  return 0.5 * FLAGS_lambda * l2_norm;
}

AdaGradLR::AdaGradLR() { }

AdaGradLR::~AdaGradLR() {
  if (weights_) delete weights_;
}

void AdaGradLR::Train(const std::string &train_file,
                      int num_epochs,
                      const std::string &test_file) {

  entropy_loss_.resize(num_epochs, 0);
  zero_one_loss_.resize(num_epochs, 0);
  reg_loss_.resize(num_epochs, 0);
  test_entropy_loss_.resize(num_epochs, 0);
  test_zero_one_loss_.resize(num_epochs, 0);
  boost::barrier process_barrier(FLAGS_num_worker_threads);
  LoadData(train_file, test_file);
  weights_ = new petuum::ml::DenseFeature<float>(feature_dim_, 0);

  AdaGradLRWorker *worker[FLAGS_num_worker_threads];

  for (int i = 0; i < FLAGS_num_worker_threads; ++i) {
    worker[i] = new AdaGradLRWorker(FLAGS_num_worker_threads,
                                    i,
                                    FLAGS_num_epochs,
                                    *this,
                                    process_barrier);
  }

  LOG(INFO) << "Created threads";

  for (auto &wptr : worker) {
    wptr->Start();
  }

  for (auto &wptr : worker) {
    wptr->Join();
  }
}

void AdaGradLR::DumpModel() { }

void AdaGradLR::LoadData(const std::string &train_file,
                         const std::string &test_file) {
  std::string meta_file = train_file + ".meta";
  petuum::ml::MetafileReader mreader(meta_file);
  num_data_ = mreader.get_int32("num_train_this_partition");

  feature_dim_ = mreader.get_int32("feature_dim");
  num_labels_ = mreader.get_int32("num_labels");
  std::string read_format = mreader.get_string("format");

  bool feature_one_based = mreader.get_bool("feature_one_based");
  bool label_one_based = mreader.get_bool("label_one_based");
  bool snappy_compressed = mreader.get_bool("snappy_compressed");

  CHECK (read_format == "libsvm");

  petuum::ml::ReadDataLabelLibSVM(
      train_file, feature_dim_, num_data_,
      &features_, &labels_, feature_one_based,
      label_one_based, snappy_compressed);

  if (test_file != "") {
    std::string test_meta_file = test_file + ".meta";
    petuum::ml::MetafileReader mreader_test(test_meta_file);
    num_test_data_ = mreader_test.get_int32("num_test");
    CHECK_EQ(feature_dim_, mreader_test.get_int32("feature_dim"));
    CHECK_EQ(num_labels_, mreader_test.get_int32("num_labels"));

    petuum::ml::ReadDataLabelLibSVM(
        test_file, feature_dim_,
        num_test_data_, &test_features_, &test_labels_);
  } else {
    num_test_data_ = 0;
  }
}

void AdaGradLR::CleanFeatures() {
  for (const auto &fptr : features_) {
    delete fptr;
  }
  features_.clear();

  for (const auto &fptr : test_features_) {
    delete fptr;
  }
  test_features_.clear();
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;
  AdaGradLR adagrad_lr;
  adagrad_lr.Train(FLAGS_train_file, FLAGS_num_epochs, FLAGS_test_file);
  LOG(INFO) << "total time = " << total_timer.elapsed();
  return 0;
}
