// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.10.11
//
// Description: Generate multi-class synthetic data from correlation
// mechanism and linear separator. The output is in libsvm format.
//
// Specifically, we start from first column, generate non zero values for
// `nnz_per_col` randomly selected rows. Going to the second column, with
// probability `correlation_strength` we use the same set of rows to have
// non-zero values at the second column; otherwise we randomly pick another
// set of `nnz_per_col` rows. After the rows are chosen, we generate
// `nnz_per_col` feature values by adding uniform rand numbers to each of the
// `nnz_per_col` feature values in column 1. Repeat for column 3 etc.
//
// After the features are generated, we assign labels based on the regression
// value of each data with respect to a randomly generated linear regressor.
// To have 10 classes, the data with the top 10% highest regressed value
// assigned label 1, then 10-20% are assigned label 2, etc.

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <petuum_ps_common/include/petuum_ps.hpp>
#include "string_buffer.hpp"
#include <fstream>
#include <cstdio>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <random>
#include <cstdint>
#include <snappy.h>

DEFINE_int32(num_train, 10000, "# of training data.");
DEFINE_int32(feature_dim, 2000, "feature dimension.");
DEFINE_int32(num_labels, 2, "# of classes.");
DEFINE_string(output_file, " ", "Generate "
    "output_file, output_file.meta");
DEFINE_int32(num_partitions, 1, "# of output files. Output files are"
    "output_file.0, output_file.1 ...");
DEFINE_int32(nnz_per_col, 25, "# of non-zeros in each column.");
DEFINE_bool(one_based, false, "feature index starts at 1. Only relevant to "
    "libsvm format.");
DEFINE_double(correlation_strength, 0.9, "correlation strength in [0, 1].");
DEFINE_string(format, "libsvm", "options: 1) libsvm; 2) sparse_feature_binary.");
DEFINE_bool(snappy_compressed, false, "compress with snappy.");
DEFINE_bool(output_spark_format, true, "Output additionally for spark, "
    "i.e., none compress, no partition.");
DEFINE_double(noise_ratio, 0.1, "With this probability an instance label is "
    "randomly assigned.");
//DEFINE_int32(num_data_per_write, 200000, "# data points per write to file. "
//    "Too big will blow up memory");

namespace {

// Sample t items from {0, 1, ..., n - 1} without replacement.
std::vector<int> SampleWithoutReplacement(int n, int t) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> unif(0, n - 1);
  std::vector<int> sample_idx(t, -1);
  for (int num_sampled = 0; num_sampled < t; ) {
    int sample = unif(gen);
    int repeat = false;
    for (int i = 0; i < num_sampled; ++i) {
      if (sample_idx[i] == sample) {
        repeat = true;
        break;
      }
    }
    if (!repeat) {
      sample_idx[num_sampled++] = sample;
    }
  }
  return sample_idx;
}

class SparseFeature {
public:
  SparseFeature() : ids_(default_size_), vals_(default_size_), nnz_(0) { }
  SparseFeature(int size) : ids_(size), vals_(size), nnz_(0) { }

  static void SetDefaultSize(int size) {
    default_size_ = size;
  }

  void PushBack(int id, float val) {
    if (nnz_ == ids_.size()) {
      ids_.resize(ids_.size() * 2);
      vals_.resize(vals_.size() * 2);
    }
    ids_[nnz_] = id;
    vals_[nnz_] = val;
    ++nnz_;
  }

  const std::vector<int>& GetFeatureIds() {
    ids_.resize(nnz_);
    return ids_;
  }

  const std::vector<float>& GetFeatureVals() {
    vals_.resize(nnz_);
    return vals_;
  }

public:
  static int default_size_;

private:
  std::vector<int> ids_;
  std::vector<float> vals_;
  int nnz_;   // # of non-zeros in the sparse feature.
};

int SparseFeature::default_size_ = 10;

float SparseDotProduct(const std::vector<float>& beta,
    SparseFeature& x) {
  float sum = 0.;
  const std::vector<int>& ids = x.GetFeatureIds();
  const std::vector<float>& vals = x.GetFeatureVals();
  for (int i = 0; i < ids.size(); ++i) {
    int feature_id = ids[i];
    sum += beta[feature_id] * vals[i];
  }
  return sum;
}

// Quantile is the label (first quantile is label 0, etc).
int FindQuantile(const std::vector<float>& quantiles, float val) {
  for (int q = 0; q < FLAGS_num_labels; ++q) {
    if (val <= quantiles[q]) {
      return q;
    }
  }
  LOG(FATAL) << "value exceeds quantile: " << val;
  return 0;
}


void GenerateMetaFile(const std::string& filename, int num_train_this_partition) {
  // Generate meta file
  std::ofstream meta_stream(filename + ".meta", std::ofstream::out
      | std::ofstream::trunc);
  meta_stream << "num_train_total: " << FLAGS_num_train << std::endl;
  meta_stream << "num_train_this_partition: "
    << num_train_this_partition << std::endl;
  meta_stream << "feature_dim: " << FLAGS_feature_dim << std::endl;
  meta_stream << "num_labels: " << FLAGS_num_labels << std::endl;
  meta_stream << "format: " << FLAGS_format  << std::endl;
  meta_stream << "feature_one_based: " << FLAGS_one_based << std::endl;
  meta_stream << "label_one_based: " << false << std::endl;
  meta_stream << "nnz_per_column: " << FLAGS_nnz_per_col << std::endl;
  meta_stream << "correlation_strength: " << FLAGS_correlation_strength << std::endl;
  meta_stream << "snappy_compressed: " << FLAGS_snappy_compressed << std::endl;
  meta_stream.close();
}


}  // anonymous namespace

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;

  // i indexs samples, j indexes feature dim, k indexes non-zero values.


  // X[i] is the ith sample. map is 0-based.
  int est_nnz_per_sample = std::ceil(static_cast<float>(FLAGS_nnz_per_col) *
      FLAGS_feature_dim / FLAGS_num_train) * 1.1;
  SparseFeature::SetDefaultSize(est_nnz_per_sample);
  std::vector<SparseFeature> X(FLAGS_num_train);

  // 0 <= sample_idx[k] < FLAGS_num_train. sample_idx[k] is the sample idx that
  // will get the k-th generated value. This gets shuffle with 10% probability
  // going to the next feature.
  // Generate first column by picking some samples to be non-zeros.
  std::vector<int> sample_idx =
    SampleWithoutReplacement(FLAGS_num_train, FLAGS_nnz_per_col);

  // Previous values. Never shuffled.
  std::vector<float> prev_val(FLAGS_nnz_per_col);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> unif_minus_plus_one(-1., 1.);

  // Generate nnz_per_col values.
  for (int k = 0; k < FLAGS_nnz_per_col; ++k) {
    prev_val[k] = unif_minus_plus_one(gen);
    int sample_k = sample_idx[k];
    //X[sample_k][0] = prev_val[k];
    X[sample_k].PushBack(0, prev_val[k]);
  }

  for (int j = 1; j < FLAGS_feature_dim; ++j) {
    LOG_EVERY_N(INFO, 100000) << "Progress: " << j << " / " << FLAGS_feature_dim;
    if (unif_minus_plus_one(gen) >= FLAGS_correlation_strength) {
      // shuffle (no correlation).
      sample_idx =
        SampleWithoutReplacement(FLAGS_num_train, FLAGS_nnz_per_col);
    }
    for (int k = 0; k < FLAGS_nnz_per_col; ++k) {
      prev_val[k] += unif_minus_plus_one(gen);   // add to existing value as corelation.
      int sample_k = sample_idx[k];
      //X[sample_k][j] = prev_val[k];
      X[sample_k].PushBack(j, prev_val[k]);
    }
  }

  // Compute labels.
  // Generate \beta.
  std::vector<float> beta(FLAGS_feature_dim);
  for (int j = 0; j < FLAGS_feature_dim; ++j) {
    beta[j] = unif_minus_plus_one(gen);
  }

  std::vector<float> regress_val(FLAGS_num_train);
  for (int i = 0; i < FLAGS_num_train; ++i) {
    regress_val[i] = SparseDotProduct(beta, X[i]);
  }

  std::vector<float> regress_sorted = regress_val;
  std::sort(regress_sorted.begin(), regress_sorted.end());
  // Find quantiles
  std::vector<float> quantiles(FLAGS_num_labels);
  for (int l = 0; l < FLAGS_num_labels; ++l) {
    quantiles[l] = (l == FLAGS_num_labels - 1) ?
      regress_sorted[regress_sorted.size() - 1] :
      regress_sorted[(l + 1) * FLAGS_num_train / FLAGS_num_labels];
  }
  std::vector<int> labels(FLAGS_num_train);
  std::uniform_int_distribution<> class_rand(0, FLAGS_num_labels - 1);
  std::uniform_real_distribution<float> unif_zero_one(0., 1.);
  for (int i = 0; i < FLAGS_num_train; ++i) {
    if (unif_zero_one(gen) < FLAGS_noise_ratio) {
      // Noise label.
      labels[i] = class_rand(gen);
    } else {
      // Linearly separable labeling.
      labels[i] = FindQuantile(quantiles, regress_val[i]);
    }
  }
  LOG(INFO) << "Generated " << FLAGS_num_train << " data of dim "
    << FLAGS_feature_dim << " in " << total_timer.elapsed();

  // Write to file.
  float avg_nnz_per_sample = (static_cast<float>(FLAGS_feature_dim)
      * FLAGS_nnz_per_col / FLAGS_num_train);
  int num_data_per_partition = std::ceil(static_cast<float>(FLAGS_num_train)
      / FLAGS_num_partitions);
  if (FLAGS_format == "libsvm") {
    LOG(INFO) << "Writing to libsvm format.";
    for (int ipar = 0; ipar < FLAGS_num_partitions; ++ipar) {
      int lower_bound = ipar * num_data_per_partition;
      int upper_bound = std::min((ipar + 1) * num_data_per_partition,
          FLAGS_num_train);
      int num_train_this_partition = upper_bound - lower_bound;
      std::stringstream ss;
      for (int i = lower_bound; i < upper_bound; ++i) {
        ss << labels[i] << " ";
        const std::vector<int>& ids = X[i].GetFeatureIds();
        const std::vector<float>& vals = X[i].GetFeatureVals();
        for (int j = 0; j < ids.size(); ++j) {
          int feature_id = FLAGS_one_based ? ids[j] + 1 : ids[j];
          ss << feature_id << ":" << vals[j] << " ";
        }
        ss << std::endl;
      }
      std::string filename = FLAGS_output_file + "x"
        + std::to_string(FLAGS_num_partitions) + ".libsvm."
        + std::to_string(ipar);
      if (FLAGS_snappy_compressed) {
        // Snappy compress.
        std::string compressed_str;
        std::string original_str = ss.str();
        snappy::Compress(original_str.data(), original_str.size(), &compressed_str);
        std::ofstream outfile(filename, std::ofstream::binary);
        outfile.write(compressed_str.data(), compressed_str.size());
        LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
          << filename << " (" << compressed_str.size() << " bytes)";
      } else {
        std::ofstream outfile(filename);
        std::string out_str = ss.str();
        outfile.write(out_str.data(), out_str.size());
        LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
          << filename << " (" << out_str.size() << " bytes)";
      }
      GenerateMetaFile(filename, num_train_this_partition);
    }
  } else if (FLAGS_format == "sparse_feature_binary") {
    CHECK(!FLAGS_snappy_compressed) << "sparse_feature_binary format does "
      "not support snappy.";
    LOG(INFO) << "Writing to sparse_feature_binary format.";
    petuum::HighResolutionTimer write_timer;
    std::vector<int32_t> feature_ids(FLAGS_feature_dim);
    std::vector<float> feature_vals(FLAGS_feature_dim);
    for (int ipar = 0; ipar < FLAGS_num_partitions; ++ipar) {
      int64_t num_bytes_this_partition = 0;
      int64_t lower_bound = ipar * num_data_per_partition;
      int64_t upper_bound = std::min((ipar + 1) * num_data_per_partition,
          FLAGS_num_train);
      LOG(INFO) << "Writing to partition " << ipar << " data " << lower_bound
        << " to " << upper_bound;
      int64_t num_train_this_partition = upper_bound - lower_bound;
      std::string filename = FLAGS_output_file + "x"
        + std::to_string(FLAGS_num_partitions) + ".sfb." + std::to_string(ipar);
      //std::ofstream outfile(filename, std::ofstream::binary);
      FILE* fp = fopen(filename.c_str(), "wb");
      for (int64_t i = lower_bound; i < upper_bound; ++i) {
        const std::vector<int>& ids = X[i].GetFeatureIds();
        const std::vector<float>& vals = X[i].GetFeatureVals();
        int32_t nnz = (int32_t) ids.size();
        fwrite(reinterpret_cast<char*>(&nnz), sizeof(int32_t), 1, fp);
        fwrite(reinterpret_cast<char*>(&labels[i]), sizeof(int32_t), 1, fp);
        //outfile.write(reinterpret_cast<char*>(&nnz), sizeof(int32_t));
        //outfile.write(reinterpret_cast<char*>(&labels[i]), sizeof(int32_t));
        for (int j = 0; j < ids.size(); ++j) {
          int feature_id = FLAGS_one_based ? ids[j] + 1 : ids[j];
          CHECK_GT(feature_id, -1);
          feature_ids[j] = feature_id;
          feature_vals[j] = vals[j];
        }
        if (ids.size() > 0) {
          fwrite(reinterpret_cast<char*>(feature_ids.data()), sizeof(int32_t), nnz, fp);
          fwrite(reinterpret_cast<char*>(feature_vals.data()), sizeof(float), nnz, fp);
          //outfile.write(reinterpret_cast<char*>(feature_ids.data()), nnz * sizeof(int32_t));
          //outfile.write(reinterpret_cast<char*>(feature_vals.data()), nnz * sizeof(float));
        }
      }
      LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
        << filename << " (" << num_bytes_this_partition << " bytes) in " << write_timer.elapsed();
      GenerateMetaFile(filename, num_train_this_partition);
    }
  }

  {
    /// DW: hack hack hack!
    int num_partitions = 36;
    num_data_per_partition = std::ceil(static_cast<float>(FLAGS_num_train)
    / num_partitions);
    LOG(INFO) << "Writing to sparse_feature_binary format (" << num_partitions << " partitions).";
    petuum::HighResolutionTimer write_timer;
    std::vector<int32_t> feature_ids(FLAGS_feature_dim);
    std::vector<float> feature_vals(FLAGS_feature_dim);
    for (int ipar = 0; ipar < num_partitions; ++ipar) {
      int lower_bound = ipar * num_data_per_partition;
      int upper_bound = std::min((ipar + 1) * num_data_per_partition,
          FLAGS_num_train);
      int num_train_this_partition = upper_bound - lower_bound;
      std::stringstream ss;
      for (int i = lower_bound; i < upper_bound; ++i) {
        ss << labels[i] << " ";
        const std::vector<int>& ids = X[i].GetFeatureIds();
        const std::vector<float>& vals = X[i].GetFeatureVals();
        for (int j = 0; j < ids.size(); ++j) {
          int feature_id = FLAGS_one_based ? ids[j] + 1 : ids[j];
          ss << feature_id << ":" << vals[j] << " ";
        }
        ss << std::endl;
      }
      std::string filename = FLAGS_output_file + "x"
        + std::to_string(num_partitions) + ".libsvm."
        + std::to_string(ipar);
      if (FLAGS_snappy_compressed) {
        // Snappy compress.
        std::string compressed_str;
        std::string original_str = ss.str();
        snappy::Compress(original_str.data(), original_str.size(), &compressed_str);
        std::ofstream outfile(filename, std::ofstream::binary);
        outfile.write(compressed_str.data(), compressed_str.size());
        LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
          << filename << " (" << compressed_str.size() << " bytes)";
      } else {
        std::ofstream outfile(filename);
        std::string out_str = ss.str();
        outfile.write(out_str.data(), out_str.size());
        LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
          << filename << " (" << out_str.size() << " bytes)";
      }
      GenerateMetaFile(filename, num_train_this_partition);
    }
  }


  if (FLAGS_output_spark_format) {
    LOG(INFO) << "Writing to Spark format (libsvm).";
    petuum::HighResolutionTimer write_timer;
    std::string filename = FLAGS_output_file + ".spark";
    FILE* outfile = fopen(filename.c_str(), "w");
    for (int i = 0; i < FLAGS_num_train; ++i) {
      fprintf(outfile, "%d ", labels[i]);
      const std::vector<int>& ids = X[i].GetFeatureIds();
      const std::vector<float>& vals = X[i].GetFeatureVals();
      int32_t nnz = (int32_t) ids.size();
      for (int j = 0; j < nnz; ++j) {
        int feature_id = FLAGS_one_based ? ids[j] + 1 : ids[j];
        fprintf(outfile, "%d:%f ", feature_id, vals[j]);
      }
      fprintf(outfile, "\n");
    }
    CHECK_EQ(0, fclose(outfile)) << "Failed to close file " << FLAGS_output_file;
    LOG(INFO) << "Wrote " << FLAGS_num_train << " data to file "
      << filename << " in " << write_timer.elapsed();
  }

  return 0;
}
/*
   {
/// DW: hack hack hack!
int num_partitions = 8;
num_data_per_partition = std::ceil(static_cast<float>(FLAGS_num_train)
/ num_partitions);
LOG(INFO) << "Writing to sparse_feature_binary format (8 partitions).";
petuum::HighResolutionTimer write_timer;
std::vector<int32_t> feature_ids(FLAGS_feature_dim);
std::vector<float> feature_vals(FLAGS_feature_dim);
for (int ipar = 0; ipar < num_partitions; ++ipar) {
int lower_bound = ipar * num_data_per_partition;
int upper_bound = std::min((ipar + 1) * num_data_per_partition,
FLAGS_num_train);
int num_train_this_partition = upper_bound - lower_bound;
int64_t est_nnz = num_train_this_partition * (FLAGS_feature_dim *
FLAGS_nnz_per_col / FLAGS_num_train) * 1.15; // give 15% error.
int64_t est_size = est_nnz * 2;  // id|val pairs
est_size +=  2 * FLAGS_num_train;  // label|nnz for each instance.
LOG(INFO) << "est_size: " << est_size;
petuum::StringBuffer sb(est_size);
for (int i = lower_bound; i < upper_bound; ++i) {
const std::vector<int>& ids = X[i].GetFeatureIds();
const std::vector<float>& vals = X[i].GetFeatureVals();
int32_t nnz = (int32_t) ids.size();
sb.Write(reinterpret_cast<char*>(&nnz), sizeof(int32_t));
sb.Write(reinterpret_cast<char*>(&labels[i]), sizeof(int32_t));
for (int j = 0; j < ids.size(); ++j) {
int feature_id = FLAGS_one_based ? ids[j] + 1 : ids[j];
feature_ids[j] = feature_id;
feature_vals[j] = vals[j];
}
if (ids.size() > 0) {
sb.Write(reinterpret_cast<char*>(feature_ids.data()), nnz * sizeof(int32_t));
sb.Write(reinterpret_cast<char*>(feature_vals.data()), nnz * sizeof(float));
}
}
std::string filename = FLAGS_output_file + "x"
+ std::to_string(num_partitions) + "." + std::to_string(ipar);
std::ofstream outfile(filename, std::ofstream::binary);
if (FLAGS_snappy_compressed) {
std::string compressed_str;
const std::vector<char>& original_str = sb.ToChars();
snappy::Compress(original_str.data(), original_str.size(), &compressed_str);
outfile.write(compressed_str.data(), compressed_str.size());
LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
<< filename << " (" << compressed_str.size() << " bytes) in "
<< write_timer.elapsed();
} else {
// Break into 10MB chuncks to write.
const int chunk_size = 1e8;
const std::vector<char>& output_str = sb.ToChars();
int num_bytes = output_str.size();
int num_chunks = std::ceil(static_cast<float>(num_bytes) / chunk_size);
for (int iwrite = 0; iwrite < num_chunks - 1; ++iwrite) {
outfile.write(output_str.data() + iwrite * chunk_size, chunk_size);
}
// last chunk could be smaller than chunk_size
int last_chunk_size = num_bytes - (num_chunks - 1) * chunk_size;
outfile.write(output_str.data() + (num_chunks - 1) * chunk_size, last_chunk_size);
LOG(INFO) << "Wrote " << upper_bound - lower_bound << " data to file "
<< filename << " (" << num_bytes << " bytes) in " << write_timer.elapsed();
}
GenerateMetaFile(filename, num_train_this_partition);
}
}
*/
