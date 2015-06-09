#pragma once

#include <petuum/ps_common/storage/dense_storage.hpp>

namespace petuum {

template<typename T>
class AppendableDenseStorage : public DenseStorage {
public:
  void Append(T const &obj) {
    vec_.push_back(obj);
  }
}
