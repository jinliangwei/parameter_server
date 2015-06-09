#pragma once

namespace petuum {

// Index provides a lightweight translation between row ID and the index to the
// underlying storage.
// For rows that exist in the storage, the translation returns a meaningful
// index to the storage. Otherwise, the index is out of the range of the
// storage.

class AbstractIndex {
public:
  virtual ~AbstractIndex() { }
  virtual int32_t Translate(int32_t row_id) const = 0;
  virtual size_t GetSizeOfSerializedRowIDs() const = 0;
  virtual size_t SerializeRowIDs(void *mem) const = 0;
  virtual bool CheckInRange(int32_t row_id) const = 0;
};

}
