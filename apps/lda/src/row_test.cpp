#include <petuum_ps_common/storage/sorted_vector_map_row.hpp>
#include <glog/logging.h>

using namespace petuum;

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "test start";

  SortedVectorMapRow<int32_t> row, row2;
  row.Init(0);
  int32_t delta = 10;

  row.ApplyInc(1, &delta);
  LOG(INFO) << "Get " << row[1];

  delta = 2;
  row.ApplyInc(13, &delta);
  LOG(INFO) << "Get " << row[13];

  LOG(INFO) << "Get " << row[2];

  row.ApplyInc(112, &delta);
  row.ApplyInc(22, &delta);
  row.ApplyInc(13, &delta);
  delta = -10;
  row.ApplyInc(1, &delta);

  std::vector<Entry<int32_t> > vec(1000);
  row.CopyToVector(&vec);

  LOG(INFO) << "Now iterate";
  for (const auto &vec_iter : vec) {
    LOG(INFO) << vec_iter.first << " "
              << vec_iter.second;
  }

  size_t ssize = row.SerializedSize();
  uint8_t *mem = new uint8_t[ssize];
  row.Serialize(mem);

  row2.ResetRowData(mem, ssize);

  row2.CopyToVector(&vec);

  LOG(INFO) << "Now iterate";
  for (const auto &vec_iter : vec) {
    LOG(INFO) << vec_iter.first << " "
              << vec_iter.second;
  }

  delete[] mem;

  return 0;
}
