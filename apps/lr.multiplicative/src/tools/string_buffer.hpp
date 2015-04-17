#pragma once
#include <string>
#include <vector>


namespace petuum {

class StringBuffer {
public:
  // Size is for reservation. Could be expanded dynamically.
  StringBuffer(size_t size = 10);

  // Write 'size' bytes / char from ptr.
  void Write(const char* ptr, size_t size);

  // Return string with right size.
  std::string str() const;

  // Return the underlying char vector.
  const std::vector<char>& ToChars();

private:
  std::vector<char> str_;
  int pos_;

  // How fast does memory grow every time we exceeds capcity.
  static const float kExpansionFactor;
};

}  // namespace petuum
