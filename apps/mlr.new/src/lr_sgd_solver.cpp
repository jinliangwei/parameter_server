// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.10.04

#include "lr_sgd_solver.hpp"
#include "common.hpp"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <petuum_ps_common/include/petuum_ps.hpp>
#include <ml/include/ml.hpp>
#include <ml/util/fastapprox/fastapprox.hpp>


namespace mlr {

LRSGDSolver::LRSGDSolver(const LRSGDSolverConfig& config) :
  w_table_(config.w_table),
  w_cache_(config.feature_dim),
  //w_delta_(config.feature_dim),
  w_last_refresh_(config.feature_dim),
  feature_dim_(config.feature_dim),
  w_table_num_cols_(config.w_table_num_cols), lambda_(config.lambda),
  predict_buff_(2) {    // 2 for binary (2 classes).
    if (config.sparse_data) {
      // Sparse feature, dense weight.
      FeatureDotProductFun_ = petuum::ml::SparseDenseFeatureDotProduct;
    } else {
      // Both weight and feature are dense
      FeatureDotProductFun_ = petuum::ml::DenseDenseFeatureDotProduct;
    }
  }

LRSGDSolver::~LRSGDSolver() { }

void LRSGDSolver::SingleDataSGD(const petuum::ml::AbstractFeature<float>& feature,
    int32_t label, double sample_lr) {
  Predict(feature, &predict_buff_);

  // Apply gradient to w_cache_
  if (!FLAGS_use_minibatch_lambda && lambda_ > 0) {
    w_cache_.Decay();
  }
  //petuum::ml::FeatureScaleAndAdd(-this->learning_rate_ * (predict_buff_[0] - label), feature,
  //    &w_cache_);
  petuum::ml::FeatureScaleAndAdd(-sample_lr * (predict_buff_[0] - label), feature,
      &w_cache_);
}

void LRSGDSolver::Predict(
    const petuum::ml::AbstractFeature<float>& feature,
    std::vector<float> *result) const {
  std::vector<float> &y_vec = *result;
  // fastsigmoid is numerically unstable for input <-88.
  y_vec[0] = petuum::ml::Sigmoid(FeatureDotProductFun_(feature, w_cache_));
  y_vec[1] = 1 - y_vec[0];
}

int32_t LRSGDSolver::ZeroOneLoss(const std::vector<float>& prediction, int32_t label)
  const {
    // prediction[0] is the probability of being in class 1
    return prediction[label] >= 0.5 ? 1 : 0;
  }

float LRSGDSolver::CrossEntropyLoss(const std::vector<float>& prediction, int32_t label)
  const {
    CHECK_LE(prediction[label], 1);
    CHECK_GE(prediction[label], 0);
    // prediction[0] is the probability of being in class 1
    float prob = (label == 0) ? prediction[1] : prediction[0];
    return -petuum::ml::SafeLog(prob);
  }

void LRSGDSolver::RefreshParams() {
  std::vector<float> w_delta(feature_dim_);
  std::vector<float>& w_cache_vec = w_cache_.GetVector();
  for (int i = 0; i < feature_dim_; ++i) {
    w_delta[i] = w_cache_vec[i] - w_last_refresh_[i];
  }
  // Write delta's to PS table.
  int num_full_rows = feature_dim_ / w_table_num_cols_;
  for (int i = 0; i < num_full_rows; ++i) {
    petuum::DenseUpdateBatch<float> w_update_batch(0, w_table_num_cols_);
    for (int j = 0; j < w_table_num_cols_; ++j) {
      int idx = i * w_table_num_cols_ + j;
      CHECK_EQ(w_delta[idx], w_delta[idx]) << "nan detected.";
      w_update_batch[j] = w_delta[idx];
    }
    w_table_.DenseBatchInc(i, w_update_batch);
  }

  if (feature_dim_ % w_table_num_cols_ != 0) {
    // last incomplete row.
    int num_cols_last_row = feature_dim_ - num_full_rows * w_table_num_cols_;
    petuum::DenseUpdateBatch<float> w_update_batch(0, num_cols_last_row);
    for (int j = 0; j < num_cols_last_row; ++j) {
      int idx = num_full_rows * w_table_num_cols_ + j;
      CHECK_EQ(w_delta[idx], w_delta[idx]) << "nan detected.";
      w_update_batch[j] = w_delta[idx];
    }
    w_table_.DenseBatchInc(num_full_rows, w_update_batch);
  }

  // Zero delta.
  //std::vector<float>& w_delta_vec = w_delta_.GetVector();
  //std::fill(w_delta_vec.begin(), w_delta_vec.end(), 0);

  //petuum::PSTableGroup::Clock();

  // Read w from the PS.
  std::vector<float> w_cache(w_table_num_cols_);
  for (int i = 0; i < num_full_rows; ++i) {
    petuum::RowAccessor row_acc;
    const auto& r = w_table_.Get<petuum::DenseRow<float>>(i, &row_acc);
    r.CopyToVector(&w_cache);
    std::copy(w_cache.begin(), w_cache.end(),
              w_cache_vec.begin() + i * w_table_num_cols_);
  }
  if (feature_dim_ % w_table_num_cols_ != 0) {
    // last incomplete row.
    int num_cols_last_row = feature_dim_ - num_full_rows * w_table_num_cols_;
    std::vector<float> w_cache(w_table_num_cols_);
    petuum::RowAccessor row_acc;
    const auto& r = w_table_.Get<petuum::DenseRow<float>>(num_full_rows, &row_acc);
    r.CopyToVector(&w_cache);
    std::copy(w_cache.begin(), w_cache.begin() + num_cols_last_row,
        w_cache_vec.begin() + num_full_rows * w_table_num_cols_);
  }
  w_last_refresh_ = w_cache_vec;
}

void LRSGDSolver::SaveWeights(const std::string& filename) const {
  LOG(ERROR) << "SaveWeights is not implemented for binary LR.";
}

// 1/2 * lambda * ||w||^2
float LRSGDSolver::EvaluateL2RegLoss() const {
  double l2_norm = 0.;
  std::vector<float> w = w_cache_.GetVector();
  for (int i = 0; i < feature_dim_; ++i) {
    l2_norm += w[i] * w[i];
  }
  return 0.5 * lambda_ * l2_norm;
}

void LRSGDSolver::Update(double batch_learning_rate) {
  w_cache_.SetDecayRate(1 - batch_learning_rate * lambda_);
  if (FLAGS_use_minibatch_lambda && lambda_ > 0) {
    w_cache_.Decay();
  }
}

}  // namespace mlr
