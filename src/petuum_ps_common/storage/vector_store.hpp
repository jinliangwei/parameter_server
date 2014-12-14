// author: jinliang

#pragma once

#include <vector>
#include <glog/logging.h>

#include <petuum_ps_common/storage/abstract_store.hpp>
#include <petuum_ps_common/storage/abstract_store_iterator.hpp>
namespace petuum {
// V is an arithmetic type. V is the data type and also the update type.
// V needs to be POD.
// When Init(), memory is zero-ed out.
template<typename V>
class VectorStore : public AbstractStore<V> {
public:
  VectorStore();
  ~VectorStore();

  VectorStore(const VectorStore<V> &other);
  VectorStore<V> & operator = (const VectorStore<V> &other);

  // When init(), the memory is zeroed out.
  void Init(size_t capacity);
  size_t SerializedSize() const;
  size_t Serialize(void *bytes) const;
  void Deserialize(const void *data, size_t num_bytes);
  void ResetData(const void *data, size_t num_bytes);

  V Get (int32_t col_id) const;
  void Inc(int32_t col_id, V delta);

  size_t get_capacity() const;

  const void Copy(void *to) const;

  class Iterator : public AbstractIterator<V> {
  public:
    Iterator() { }
    ~Iterator() { }

    int32_t get_key();
    V & operator *();
    void operator ++ ();
    bool is_end();

    friend class VectorStore<V>;

  private:
    V *data_ptr_;
    int32_t col_id_;
    int32_t end_id_;
  };

  void RangeBegin(AbstractIterator<V> **iterator, int32_t begin,
                  int32_t end);
private:
  std::vector<V> data_;
  Iterator iter_;
};

template<typename V>
VectorStore<V>::VectorStore() { }

template<typename V>
VectorStore<V>::~VectorStore() { }

template<typename V>
VectorStore<V>::VectorStore(const VectorStore<V> &other):
    data_(other.data_) { }

template<typename V>
VectorStore<V> & VectorStore<V>::operator = (const VectorStore<V> &other) {
  data_ = other.data_;
  return *this;
}

template<typename V>
void VectorStore<V>::Init(size_t capacity) {
  data_.resize(capacity);
  memset(data_.data(), 0, capacity * sizeof(V));
}

template<typename V>
size_t VectorStore<V>::SerializedSize() const {
  return data_.size() * sizeof(V);
}

template<typename V>
size_t VectorStore<V>::Serialize(void *bytes) const {
  size_t num_bytes = data_.size()*sizeof(V);
  memcpy(bytes, data_.data(), num_bytes);
  return num_bytes;
}

template<typename V>
void VectorStore<V>::Deserialize(const void *data, size_t num_bytes) {
  size_t num_entries = num_bytes / sizeof(V);
  data_.resize(num_entries);
  memcpy(data_.data(), data, num_bytes);
}

template<typename V>
void VectorStore<V>::ResetData(const void *data, size_t num_bytes) {
  memcpy(data_.data(), data, num_bytes);
}

template<typename V>
V VectorStore<V>::Get (int32_t col_id) const {
  V v = data_[col_id];
  return v;
}

template<typename V>
void VectorStore<V>::Inc(int32_t col_id, V delta) {
  data_[col_id] += delta;
}

template<typename V>
size_t VectorStore<V>::get_capacity() const {
  return data_.size();
}

template<typename V>
const void VectorStore<V>::Copy(void *to) const {
  std::vector<V> *vec = reinterpret_cast<std::vector<V>*>(to);
  memcpy(vec->data(), data_.data(), data_.size()*sizeof(V));
}

template<typename V>
void VectorStore<V>::RangeBegin(AbstractIterator<V> **iterator, int32_t begin,
                                int32_t end) {
  iter_.data_ptr_ = data_.data() + begin;
  iter_.col_id_ = begin;
  iter_.end_id_ = (end < data_.size()) ? end : (data_.size() - 1);
  *iterator = &iter_;
}

template<typename V>
int32_t VectorStore<V>::Iterator::get_key() {
  return col_id_;
}

template<typename V>
V &VectorStore<V>::Iterator::operator *() {
  return *data_ptr_;
}

template<typename V>
void VectorStore<V>::Iterator::operator ++ () {
  ++col_id_;
  data_ptr_ += 1;
}

template<typename V>
bool VectorStore<V>::Iterator::is_end() {
  return col_id_ > end_id_;
}

}
