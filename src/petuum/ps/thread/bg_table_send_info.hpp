#pragma once

#include <petuum/ps_common/util/high_resolution_timer.hpp>

namespace petuum {

class BgTableSendInfo {
public:
  BgTableSendInfo():
      clock_has_pushed_(-1),
      last_send_wait_time_(0) { }

  BgTableSendInfo(const BgTableSendInfo & other):
      clock_has_pushed_(other.clock_has_pushed_),
      last_send_wait_time_(other.last_send_wait_time_) { }

  BgTableSendInfo & operator = (const BgTableSendInfo &other) {
    clock_has_pushed_ = other.clock_has_pushed_;
    last_send_wait_time_ = other.last_send_wait_time_;
    return *this;
  }

  int32_t get_clock_has_pushed() {
    return clock_has_pushed_;
  }

  void set_clock_has_pushed(int32_t clock_has_pushed) {
    clock_has_pushed_ = clock_has_pushed;
  }

  void timer_restart() {
    msg_send_timer_.restart();
  }

  double timer_elapsed() {
    return msg_send_timer_.elapsed();
  }

  double get_last_send_wait_time() {
    return last_send_wait_time_;
  }

  void set_last_send_wait_time(double last_send_wait_time) {
    last_send_wait_time_ = last_send_wait_time;
  }

private:
  int32_t clock_has_pushed_;
  double last_send_wait_time_;
  HighResolutionTimer msg_send_timer_;
};

}
