#pragma once

#include <petuum/ps_common/index/abstract_index.hpp>

namespace petuum {

class Search {
  template<template<typename> StorageT, typename ObjT>
  static ObjT const &Find(const AbstractIndex *index, const StorageT<Obj> &storage,
                      int32_t row_id, int32_t *idx) {
    *idx = index->Translate(row_id);
    return storage.Get(*idx);
  }
};
}
