#pragma once

namespace petuum {
class NonCopyable {
public:
  NonCopyable() { }
private:
  NonCopyable(const NonCopyable &other) = delete;
  NonCopyable & operator = (const NonCopyable &other) = delete;
};

}
