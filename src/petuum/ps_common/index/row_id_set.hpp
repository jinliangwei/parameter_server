#pragma once

#include <petuum/include/configs.hpp>
#include <vector>

namespace petuum {

struct RowIDSetHeader {
  IndexType type;
  size_t num_ids;
  void SetNthID(int32_t n, int32_t row_id) {
    uint8_t *ids_mem = reinterpret_cast<uint8_t*>(this) + sizeof(RowIDSetHeader);
    int32_t *ids = reinterpret_cast<int32_t*>(ids_mem);
    ids[n] = row_id;
  }
};

class RowIDSetParser {
public:
  static size_t get_num_ids(const RowIDSetHeader* row_id_set_header) {
    return row_id_set_header->num_ids;
  }

  static int32_t get_nth_id(int32_t n,
                            const RowIDSetHeader* row_id_set_header) {
    int32_t id = 0;
    switch (row_id_set_header->type) {
      case Dense:
        id = n;
        break;
      case OffsetDense:
        {
          size_t offset = *(reinterpret_cast<const size_t*>(
              reinterpret_cast<const uint8_t*>(row_id_set_header)
              + sizeof(RowIDSetHeader)));
          id = n + offset;
        }
        break;
      case Sparse:
        {
          const int32_t *ids
              = reinterpret_cast<const int32_t*>(
                  reinterpret_cast<const uint8_t*>(row_id_set_header)
                  + sizeof(RowIDSetHeader));
          id = ids[n];
        }
        break;
      default:
        LOG(FATAL) << "Unkonwn RowIDSetType";
    }
    return id;
  }

private:
  const uint8_t *row_id_set_;
};

class RowIDSetFactory {
public:
  static RowIDSetHeader *CreateRowIDSet(IndexType type, size_t num_ids) {
    RowIDSetHeader *header = 0;
    switch(type) {
      case Dense:
        header = reinterpret_cast<RowIDSetHeader*>(
            new uint8_t[sizeof(RowIDSetHeader)]);
        break;
      case OffsetDense:
        header = reinterpret_cast<RowIDSetHeader*>(
            new uint8_t[sizeof(RowIDSetHeader) + sizeof(size_t)]);
        break;
      case Sparse:
        header = reinterpret_cast<RowIDSetHeader*>(
            new uint8_t[sizeof(RowIDSetHeader) + num_ids * sizeof(int32_t)]);
        break;
      default:
        LOG(FATAL) << "unknown type";
    }
    header->type = type;
    header->num_ids = num_ids;
    return header;
  }

  static void FreeRowIDSet(RowIDSetHeader *row_id_set) {
    delete[] reinterpret_cast<uint8_t*>(row_id_set);
  }
};

}
