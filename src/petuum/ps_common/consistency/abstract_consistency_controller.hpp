#pragma once

#include <petuum/ps_common/storage/storage_types.hpp>
#include <petuum/include/configs.hpp>
#include <petuum/ps_common/util/vector_clock_mt.hpp>

#include <boost/utility.hpp>
#include <cstdint>
#include <vector>

namespace petuum {

// Interface for consistency controller modules. For each table we associate a
// consistency controller (e.g., SSPController) that's essentially the "brain"
// that maintains a prescribed consistency policy upon each table action. All
// functions should be fully thread-safe.
class AbstractConsistencyController : boost::noncopyable {
public:
  // Controller modules rely on TableInfo to specify policy parameters (e.g.,
  // staleness in SSP). Does not take ownership of any pointer here.
  AbstractConsistencyController(
      int32_t table_id,
      ParameterCache& parameter_cache,
      const AbstractRow* sample_row) :
      parameter_cache_(parameter_cache),
      table_id_(table_id),
      sample_row_(sample_row) { }

  virtual ~AbstractConsistencyController() { }

  // Read a row in the table and is blocked until a valid row is obtained
  // (e.g., from server). A row is valid if, for example, it is sufficiently
  // fresh in SSP. The result is returned in row_accessor.
  virtual ClientRow *Get(int32_t row_id, RowAccessor* row_accessor) = 0;

  // Increment (update) an entry. Does not take ownership of input argument
  // delta, which should be of template type UPDATE in Table. This may trigger
  // synchronization (e.g., in value-bound) and is blocked until consistency
  // is ensured.
  virtual void Inc(int32_t row_id, int32_t column_id, const void* delta) = 0;

  // Increment column_ids.size() entries of a row. deltas points to an array.
  virtual void BatchInc(int32_t row_id, const int32_t* column_ids,
    const void* updates, int32_t num_updates) = 0;

  virtual void DenseBatchInc(int32_t row_id, const void *updates,
                             int32_t index_st, int32_t num_updates) = 0;

  virtual void Clock() = 0;

protected:    // common class members for all controller modules.
  // Process cache, highly concurrent.
  ParameterCache& parameter_cache_;

  int32_t table_id_;
};

}    // namespace petuum
