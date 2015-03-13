#include <ml/include/ml.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <fstream>
#include <stdio.h>

DEFINE_uint64(num_train_samples, 0, "number data samples?");
DEFINE_uint64(num_test_samples, 0, "number data samples?");
DEFINE_string(data_file, "", "data file");

std::vector<petuum::ml::AbstractFeature<float>*> features_;
std::vector<int32_t> labels_;
size_t feature_dim_;
std::string read_format_;
bool feature_one_based_;
bool label_one_based_;
bool snappy_compressed_;
size_t num_labels_;
size_t num_data_;

void ReadDataML(const std::string &filename) {
  std::string meta_file = filename + ".meta";
  petuum::ml::MetafileReader mreader(meta_file);

  num_data_ = mreader.get_int32("num_train_this_partition");
  feature_dim_ = mreader.get_int32("feature_dim");
  num_labels_ = mreader.get_int32("num_labels");
  read_format_ = mreader.get_string("format");
  feature_one_based_ = mreader.get_bool("feature_one_based");
  label_one_based_ = mreader.get_bool("label_one_based");
  snappy_compressed_ = mreader.get_bool("snappy_compressed");

  //CHECK_EQ(num_labels_, num_data_);

  features_.resize(num_data_);
  for (auto &feature : features_) {
    feature = new petuum::ml::DenseFeature<float>(feature_dim_);
  }

  labels_.resize(num_data_);

  if (read_format_ == "bin") {
    petuum::ml::ReadDataLabelBinary(
        filename, feature_dim_,
        num_data_, &features_, &labels_);
  } else if (read_format_ == "libsvm") {
    petuum::ml::ReadDataLabelLibSVM(
        filename, feature_dim_, num_data_,
        &features_, &labels_, feature_one_based_,
        label_one_based_, snappy_compressed_);
  } else {
    LOG(FATAL) << "Unknown data format " << read_format_;
  }
}

void OutputSubset(size_t num_train_samples,
                  size_t num_test_samples,
                  const std::string &filename) {
  {
    std::string output_filename
        = filename + "." + std::to_string(num_train_samples)
        + "." + "train";
    FILE *subset_output = fopen(output_filename.c_str(), "wb");
    if (subset_output == 0) {
      LOG(FATAL) << "Failed to open " << output_filename;
    }

    for (int i = 0; i < num_train_samples; ++i) {
      fwrite(&(labels_[i]), sizeof(int32_t), 1, subset_output);
      fwrite(static_cast<petuum::ml::DenseFeature<float>*>(
          features_[i])->GetVector().data(), sizeof(float), feature_dim_,
             subset_output);
    }
    fclose(subset_output);

    std::string output_filename_meta
        = output_filename + ".meta";
    std::ofstream meta_stream(output_filename_meta, std::ofstream::out
                              | std::ofstream::trunc);
    meta_stream << "num_train_total: " << num_train_samples << std::endl;
    meta_stream << "num_train_this_partition: "
                << num_train_samples << std::endl;
    meta_stream << "feature_dim: " << feature_dim_ << std::endl;
    meta_stream << "num_labels: " << num_labels_ << std::endl;
    meta_stream << "format: " << "bin" << std::endl;
    meta_stream << "feature_one_based: 0" << std::endl;
    meta_stream << "label_one_based: 0" << std::endl;
    meta_stream << "snappy_compressed: 0" << std::endl;
    meta_stream.close();
  }

  {
    std::string output_filename
        = filename + "." + std::to_string(num_test_samples)
        + "." + "test";
    FILE *subset_output = fopen(output_filename.c_str(), "wb");
    if (subset_output == 0) {
      LOG(FATAL) << "Failed to open " << output_filename;
    }

    for (int i = num_train_samples; i < num_train_samples; ++i) {
      fwrite(&(labels_[i]), sizeof(int32_t), 1, subset_output);
      fwrite(static_cast<petuum::ml::DenseFeature<float>*>(
          features_[i])->GetVector().data(), sizeof(float), feature_dim_,
             subset_output);
    }
    fclose(subset_output);

    std::string output_filename_meta
        = output_filename + ".meta";
    std::ofstream meta_stream(output_filename_meta, std::ofstream::out
                              | std::ofstream::trunc);
    meta_stream << "num_test: " << num_test_samples << std::endl;
    meta_stream << "feature_dim: " << feature_dim_ << std::endl;
    meta_stream << "num_labels: " << num_labels_ << std::endl;
    meta_stream << "format: " << "bin" << std::endl;
    meta_stream << "feature_one_based: 0" << std::endl;
    meta_stream << "label_one_based: 0" << std::endl;
    meta_stream << "snappy_compressed: 0" << std::endl;
    meta_stream.close();
  }
}

void ClearFeatures() {
  for (auto &feature : features_) {
    delete feature;
    feature = 0;
  }
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  ReadDataML(FLAGS_data_file);
  LOG(INFO) << "Read data done";
  OutputSubset(FLAGS_num_train_samples,
               FLAGS_num_test_samples,
               FLAGS_data_file);
  return 0;
}
