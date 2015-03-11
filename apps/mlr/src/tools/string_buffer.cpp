#include "string_buffer.hpp"
#include <algorithm>
#include <glog/logging.h>
#include <cstdint>

namespace petuum {

const float StringBuffer::kExpansionFactor = 2;

StringBuffer::StringBuffer(size_t size) : str_(size), pos_(0) {
}

void StringBuffer::Write(const char* ptr, size_t size) {
  if (pos_ + size > str_.size()) {
    int64_t curr_size = str_.size();
    int64_t new_size = std::max(static_cast<int64_t>(pos_ + size),
        static_cast<int64_t>(kExpansionFactor * curr_size));
    LOG_EVERY_N(INFO, 1000) << "StringBuffer expanding from " << curr_size << " bytes to "
      << new_size << " bytes.";
    str_.resize(new_size);
  }
  std::copy(ptr, ptr + size, &str_[pos_]);
  pos_ += size;
}

std::string StringBuffer::str() const {
  return std::string(str_.data(), pos_);
}

const std::vector<char>& StringBuffer::ToChars() {
  str_.resize(pos_);
  return str_;
}

}  // namespace petuum
