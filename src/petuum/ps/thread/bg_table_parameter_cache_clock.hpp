#pragma once

#include <petuum/ps_common/util/vector_clock.hpp>
#include <petuum/ps_common/util/vector_clock_mt.hpp>
#include <petuum/ps_common/util/noncopyable.hpp>
#include <petuum/ps_common/thread/thread_context.hpp>

namespace petuum {
class BgTableParameterCacheClock {
public:
  BgTableParameterCacheClock(std::vector<int32_t> server_ids,
                             VectorClockMT *bg_server_clock):
      bg_server_clock_(bg_server_clock),
      server_clock_(server_ids) { }

  BgTableParameterCacheClock(const BgTableParameterCacheClock & other):
      bg_server_clock_(other.bg_server_clock_),
      server_clock_(other.server_clock_) { }

  BgTableParameterCacheClock & operator = (const BgTableParameterCacheClock & other) {
    bg_server_clock_ = other.bg_server_clock_;
    server_clock_ = other.server_clock_;
    return *this;
  }

  int32_t SetServerClock(int32_t server_id, int32_t clock) {
    int32_t new_clock = server_clock_.TickUntil(server_id, clock);

    if (new_clock)
      return bg_server_clock_->TickUntil(ThreadContext::get_id(), new_clock);
    return 0;
  }

private:
  // shared with other instances in other threads.
  VectorClockMT *bg_server_clock_;
  VectorClock server_clock_;
};
}
