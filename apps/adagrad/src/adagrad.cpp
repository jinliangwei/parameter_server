#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/noncopyable.hpp>
#include <string>
#include <fstream>
#include <math.h>

#include <petuum_ps_common/util/high_resolution_timer.hpp>
#include <ml/include/ml.hpp>
#include <ml/feature/sparse_feature.hpp>

DEFINE_string(train_file, "", "train");
DEFINE_string(test_file, "", "test");
DEFINE_int32(num_epochs, 1, "num epochs");
DEFINE_int32(num_epochs_per_eval, 1, "num epochs");
DEFINE_double(init_lr, 1, "init lr");
DEFINE_double(lr_decay, 0.99, "decay rate");
DEFINE_string(loss_file, "", "");

class AdaGradLR : public boost::noncopyable {
public:
  AdaGradLR();
  ~AdaGradLR();

  void Train(const std::string &train_file, int num_epochs,
             const std::string &test_file);
  void Test();
  void DumpModel();

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

  void LoadData(const std::string &train_file,
                const std::string &test_file);
  void CleanFeatures();

  // data
  size_t feature_dim_;
  size_t num_labels_;
  size_t num_data_;
  size_t num_test_data_;
  std::vector<std::pair<
                int32_t,
                petuum::ml::AbstractFeature<float>*> > data_;

  std::vector<std::pair<
                int32_t,
                petuum::ml::AbstractFeature<float>*> > test_data_;

  petuum::ml::DenseFeature<float> *weights_;
  std::vector<float> hist_gradients_;
};

AdaGradLR::AdaGradLR() { }
AdaGradLR::~AdaGradLR() {
  if (weights_) delete weights_;
}

void AdaGradLR::Train(const std::string &train_file,
                      int num_epochs,
                      const std::string &test_file) {
  petuum::HighResolutionTimer train_timer;

  LoadData(train_file, test_file);
  weights_ = new petuum::ml::DenseFeature<float>(feature_dim_, 0);
  hist_gradients_.resize(feature_dim_);

  std::vector<float> predict_buff(num_labels_);
  float lr = FLAGS_init_lr;
  for (int epoch = 0; epoch < num_epochs; ++epoch) {
    lr *= FLAGS_lr_decay;
    int j = 0;
    for (const auto &datum : data_) {
      const petuum::ml::AbstractFeature<float>& feature
          = *(datum.second);
      int32_t label = datum.first;
      Predict(feature, &predict_buff);

      {
        std::vector<float>& weights_vec = weights_->GetVector();
        float diff = predict_buff[0] - label;
        if (diff == 0) {
          LOG(INFO) << "data = " << j
                    << " predict = " << predict_buff[0];
          LOG(INFO) << weights_->ToString();
        }
        for (int i = 0; i < feature.GetNumEntries(); ++i) {
          int32_t fid = feature.GetFeatureId(i);
          float fval = feature.GetFeatureVal(i);
          if (fval == 0) continue;

          float gradient = diff * fval;
          hist_gradients_[fid] += pow(gradient, 2);
          float update = FLAGS_init_lr/sqrt(hist_gradients_[fid]) * gradient;
          CHECK(update == update) << j << " " << hist_gradients_[fid] << " " << diff
                                  << " " << fval;
          weights_vec[fid] -= update;
          CHECK(weights_vec[fid] == weights_vec[fid]);
        }
      }
      ++j;
    }

    if (epoch % FLAGS_num_epochs_per_eval == 0) {
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
      if (test_file != "" ) {
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
      std::ofstream loss_stream(FLAGS_loss_file, std::ios_base::app);
      CHECK(loss_stream.is_open());
      loss_stream << "Train "
                  << epoch << " " << train_timer.elapsed() << " "
                  << total_zero_one_loss << " " << total_entropy_loss
                  << " Test "
                  << test_total_zero_one_loss << " " << test_total_entropy_loss
                  << std::endl;
      loss_stream.close();

      LOG(INFO) << "Train "
                << epoch << " " << train_timer.elapsed() << " "
                << total_zero_one_loss << " " << total_entropy_loss
                << " Test "
                << test_total_zero_one_loss << " " << test_total_entropy_loss;
    }
  }

  CleanFeatures();
}

void AdaGradLR::Test() { }

void AdaGradLR::DumpModel() { }

void AdaGradLR::Predict(
    const petuum::ml::AbstractFeature<float>& feature,
    std::vector<float> *result) const {
  std::vector<float> &y_vec = *result;
  // fastsigmoid is numerically unstable for input <-88.
  y_vec[0] = petuum::ml::Sigmoid(
      petuum::ml::SparseDenseFeatureDotProduct(
          feature, *weights_));
  y_vec[1] = 1 - y_vec[0];
}

int32_t AdaGradLR::ZeroOneLoss(
    const std::vector<float>& prediction, int32_t label) {
    // prediction[0] is the probability of being in class 1
    return prediction[label] >= 0.5 ? 1 : 0;
}

float AdaGradLR::CrossEntropyLoss(
    const std::vector<float>& prediction, int32_t label) {
    CHECK_LE(prediction[label], 1);
    CHECK_GE(prediction[label], 0);
    // prediction[0] is the probability of being in class 1
    float prob = (label == 0) ? prediction[1] : prediction[0];
    return -petuum::ml::SafeLog(prob);
  }

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

  std::vector<int32_t> labels;
  std::vector<petuum::ml::AbstractFeature<float> *> features;

  CHECK (read_format == "libsvm");

  petuum::ml::ReadDataLabelLibSVM(
      train_file, feature_dim_, num_data_,
      &features, &labels, feature_one_based,
      label_one_based, snappy_compressed);

  data_.resize(num_data_);

  for (int i = 0; i < num_data_; ++i) {
    data_[i] = std::make_pair(labels[i], features[i]);
  }

  if (test_file != "") {
    std::string test_meta_file = test_file + ".meta";
    petuum::ml::MetafileReader mreader_test(test_meta_file);
    num_test_data_ = mreader_test.get_int32("num_test");
    CHECK_EQ(feature_dim_, mreader_test.get_int32("feature_dim"));
    CHECK_EQ(num_labels_, mreader_test.get_int32("num_labels"));

    std::vector<int32_t> test_labels;
    std::vector<petuum::ml::AbstractFeature<float> *> test_features;

    petuum::ml::ReadDataLabelLibSVM(
        test_file, feature_dim_,
        num_test_data_, &test_features, &test_labels);

    test_data_.resize(num_test_data_);

    for (int i = 0; i < num_test_data_; ++i) {
      test_data_[i] = std::make_pair(test_labels[i], test_features[i]);
    }
  }
}

void AdaGradLR::CleanFeatures() {
  for (const auto &datum : data_) {
    delete datum.second;
  }
  data_.clear();

  for (const auto &datum : test_data_) {
    delete datum.second;
  }
  test_data_.clear();
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
