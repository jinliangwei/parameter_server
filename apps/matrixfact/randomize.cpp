#include <fstream>
#include <vector>
#include <map>
#include <random>
#include <algorithm>

#include <glog/logging.h>

int N_, M_; // Number of rows and cols. (L_table has N_ rows, R_table has M_ rows.)

std::map<int, std::map<int, float> > mat;

std::vector<int> row_id_map;
std::vector<int> col_id_map;

void ReadSparseMatrix(const std::string &inputfile) {
  N_ = 0;
  M_ = 0;
  std::ifstream inputstream(inputfile.c_str());
  CHECK(inputstream) << "Failed to read " << inputfile;
  while(true) {
    int row, col;
    float val;
    inputstream >> row >> col >> val;

    auto row_iter = mat.find(row);

    if (row_iter == mat.end()) {
      mat.insert(std::make_pair(row, std::map<int, float>()));
      row_iter = mat.find(row);
    }

    row_iter->second.insert(std::make_pair(col, val));

    N_ = row + 1 > N_ ? row + 1 : N_;
    M_ = col + 1 > M_ ? col + 1 : M_;

    if (!inputstream) {
      break;
    }
  }
  inputstream.close();

  LOG(INFO) << "N_ = " << N_ << " M_ = " << M_;
}


void WriteSparseMatrix(const std::string &outputfile) {
  std::ofstream outputstream(outputfile.c_str());
  for (int i = 0; i < row_id_map.size(); ++i) {
    int row = row_id_map[i];

    auto row_iter = mat.find(row);

    if (row_iter == mat.end()) continue;

    auto &row_entries = row_iter->second;
    for (int j = 0; j < col_id_map.size(); ++ j) {
      int col = col_id_map[j];
      auto col_iter = row_entries.find(col);

      if (col_iter == row_entries.end()) continue;
      float val = col_iter->second;

      outputstream << i << " "
                   << j << " "
                   << val << "\n";
    }

    if (i % 1000 == 0)
      LOG(INFO) << "Written " << i << " rows";
  }
  outputstream.close();
}

void ShuffleRowAndCol() {
  row_id_map.resize(N_);
  col_id_map.resize(M_);

  for (int i = 0; i < N_; ++i) {
    row_id_map[i] = i;
  }

  for (int i = 0; i < M_; ++i) {
    col_id_map[i] = i;
  }

  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(row_id_map.begin(), row_id_map.end(), g);
  std::shuffle(col_id_map.begin(), col_id_map.end(), g);
}

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  ReadSparseMatrix("/home/jinliang/data/matrixfact_data/netflix.dat.list.gl");

  LOG(INFO) << "Read data done!";

  ShuffleRowAndCol();

  LOG(INFO) << "Done shuffling";

  WriteSparseMatrix("/home/jinliang/data/matrixfact_data/netflix.dat.list.gl.perm");

  LOG(INFO) << "Done writing";

  return 0;
}
