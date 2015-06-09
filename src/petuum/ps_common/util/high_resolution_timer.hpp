#pragma once

#include <time.h>
#include <petuum/ps_common/util/noncopyable.hpp>

namespace petuum {

// This is a simpler implementation of timer to replace
// boost::high_resolution_timer. Code based on
// http://boost.2283326.n4.nabble.com/boost-shared-mutex-performance-td2659061.html
class HighResolutionTimer : NonCopyable {
  public:
  HighResolutionTimer();

  HighResolutionTimer(HighResolutionTimer && other):
      total_time_(other.total_time_),
      start_time_(other.start_time_) { }

  void restart();

  // return elapsed time (including previous restart-->pause time) in seconds.
  double elapsed() const;

  // return estimated maximum value for elapsed()
  double elapsed_max() const;

  // return minimum value for elapsed()
  double elapsed_min() const;

 private:
  double total_time_;
  struct timespec start_time_;
};

} // namespace petuum
