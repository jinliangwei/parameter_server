#include <iostream>
#include <fstream>
#include <boost/noncopyable.hpp>
#include <sstream>
#include <fstream>
#include <map>
#include <petuum_ps_common/storage/dense_row.hpp>
#include <petuum_ps_common/util/high_resolution_timer.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <petuum_ps_common/include/constants.hpp>

DEFINE_string(snapshot_dir, "", "snapshot directory");
DEFINE_int32(table_id, 0, "table id");
DEFINE_int32(clock, 0, "clock");
DEFINE_int32(num_clients, 1, "clock");
DEFINE_int32(num_comm_channels_per_client, 1, "clock");
DEFINE_string(output_textfile, "", "output textfile");

namespace petuum {

const int32_t kServerThreadIDStartOffset = 1;
const int32_t kMaxNumThreadsPerClient = 1000;

const int32_t kEndOfFile = -1;
const size_t kDiskBuffSize = 64*k1_Mi;

class SnapshotProcessor : boost::noncopyable {
public:
  SnapshotProcessor(int32_t num_clients,
                    int32_t num_comm_channels_per_client):
      num_clients_(num_clients),
      num_comm_channels_per_client_(num_comm_channels_per_client) { }

  ~SnapshotProcessor() {
    for (auto &row : table_) {
      delete row.second;
      row.second = 0;
    }
  }

  void LoadSnapshot(const std::string &snapshot_dir,
                    int32_t table_id, int32_t clock) {
    uint8_t *disk_buff = new uint8_t[kDiskBuffSize];
    size_t disk_buff_offset = 0;

    for (int i = 0; i < num_clients_; ++i) {
      for (int j = 0; j < num_comm_channels_per_client_; ++j) {
        int32_t server_id = kMaxNumThreadsPerClient*i
                            + kServerThreadIDStartOffset + j;
        std::string snapshot_filename;
        MakeSnapShotFileName(snapshot_dir, server_id, table_id, clock,
                             &snapshot_filename);

        std::ifstream snapshot_file;
        snapshot_file.open(snapshot_filename, std::ifstream::binary);
        CHECK(snapshot_file.good()) << snapshot_filename;

        while (!snapshot_file.eof()) {
          snapshot_file.read(reinterpret_cast<char*>(disk_buff), kDiskBuffSize);
          disk_buff_offset = 0;
          if (snapshot_file.eof()) break;

          while (disk_buff_offset + sizeof(int32_t) <= kDiskBuffSize) {
            int32_t row_id
                = *(reinterpret_cast<int32_t*>(disk_buff + disk_buff_offset));
            LOG(INFO) << "id = " << row_id;

            if (row_id == kEndOfFile) break;
            disk_buff_offset += sizeof(int32_t);
            size_t row_size
                = *(reinterpret_cast<size_t*>(disk_buff + disk_buff_offset));
            disk_buff_offset += sizeof(size_t);
            DenseRow<float> *row = new DenseRow<float>();
            row->Deserialize(disk_buff + disk_buff_offset, row_size);
            disk_buff_offset += row_size;
            table_.insert(std::make_pair(row_id, row));
          }
        }
      }
    }
    delete[] disk_buff;
  }

  void PrintTable() {
    std::string str;
    for (auto &row : table_) {
      const std::vector<float> *row_vec
          = reinterpret_cast<const std::vector<float>*>(row.second->GetDataPtr());
      RowToString(*row_vec, &str);
      LOG(INFO) << row.first
                << " " << str;
    }
  }

  void OutputTextFile(const std::string &path) {
    std::ofstream text_file;
    text_file.open(path);
    CHECK(text_file.good());
    std::string str;
    for (auto &row : table_) {
      const std::vector<float> *row_vec
          = reinterpret_cast<const std::vector<float>*>(row.second->GetDataPtr());
      RowToString(*row_vec, &str);
      text_file << row.first
                << " " << str << "\n";
    }
    text_file.close();
  }

private:
  static void MakeSnapShotFileName(
      const std::string &snapshot_dir,
      int32_t server_id, int32_t table_id, int32_t clock,
      std::string *filename) {

    std::stringstream ss;
    ss << snapshot_dir << "/server_table" << ".server-" << server_id
       << ".table-" << table_id << ".clock-" << clock
       << ".dat";
    *filename = ss.str();
  }

  static void RowToString(
      const std::vector<float> &row,
      std::string *str) {
    *str = "";
    for (int i = 0; i < row.size(); ++i) {
      *str += std::to_string(row[i]);
      *str += " ";
    }
  }

  const int32_t num_clients_;
  const int32_t num_comm_channels_per_client_;

  std::map<int32_t, DenseRow<float>*> table_;
};

}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;

  petuum::SnapshotProcessor snapshot_processor(
      FLAGS_num_clients, FLAGS_num_comm_channels_per_client);
  snapshot_processor.LoadSnapshot(
      FLAGS_snapshot_dir, FLAGS_table_id, FLAGS_clock);
  //snapshot_processor.PrintTable();

  if (FLAGS_output_textfile != "")
    snapshot_processor.OutputTextFile(FLAGS_output_textfile);
  LOG(INFO) << "good";
}
