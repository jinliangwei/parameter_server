#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <petuum/ps_common/util/noncopyable.hpp>

namespace petuum {
class ParameterCacheClock : NonCopyable {
public:
  ParameterCacheClock():
      clock_(0) { }

  int32_t GetClock() {
    return static_cast<int32_t>(clock_.load());
  }

  void WaitUntilClock(int32_t clock) {
    std::unique_lock<std::mutex> lock(mtx_);
    // The bg threads might have advanced the clock after my last check.
    while (static_cast<int32_t>(clock_.load()) < clock) {
      cv_.wait(lock);
    }
  }

  void UpdateClock(int32_t new_clock) {
    clock_ = new_clock;
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.notify_all();
  }

private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::atomic_int_fast32_t clock_;
};

}
