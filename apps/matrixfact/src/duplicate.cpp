#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_string(datafile, "", "Input sparse matrix");
DEFINE_int32(row_duplicate_factor, 1, "duplicate factor");
DEFINE_int32(col_duplicate_factor, 1, "duplicate factor");
DEFINE_int32(num_partitions, 1, "num partitions");

size_t N_, M_; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)
std::vector<int32_t> row_starts;
std::vector<int32_t> X_row; // Row index of each nonzero entry in the data matrix
std::vector<int32_t> X_col; // Column index of each nonzero entry in the data matrix
std::vector<float> X_val; // Value of each nonzero entry in the data matrix

void ReadSparseMatrix(std::string inputfile) {
  X_row.clear();
  X_col.clear();
  X_val.clear();
  N_ = 0;
  M_ = 0;
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
      row_starts.push_back(X_row.size());
      curr_row = row;
    }
    X_row.push_back(row);
    X_col.push_back(col);
    X_val.push_back(val);
    N_ = row+1 > N_ ? row+1 : N_;
    M_ = col+1 > M_ ? col+1 : M_;
  }
  inputstream.close();
  LOG(INFO) << "N_ = " << N_
            << " M_ = " << M_;
}

void WriteDuplicates(int32_t row_duplicate_factor,
		     int32_t col_duplicate_factor,
		     size_t nnz_per_partition) {
  int32_t partition_idx = 0;
  std::stringstream ss;
  ss << FLAGS_datafile << ".duplicate" 
     << ".Rx" << row_duplicate_factor
     << ".Cx" << col_duplicate_factor
     << "." << partition_idx;

  partition_idx++;

  LOG(INFO) << "output to " << ss.str().c_str();
  std::ofstream outputstream(ss.str().c_str());

  size_t nnz_curr_partition = 0;

  for (int j = 0; j < row_duplicate_factor; ++j) {
    for (const auto &start_index : row_starts) {
      for (int i = 0; i < col_duplicate_factor; ++i) {
	int32_t index = start_index;
	int32_t row = X_row[index];
	while (X_row[index] == row) {
	  outputstream << row + N_ * j << " " << X_col[index] + M_*i
		       << " " << X_val[index] << "\n";
	  index++;
	  nnz_curr_partition++;
	}
      }
      LOG_EVERY_N(INFO, 10000) << "Written " << google::COUNTER
			       << " rows";
      if (nnz_curr_partition >= nnz_per_partition
	  && partition_idx < FLAGS_num_partitions) {
	outputstream.close();
	std::stringstream ss;
	ss << FLAGS_datafile << ".duplicate" 
	   << ".Rx" << row_duplicate_factor
	   << ".Cx" << col_duplicate_factor
	   << "." << partition_idx;
	
	partition_idx++;
       
	LOG(INFO) << "output to " << ss.str().c_str();
	outputstream.open(ss.str().c_str());
	CHECK(outputstream.good());
	nnz_curr_partition = 0;
      }
    }
  }
  outputstream.close();
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "Starting here";
  ReadSparseMatrix(FLAGS_datafile);
  LOG(INFO) << "Read done";

  size_t nnz_per_partition
    = std::ceil(float(X_val.size() 
		      * FLAGS_row_duplicate_factor * FLAGS_col_duplicate_factor) 
		/ FLAGS_num_partitions);

  WriteDuplicates(FLAGS_row_duplicate_factor,
		  FLAGS_col_duplicate_factor,
		  nnz_per_partition);

  return 0;
}
