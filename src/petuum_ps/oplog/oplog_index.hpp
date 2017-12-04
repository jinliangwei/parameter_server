#pragma once

#include <vector>
#include <libcuckoo/cuckoohash_map.hh>
#include <unordered_set>
#include <stdint.h>
#include <boost/noncopyable.hpp>

#include <petuum_ps_common/util/lock.hpp>
#include <petuum_ps_common/include/row_id.hpp>
#include <petuum_ps_common/util/striped_lock.hpp>
#include <petuum_ps/thread/context.hpp>

namespace petuum {
using OpLogRowIdSet = std::unordered_set<RowId>;
using SharedOpLogIndex = cuckoohash_map<RowId, bool>;

class PartitionOpLogIndex : boost::noncopyable {
public:
  explicit PartitionOpLogIndex(size_t capacity);
  PartitionOpLogIndex(PartitionOpLogIndex && other);
  PartitionOpLogIndex & operator = (PartitionOpLogIndex && other) = delete;

  ~PartitionOpLogIndex();
  void AddIndex(const OpLogRowIdSet &oplog_index);
  SharedOpLogIndex *Reset();
  size_t GetNumRowOpLogs();
private:
  size_t capacity_;
  SharedMutex smtx_;
  StripedLock<RowId> locks_;
  SharedOpLogIndex *shared_oplog_index_;
};

class TableOpLogIndex : boost::noncopyable{
public:
  explicit TableOpLogIndex(size_t capacity);
  void AddIndex(int32_t partition_num,
                const OpLogRowIdSet &oplog_index);
  SharedOpLogIndex *ResetPartition(int32_t partition_num);
  size_t GetNumRowOpLogs(int32_t partition_num);
private:
  std::vector<PartitionOpLogIndex> partition_oplog_index_;
};
}
