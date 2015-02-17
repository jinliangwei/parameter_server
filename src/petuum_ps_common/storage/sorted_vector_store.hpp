#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <type_traits>
#include <boost/noncopyable.hpp>

#include <petuum_ps_common/storage/entry.hpp>
#include <petuum_ps_common/storage/abstract_store.hpp>

namespace petuum {

// WARNING: THIS IS INCOMPLETE! DOES NOT WORK!!

// SortedVectorStore stores pairs of (int32_t, V) in an array, sorted on
// int32_t (the weight) in descending order. Like map, it supports unbounded
// number of items through dynamic memory allocation.
template<typename V>
class SortedVectorStore : public AbstractStore<V> {
public:
  SortedVectorStore();
  ~SortedVectorStore();

  SortedVectorStore(const SortedVectorStore<V> &other);
  SortedVectorStore<V> & operator = (const SortedVectorStore<V> & other);

  void Init(size_t capacity);
  size_t SerializedSize() const;
  size_t Serialize(void *bytes) const;

  void Deserialize(const void *data, size_t num_bytes);

  V Get(int32_t key) const;
  void Inc(int32_t key, V delta);

  const void Copy(void *to) const;

  // Number of (non-zero) entries in the underlying vector.
  size_t num_entries() const;
  size_t capacity() const;

  // Any change made to the list makes the iterator invalid.
  class ConstIterator {
  public:
    ConstIterator(Entry<V> *begin, Entry<V> *end):
        entry_ptr_(begin),
        end_ptr_(end) { }

    ~ConstIterator() { }

    ConstIterator (const ConstIterator &other):
        entry_ptr_(other.entry_ptr_),
        end_ptr_(other.end_ptr_) { }

    ConstIterator& operator = (const ConstIterator &other) {
      entry_ptr_ = other.entry_ptr_;
      end_ptr_ = other.end_ptr_;
    }

    Entry<V> operator*();

    ConstIterator operator++();

    bool is_end();

  private:
    Entry<V> *entry_ptr_;
    Entry<V> *end_ptr_;
  };

  ConstIterator CBegin();

private:
  // Return ture if successfully Inc-ed.
  // If false:
  // 1) *size > capacity_: need to expand and retry
  // 2) *size < capacity_: shrink and copy
  bool Inc(int32_t key, V delta, size_t *size);

  // Return vector index of associated with column_id, or -1 if not found.
  int32_t FindIndex(int32_t key) const;

  // Move vector_idx forward (or backward) to maintain descending order.
  void LinearSearchAndMove(int32_t vector_idx, bool forward);

  // Remove vector_idx and shift the rest forward.
  size_t RemoveOneEntryAndCompact(int32_t vector_idx);

  static size_t GetSerializedNumEntries(size_t num_bytes);

  static bool ShouldCompact(size_t capacity, size_t num_entries);

  static const size_t kBlockSize;

  // Array of sorted entries.
  std::unique_ptr<Entry<V>[]> entries_;

  // # of entries in entries_.
  size_t num_entries_;

  // # of entries entries_ can hold.
  size_t capacity_;
};

// ================ Implementation =================

template<typename V>
const size_t SortedVectorStore<V>::kBlockSize = 64;

template<typename V>
SortedVectorStore<V>::SortedVectorStore():
    num_entries_(0),
    capacity_(0) {
  LOG(INFO) << "INCOMPLETE!!";
}

template<typename V>
SortedVectorStore<V>::~SortedVectorStore() { }

template<typename V>
SortedVectorStore<V>::SortedVectorStore(const SortedVectorStore<V> &other) :
    entries_ (new Entry<V>[other.capacity_]) ,
    num_entries_(other.num_entries_),
    capacity_(other.capacity_) {
  memcpy (entries_.get(), other.entries_.get(), num_entries_*sizeof(Entry<V>));
}

template<typename V>
SortedVectorStore<V> & SortedVectorStore<V>::operator =
(const SortedVectorStore<V> &other) {
  entries_.reset(new Entry<V>[other.capacity_]);
  num_entries_ = other.num_entries_;
  capacity_ = other.capacity_;
  memcpy (entries_.get(), other.entries_.get(), num_entries_*sizeof(Entry<V>));
  return *this;
}

template<typename V>
void SortedVectorStore<V>::Init(size_t capacity) {
  entries_.reset(new Entry<V>[capacity]);
  capacity_ = capacity;
  num_entries_ = 0;
}

template<typename V>
size_t SortedVectorStore<V>::SerializedSize() const {
  return num_entries_ * sizeof(Entry<V>);
}

template<typename V>
size_t SortedVectorStore<V>::Serialize(void *bytes) const {
  size_t num_bytes = SerializedSize();
  memcpy(bytes, entries_.get(), num_bytes);
  return num_bytes;
}

template<typename V>
void SortedVectorStore<V>::Deserialize(const void *data, size_t num_bytes) {
  size_t new_num_entries = GetSerializedNumEntries(num_bytes);
  if (new_num_entries > capacity_
      || ShouldCompact(capacity_, new_num_entries)) {
    Init(new_num_entries);
  }

  memcpy(entries_.get(), data, num_bytes);
  num_entries_ = new_num_entries;
}

template<typename V>
V SortedVectorStore<V>::Get(int32_t key) const {
  int32_t vector_idx = FindIndex(key);
  if (vector_idx == -1)
    return V(0);
  return entries_[vector_idx].second;
}

template<typename V>
void SortedVectorStore<V>::Inc(int32_t key, V delta) {
  size_t new_size = 0;
  bool suc = Inc(key, delta, &new_size);
  if (!suc) {
    if (new_size > capacity_) {
      Entry<V> *new_entries = new Entry<V>[new_size];
      memcpy(new_entries, entries_.get(), sizeof(Entry<V>)*num_entries_);

      entries_.reset(new_entries);
      capacity_ = new_size;

      CHECK(Inc(key, delta, &new_size));
    } else {
      CHECK_LE(new_size, capacity_);

      Entry<V> *new_entries = new Entry<V>[num_entries_];
      memcpy(new_entries, entries_.get(), sizeof(Entry<V>)*num_entries_);

      entries_.reset(new_entries);
      capacity_ = num_entries_;
    }
  }
}

template<typename V>
const void SortedVectorStore<V>::Copy(void *to) const {
  std::vector<Entry<V> > *vec
      = reinterpret_cast<std::vector<Entry<V> >* >(to);
  vec->resize(num_entries_);
  memcpy(vec->data(), entries_.get(), num_entries_*sizeof(Entry<V>));
}

template<typename V>
size_t SortedVectorStore<V>::num_entries() const {
  return num_entries_;
}

template<typename V>
size_t SortedVectorStore<V>::capacity() const {
  return capacity_;
}

template<typename V>
typename SortedVectorStore<V>::ConstIterator SortedVectorStore<V>::CBegin() {
  return ConstIterator(entries_.get(),
                       entries_.get() + sizeof(Entry<V>) * num_entries_);
}

// ================ Private Methods =================

template<typename V>
size_t SortedVectorStore<V>::GetSerializedNumEntries(size_t num_bytes) {
  return num_bytes / sizeof(Entry<V>);
}

template<typename V>
bool SortedVectorStore<V>::ShouldCompact(size_t capacity, size_t num_entries) {
  return (capacity - num_entries >= 2 * kBlockSize);
}

template<typename V>
int32_t SortedVectorStore<V>::FindIndex(int32_t key) const {
  for (int i = 0; i < num_entries_; ++i) {
    if (entries_[i].first == key) {
      return i;
    }
  }
  return -1;
}

template<typename V>
void SortedVectorStore<V>::LinearSearchAndMove(int32_t vector_idx,
    bool forward) {
  if (forward) {
    // The correct index for vector_idx; initially assume vector_idx doesn't
    // need to be moved.
    int32_t new_idx = vector_idx;
    V& val = entries_[vector_idx].second;
    for (int i = vector_idx + 1; i < num_entries_; ++i) {
      if (val < entries_[i].second) {
        new_idx = i;
      } else {
        break;
      }
    }
    if (new_idx > vector_idx) {
      // Do the move.
      Entry<V> tmp = entries_[vector_idx];
      memmove(entries_.get() + vector_idx, entries_.get() + vector_idx + 1,
          (new_idx - vector_idx) * sizeof(Entry<V>));
      entries_[new_idx] = tmp;
    }
  } else {
    // Move backward
    int32_t new_idx = vector_idx;
    V& val = entries_[vector_idx].second;
    for (int i = vector_idx - 1; i >= 0; --i) {
      if (val > entries_[i].second) {
        new_idx = i;
      } else {
        break;
      }
    }
    if (new_idx < vector_idx) {
      // Do the move.
      Entry<V> tmp = entries_[vector_idx];
      memmove(entries_.get() + new_idx + 1, entries_.get() + new_idx,
          (vector_idx - new_idx) * sizeof(Entry<V>));
      entries_[new_idx] = tmp;
    }
  }
}

template<typename V>
size_t SortedVectorStore<V>::RemoveOneEntryAndCompact(int32_t vector_idx) {
  memmove(entries_.get() + vector_idx, entries_.get() + vector_idx + 1,
          (num_entries_ - vector_idx - 1) * sizeof(Entry<V>));
  --num_entries_;

  // Compact criterion.
  if (ShouldCompact(capacity_, num_entries_)) {
    size_t new_capacity = num_entries_;
    int32_t remainder = new_capacity % kBlockSize;
    new_capacity += (remainder != 0) ? kBlockSize - remainder : 0;
    return new_capacity;
  }
  return capacity_;
}

template<typename V>
bool SortedVectorStore<V>::Inc(int32_t key, V delta, size_t *size) {
  if (delta == V(0)) return true;
  int32_t vector_idx = FindIndex(key);
  // Not found, insert it.
  if (vector_idx == -1) {
    if (num_entries_ == capacity_) {
      *size = capacity_ + kBlockSize;
      return false;
    }
    vector_idx = num_entries_;
    ++num_entries_;
    entries_[vector_idx].first = key;
    entries_[vector_idx].second = delta;

    // move backwards
    LinearSearchAndMove(vector_idx, false);

    return true;
  }

  // Found
  entries_[vector_idx].second += delta;

  if (entries_[vector_idx].second == V(0)) {
    size_t compact_size = RemoveOneEntryAndCompact(vector_idx);
    if (compact_size != capacity_) {
      *size = compact_size;
      return false;
    }
  }

  return true;
}

template<typename V>
Entry<V> SortedVectorStore<V>::ConstIterator::operator*() {
  return *entry_ptr_;
}

template<typename V>
typename SortedVectorStore<V>::ConstIterator
SortedVectorStore<V>::ConstIterator::operator++() {
  entry_ptr_ += 1;
}

template<typename V>
bool SortedVectorStore<V>::ConstIterator::is_end() {
  return (entry_ptr_ >= end_ptr_);
}

}  // namespace petuum
