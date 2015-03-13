// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2015.02.03

#pragma once

#include <petuum_ps_common/include/petuum_ps.hpp>
#include <ml/include/ml.hpp>
#include <cstdint>
#include <vector>
#include <functional>

namespace mlr {

class AbstractMLRSGDSolver {
public:
  virtual ~AbstractMLRSGDSolver() { }

  virtual void SetLearningRate(double learning_rate) {
    learning_rate_ = learning_rate;
  }

  // Compute gradient using feature and label and store internally.
  virtual void SingleDataSGD(const petuum::ml::AbstractFeature<float>& feature,
      int32_t label) = 0;

  // Predict the probability of each label.
  virtual void Predict(const petuum::ml::AbstractFeature<float>& feature,
      std::vector<float> *result) const = 0;

  // Return 0 if a prediction (of length num_labels_) correctly gives the
  // ground truth label 'label'; 0 otherwise.
  virtual int32_t ZeroOneLoss(const std::vector<float>& prediction, int32_t label)
    const = 0;

  // Compute cross entropy loss of a prediction (of length num_labels_) and the
  // ground truth label 'label'.
  virtual float CrossEntropyLoss(const std::vector<float>& prediction, int32_t label)
    const = 0;

  // Evaluate L2 regularization term \lambda ||w||^2
  virtual float EvaluateL2RegLoss() const = 0;

  // Write pending updates to PS and read new w_cache_. It will use either
  // RefreshParamDense() or RefreshParamSparse().
  virtual void RefreshParams() = 0;

  // Save the current weight in cache in libsvm format.
  virtual void SaveWeights(const std::string& filename) const = 0;

protected:
  double learning_rate_;
};

}  // namespace mlr
