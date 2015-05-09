#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <limits>
#include <random>
#include <set>

size_t X_num_rows, X_num_cols; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)
std::vector<int32_t> X_row; // Row index of each nonzero entry in the data matrix
std::vector<int32_t> X_col; // Column index of each nonzero entry in the data matrix
std::vector<float> X_val; // Value of each nonzero entry in the data matrix
std::vector<int32_t> X_partition_starts;

void ReadSparseMatrix(std::string inputfile) {
  X_row.clear();
  X_col.clear();
  X_val.clear();
  X_num_rows = 0;
  X_num_cols = 0;
  int row_count = 0;
  int curr_row = -1;
  std::ifstream inputstream(inputfile.c_str());
  if (!inputstream) {
    std::cout << "failed reading " << inputfile << std::endl;
    abort();
  }

  while (true) {
    int32_t row, col;
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

    X_num_rows = row+1 > X_num_rows ? row+1 : X_num_rows;
    X_num_cols = col+1 > X_num_cols ? col+1 : X_num_cols;
  }
  inputstream.close();
}

void ReadBinaryMatrix(const std::string &filename, int32_t partition_id) {
  std::string bin_file = filename + "." + std::to_string(partition_id);

  FILE *bin_input = fopen(bin_file.c_str(), "rb");
  if (bin_input  == 0) {
    std::cout << "failed to read " << bin_file << std::endl;
  }
  uint64_t num_nnz_this_partition = 0,
        num_rows_this_partition = 0,
        num_cols_this_partition = 0;
  size_t read_size = fread(&num_nnz_this_partition, sizeof(uint64_t), 1, bin_input);
  read_size = fread(&num_rows_this_partition, sizeof(uint64_t), 1, bin_input);
  read_size = fread(&num_cols_this_partition, sizeof(uint64_t), 1, bin_input);
  std::cout << "num_nnz_this_partition: " << num_nnz_this_partition
	    << " num_rows_this_partition: " << num_rows_this_partition
	    << " num_cols_this_partition: " << num_cols_this_partition << std::endl;

  X_row.resize(num_nnz_this_partition);
  X_col.resize(num_nnz_this_partition);
  X_val.resize(num_nnz_this_partition);

  read_size = fread(X_row.data(), sizeof(int32_t), num_nnz_this_partition, bin_input);
  read_size = fread(X_col.data(), sizeof(int32_t), num_nnz_this_partition, bin_input);
  read_size = fread(X_val.data(), sizeof(float), num_nnz_this_partition, bin_input);

  X_num_rows = num_rows_this_partition;
  X_num_cols = num_cols_this_partition;
  std::cout << "partition = " << partition_id
            << " #row = " << X_num_rows
            << " #cols = " << X_num_cols << std::endl;
}

int main (int argc, char *argv[]) {
  //ReadBinaryMatrix("/l0/netflix.64.bin", 33);
  //ReadSparseMatrix("/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.x256/netflix.dat.list.gl.perm.duplicate.Rx16.Cx16.33");
  ReadBinaryMatrix("/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.x256.bin/netflix.dat.list.gl.perm.duplicate.Rx16.Cx16.64.bin", 33);
  int32_t max_col_id = 0;

  std::cout << "read done" << std::endl;

  for (auto & col_id : X_col) {
    if (col_id > max_col_id)
      max_col_id = col_id;
  } 
  std::cout << "max col id = " << max_col_id << std::endl;
}
