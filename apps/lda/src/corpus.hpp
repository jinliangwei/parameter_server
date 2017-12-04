// author: jinliang

#pragma once

#include "document_word_topics.hpp"

#include <list>
#include <stdint.h>
#include <mutex>
#include <memory>

namespace lda {

  // Work unit is defined as multiple of iterations.

class Corpus {
public:

  Corpus();

  void AddDoc(const uint8_t *doc_data);
  DocumentWordTopics *CreateAndGet();
  void RandomInit(DocumentWordTopics *doc);

  void RestartWorkUnit(uint32_t iters_per_work_unit);

  // Return a document iterator. It should be checked with
  // EndOfWorkUnit(). The document iterator is valid if and only if
  // if it has not reached then end of a work unit.
  // When the document fetched happens to be the last one in an
  // iteration, set num_tokens_iter to the number of tokens processed
  // in this iteration; otherwise it is set to a negative number.
  // Num_docs_work_unit is set to the number of documents has been processed in
  // this work unit.
  std::list<DocumentWordTopics>::iterator
  GetOneDoc(int32_t *num_iters_this_work_unit);

  std::list<DocumentWordTopics>::iterator
  GetOneDoc();

  bool EndOfWorkUnit(const std::list<DocumentWordTopics>::iterator &iter);

  size_t GetNumDocs();

private:

  uint32_t iters_per_work_unit_;

  std::list<DocumentWordTopics> docs_;
  std::list<DocumentWordTopics>::iterator docs_iter_;

  int32_t num_iters_this_work_unit_;
  std::mutex docs_mtx_;

  int32_t K_;
  // Random number generator stuff.
  boost::mt19937 gen_;
  std::unique_ptr<boost::uniform_int<> > one_K_dist_;
  std::unique_ptr<rng_t> one_K_rng_;
};

}
