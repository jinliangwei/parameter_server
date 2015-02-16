// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.04.03

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>    // std::random_shuffle
#include <time.h>
#include <set>
#include <random>
#include <ctype.h>
#include <sstream>
#include <leveldb/db.h>
#include <boost/unordered_map.hpp>

#include "document_word_topics.hpp"

// All these are required command line inputs
DEFINE_string(data_file, " ", "path to doc file in libsvm format.");
DEFINE_string(output_file, " ",
    "Results are in output_file.0, output_file.1, etc.");
DEFINE_int32(num_partitions, 0, "number of chunks to spit out");

// random generator function:
int MyRandom (int i) {
  static std::default_random_engine e;
  return e() % i;
}

std::string MakePartitionDBPath(int32_t partition) {
  std::stringstream ss;
  ss << FLAGS_output_file;
  ss << "." << partition;
  return ss.str();
}

std::vector<lda::DocumentWordTopics> corpus;
boost::unordered_map<int32_t, bool> vocab_occur;

int max_word_id = 0;
const int base = 10;

int num_tokens = 0;

std::vector<leveldb::DB*> db_vector;

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  char *line = NULL, *ptr = NULL, *endptr = NULL;
  size_t num_bytes;

  FILE *data_stream = fopen(FLAGS_data_file.c_str(), "r");
  CHECK_NOTNULL(data_stream);
  LOG(INFO) << "Reading from data file " << FLAGS_data_file;
  int doc_id = 0;
  int32_t word_id, count;
  while (getline(&line, &num_bytes, data_stream) != -1) {
    lda::DocumentWordTopics new_doc;
    new_doc.InitAppendWord();

    strtol(line, &endptr, base); // ignore first field (category label)
    ptr = endptr;

    while (*ptr != '\n') {
      // read a word_id:count pair
      word_id = strtol(ptr, &endptr, base);
      ptr = endptr;

      CHECK_EQ(':', *ptr) << "doc_id = " << doc_id;
      ++ptr;

      count = strtol(ptr, &endptr, base);
      ptr = endptr;

      new_doc.AppendWord(word_id, count);

      max_word_id = std::max(word_id, max_word_id);
      vocab_occur[word_id] = true;

      num_tokens += count;

      while(isspace(*ptr) && (*ptr != '\n')) ++ptr;
    }

    corpus.push_back(new_doc);
    ++doc_id;
    LOG_EVERY_N(INFO, 100000) << "Reading doc " << doc_id;
  }
  free(line);
  CHECK_EQ(0, fclose(data_stream)) << "Failed to close file "
                                   << FLAGS_data_file;

  int num_tokens_per_partition = static_cast<int>(num_tokens
      / FLAGS_num_partitions);

  // Do a random shuffling so each partition with the same number of tokens
  // will have roughly the same number of vocabs.
  srand(time(NULL));
  std::random_shuffle(corpus.begin(), corpus.end(), MyRandom);

  size_t num_tokens_per_doc = num_tokens / corpus.size();

  LOG(INFO) << "number of docs = " << corpus.size();
  LOG(INFO) << "vocab size = " << vocab_occur.size();
  LOG(INFO) << "number of tockens = " << num_tokens;
  LOG(INFO) << "number of tokens per partition = " << num_tokens_per_partition;
  LOG(INFO) << "number of tokens per doc = " << num_tokens_per_doc;
  LOG(INFO) << "max_word_id = " << max_word_id;

  db_vector.resize(FLAGS_num_partitions);
  for (int i = 0; i < FLAGS_num_partitions; ++i) {
    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;
    options.block_size = 1024*1024;
    std::string db_path = MakePartitionDBPath(i);
    leveldb::Status db_status = leveldb::DB::Open(options, db_path,
                                                  &(db_vector[i]));
    CHECK(db_status.ok())
        << "open leveldb failed, partition = " << i;
  }

  size_t num_tokens_curr_partition = 0;
  int32_t curr_partition = 0;
  uint8_t *mem_buf = new uint8_t[1024];
  size_t buf_size = 1024;
  int32_t prev_par_doc_idx = 0;
  size_t partition_data_size = 0;
  for (int32_t doc_idx = 0; doc_idx < corpus.size(); ++doc_idx) {
    size_t serialized_size = corpus[doc_idx].SerializedDocSize();
    if (buf_size < serialized_size) {
      delete[] mem_buf;
      buf_size = serialized_size;
      mem_buf = new uint8_t[buf_size];
    }

    partition_data_size += serialized_size;

    corpus[doc_idx].SerializeDoc(mem_buf);
    leveldb::Slice key(reinterpret_cast<const char*>(&doc_idx),
                       sizeof(doc_idx));

    leveldb::Slice value(reinterpret_cast<const char*>(mem_buf),
                         serialized_size);

    db_vector[curr_partition]->Put(leveldb::WriteOptions(), key, value);
    num_tokens_curr_partition += corpus[doc_idx].NumTokens();

    if ((curr_partition < FLAGS_num_partitions - 1)
	&& (num_tokens_curr_partition
	    > (num_tokens_per_partition - num_tokens_per_doc))) {
      LOG(INFO) << "number tokens current partition = "
		<< num_tokens_curr_partition;
      LOG(INFO) << "number docs current partition = "
		<< doc_idx - prev_par_doc_idx + 1;
      LOG(INFO) << "partition data size = "
		<< partition_data_size;
      prev_par_doc_idx = doc_idx;
      partition_data_size = 0;
      num_tokens_curr_partition = 0;
      ++curr_partition;
    }
  }

  delete[] mem_buf;
  for (int i = 0; i < FLAGS_num_partitions; ++i) {
    delete db_vector[i];
  }

  return 0;
}
