#pragma once

namespace petuum {
struct RowMeta {
  int32_t row_id;
  int32_t clock;
  double importance;
  int32_t version;
};
}
