#pragma once

#include <functional>
#include <vector>
#include <petuum/ps_common/util/noncopyable.hpp>

namespace petuum {

template<typename T>
class DenseStorage : NonCopyable {
public:
  DenseStorage(size_t capacity);

  DenseStorage(size_t capacity,
               std::function<T()> create_obj,
               std::function<void(T const &)> destroy_obj);

  DenseStorage(size_t capacity,
               std::function<T(int)> create_obj,
               std::function<void(T const &)> destroy_obj);

  virtual ~DenseStorage();

  T const & get(int32_t index);

protected:
  std::vector<T> vec_;
  std::function<void(T const &)> destroy_obj_;
};

template<typename T>
DenseStorage<T>::DenseStorage(size_t capacity):
vec_(capacity) { }

template<typename T>
DenseStorage<T>::DenseStorage(
    size_t capacity,
    std::function<T()> create_obj,
    std::function<void(T const &)> destroy_obj):
    vec_(capacity),
    destroy_obj_(destroy_obj) {
  for (auto &t : vec_) {
    t = create_obj();
  }
}

template<typename T>
DenseStorage<T>::DenseStorage(
    size_t capacity,
    std::function<T(int)> create_obj,
    std::function<void(T const &)> destroy_obj):
    vec_(capacity),
    destroy_obj_(destroy_obj) {
  int i = 0;
  for (auto &t : vec_) {
    t = create_obj(i);
    i++;
  }
}

template<typename T>
DenseStorage<T>::~DenseStorage() {
  for (auto &t : vec_) {
    destroy_obj_(t);
  }
}

template<typename T>
T const & DenseStorage<T>::get(int32_t index) {
  return vec_[index];
}

}
