// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.03.29

#pragma once

#include <petuum_ps_common/include/petuum_ps.hpp>

#include "corpus.hpp"
#include "fast_doc_sampler.hpp"
#include "lda_stats.hpp"
#include "context.hpp"
#include "document_word_topics.hpp"
#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <set>

namespace lda {

// Engine takes care of the entire pipeline of LDA, from reading data to
// spawning threads, to recording execution time and loglikelihood.
class LDAEngine {
public:
  LDAEngine();

  void ReadData(const std::string& doc_file);

  // The main thread (in main()) should spin threads, each running Start()
  // concurrently.
  void Start();

  void ResetDocIteratorBarrier(int32_t thread_id);

private:  // private data
  int32_t K_;   // number of topics

  // Number of application threads.
  int32_t num_threads_;

  // vocabs in this data partition.
  std::set<int32_t> local_vocabs_;
  std::mutex local_vocabs_mtx_;

  // Compute complete log-likelihood (ll) every compute_ll_interval_
  // iterations.
  int32_t compute_ll_interval_;

  // LDAStats is thread-safe, but has to be initialized after PS tables are
  // created (thus can't be initialized in constructor).
  std::unique_ptr<LDAStats> lda_stats_;

  // Local barrier across threads.
  std::unique_ptr<boost::barrier> process_barrier_;

  Corpus corpus_;

  std::atomic<int32_t> thread_counter_;

  // # of tokens processed in a work_unit
  std::atomic<int32_t> num_tokens_this_work_unit_;
  std::atomic<int32_t> num_docs_this_work_unit_;
};

}   // namespace lda
