#pragma once

#include <petuum/ps_common/storage/dense_storage.hpp>
#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/ps_common/oplog/abstract_row_oplog.hpp>
#include <petuum/ps_common/storage/row_meta.hpp>

#include <mutex>

namespace petuum {

typedef DenseStorage<AbstractRow*> ParameterCache;
typedef DenseStorage<std::pair<RowMeta, AbstractRowOpLog*>> UpdateBuffer;
typedef DenseStorage<std::mutex> LockEngine;
}
