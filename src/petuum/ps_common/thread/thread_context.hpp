#pragma once

#include <stdint.h>
#include <boost/noncopyable.hpp>

namespace petuum {

// In petuum PS, thread is treated as first-class citizen. Some globaly
// shared thread information, such as ID, are stored in static variable to
// avoid having passing some variables every where.
class ThreadContext {
public:
  static void RegisterThread(int32_t thread_id) {
    thr_info_ = new Info(thread_id);
  }

  static int32_t get_id() {
    return thr_info_->entity_id_;
  }

private:
  struct Info : boost::noncopyable {
    explicit Info(int32_t entity_id):
        entity_id_(entity_id) { }

    ~Info(){ }

    const int32_t entity_id_;
  };

  // We do not use thread_local here because there's a bug in
  // g++ 4.8.1 or lower: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55800
  static __thread Info *thr_info_;
};
}
