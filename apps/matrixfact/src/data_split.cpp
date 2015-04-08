#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <limits>
#include <random>
#include <set>
#include <thread>
#include <boost/thread/barrier.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

// Command-line flags
DEFINE_string(datafile, "", "Input sparse matrix");
DEFINE_string(data_format, "list", "list, mmt or libsvm");
DEFINE_int32(num_partitions, 1, "number of partitions");

// Data variables
size_t N_, M_; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)
std::vector<int> row_counts;
std::vector<int> X_row; // Row index of each nonzero entry in the data matrix
std::vector<int> X_col; // Column index of each nonzero entry in the data matrix
std::vector<float> X_val; // Value of each nonzero entry in the data matrix

// Read the full data set in libsvm format.
void ReadLibsvmData(const std::string& file) {
  char *line = NULL, *ptr = NULL, *endptr = NULL;
  size_t num_bytes;
  FILE *data_stream = fopen(file.c_str(), "r");
  CHECK_NOTNULL(data_stream);
  int base = 10;

  // Read first line: #-rows/users #-columns/movies
  CHECK_NE(-1, getline(&line, &num_bytes, data_stream));
  N_ = strtol(line, &endptr, base);
  ptr = endptr + 1;   // +1 to skip the space.
  M_ = strtol(ptr, &endptr, base);

  int nnz = 0;  // number of non-zero entries.
  int row_id = 0;
  while (getline(&line, &num_bytes, data_stream) != -1) {
    int nnz_line = strtol(line, &endptr, base);
    //LOG(INFO) << "nnz " << nnz_line;
    int curr_nnz = 0;
    ptr = endptr;
    while (curr_nnz < nnz_line) {
      // read a word_id:count pair
      int col_id = strtol(ptr, &endptr, base);
      //LOG(INFO) << col_id;
      ptr = endptr; // *ptr = colon
      CHECK_EQ(':', *ptr);

      float val = strtof(++ptr, &endptr);
      //LOG(INFO) << val;
      ptr = endptr;
      X_row.push_back(row_id);
      X_col.push_back(col_id);
      X_val.push_back(val);
      row_counts.push_back(row_id);
      ++nnz;
      ++curr_nnz;
      //LOG(INFO) << "curr_nnz = " << curr_nnz;

      //while (*ptr == ' ') ++ptr; // goto next non-space char
    }
    CHECK_EQ(nnz_line, curr_nnz);
    ++row_id;
  }
  fclose(data_stream);
}

// Read sparse data matrix into X_row, X_col and X_val. Each line of the matrix
// is a whitespace-separated triple (row,col,value), where row>=0 and col>=0.
// For example:
//
// 0 0 0.5
// 1 2 1.5
// 2 1 2.5
//
// This specifies a 3x3 matrix with 3 nonzero elements: 0.5 at (0,0), 1.5 at
// (1,2) and 2.5 at (2,1).
void ReadSparseMatrix(std::string inputfile) {
  X_row.clear();
  X_col.clear();
  X_val.clear();
  N_ = 0;
  M_ = 0;
  int row_count = 0;
  int curr_row = -1;
  std::ifstream inputstream(inputfile.c_str());
  CHECK(inputstream) << "Failed to read " << inputfile;
  while(true) {
    int row, col;
    float val;
    inputstream >> row >> col >> val;
    if (!inputstream) {
      break;
    }
    if (row != curr_row) {
      row_count++;
      curr_row = row;
    }
    X_row.push_back(row);
    X_col.push_back(col);
    X_val.push_back(val);
    row_counts.push_back(row_count - 1);

    N_ = row+1 > N_ ? row+1 : N_;
    M_ = col+1 > M_ ? col+1 : M_;
  }
  inputstream.close();
}

// read a MMT matrix in coordinate format.
void ReadMmtData(const std::string &inputfile) {
  LOG(FATAL) << "Not yet supported";
  X_row.clear();
  X_col.clear();
  X_val.clear();
  char *line = NULL, *ptr = NULL, *endptr = NULL;
  size_t num_bytes;
  FILE *data_stream = fopen(inputfile.c_str(), "r");
  CHECK_NOTNULL(data_stream);
  int base = 10, nnz;

  int suc = getline(&line, &num_bytes, data_stream);
  CHECK_NE(suc, -1);

  // skip the banner
  while (line[0] == '%') {
    suc = getline(&line, &num_bytes, data_stream);
    CHECK_NE(suc, -1);
  }

  N_ = strtol(line, &endptr, base);
  ptr = endptr;
  M_ = strtol(ptr, &endptr, base);
  ptr = endptr;
  nnz = strtol(ptr, &endptr, base);

  LOG(INFO) << "M_ = " << M_
            << " N_ = " << N_
            << " nnz = " << nnz;

  suc = getline(&line, &num_bytes, data_stream);

  int cnt = 0;

  while (suc != -1) {
    int row_id = strtol(line, &endptr, base);   // Read the row id.
    ptr = endptr;
    int col_id = strtol(ptr, &endptr, base);
    ptr = endptr;
    float val = strtof(++ptr, &endptr);

    X_row.push_back(row_id);
    X_col.push_back(col_id);
    X_val.push_back(val);

    suc = getline(&line, &num_bytes, data_stream);
    ++cnt;
  }
  fclose(data_stream);
  LOG(INFO) << "Read " << cnt << " lines";
}

void PartitionToBins(const std::string &filename, int32_t num_partitions) {
  size_t num_nnz_per_partition
      = X_val.size() / num_partitions;

  int64_t partition_start = 0;

  for (int i = 0; i < num_partitions; ++i) {
    CHECK_LT(partition_start, X_row.size() - 1);
    int64_t partition_end = (i == num_partitions - 1) ?
                            X_row.size() - 1 :
                            partition_start + num_nnz_per_partition;

    int end_row_id = X_row[partition_end];

    CHECK(partition_end < X_row.size()) << "i = " << i;

    if (i != num_partitions - 1) {
      while (partition_end < X_row.size()
             && end_row_id == X_row[partition_end]) {
        ++partition_end;
      }

      CHECK(partition_end < X_row.size()) << "There's empty bin " << i;
      partition_end--;
    }
    std::string bin_file = filename + ".bin" + "." + std::to_string(num_partitions)
                           + "." + std::to_string(i);
    FILE *output = fopen(bin_file.c_str(), "wb");
    if (output == 0) {
      LOG(FATAL) << "Failed to open " << bin_file;
    }

    size_t num_nnz_this_partition = partition_end - partition_start + 1;
    size_t num_rows_this_partition = row_counts[partition_end]
                                     - row_counts[partition_start] + 1;

    fwrite(&num_nnz_this_partition, sizeof(size_t), 1, output);
    fwrite(&num_rows_this_partition, sizeof(size_t), 1, output);
    fwrite(&M_, sizeof(size_t), 1, output);

    fwrite(&(X_row[partition_start]), sizeof(int),
             num_nnz_this_partition, output);
    fwrite(&(X_col[partition_start]), sizeof(int),
             num_nnz_this_partition, output);
    fwrite(&(X_val[partition_start]), sizeof(float),
             num_nnz_this_partition, output);
    fclose(output);

    LOG(INFO) << "partition = " << i
              << " num_rows = " << num_rows_this_partition
              << " num_nnz = " << num_nnz_this_partition;

    partition_start = partition_end + 1;
  }
}

// Main function
int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_data_format == "libsvm") {
    ReadLibsvmData(FLAGS_datafile);
  } else if (FLAGS_data_format == "list") {
    ReadSparseMatrix(FLAGS_datafile);
  } else if (FLAGS_data_format == "mmt") {
    ReadMmtData(FLAGS_datafile);
  } else {
    LOG(FATAL) << "Unknown data format " << FLAGS_data_format;
  }

  LOG(INFO) << "Read Data End";

  PartitionToBins(FLAGS_datafile, FLAGS_num_partitions);
  return 0;
}
