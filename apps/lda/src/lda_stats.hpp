// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.04.03

#pragma once

#include <petuum_ps_common/include/petuum_ps.hpp>

#include "document_word_topics.hpp"
#include <string>

namespace lda {


// We reference Griffiths and Steyvers (PNAS, 2003).
class LDAStats {
public:
  LDAStats();

  // This is part of log P(z) in eq.[3].
  double ComputeOneDocLLH(DocumentWordTopics* doc);

  // This computes the terms in numerator part of log P(w|z) in eq.[2].
  // Covers words within [word_idx_start, word_idx_end).
  void ComputeWordLLH(int32_t ith_llh, int word_idx_start, int word_idx_end);

  // Only 1 client should call this in a iteration.
  void ComputeWordLLHSummary(int32_t ith_llh, int iter);

  // Record time (counted from the start of sampling).
  void SetTime(int32_t ith_llh, float time_in_sec);

  // Return a string of two columns: "iter-# llh."
  std::string PrintLLH(int32_t num_llh);

  // Print just the ith_llh llh in "iter-# llh" format.
  std::string PrintOneLLH(int32_t ith_llh);

private:  // private functions
  // Get (from cache or computed afresh) loggamma(val + alpha_).
  double GetLogGammaAlphaOffset(int val);

  // Get (from cache or computed afresh) loggamma(val + alpha_sum_).
  double GetLogGammaAlphaSumOffset(int val);

  // Get (from cache or computed afresh) loggamma(val + beta_).
  double GetLogGammaBetaOffset(int val);

private:
  // ================ Topic Model Parameters =================
  // Number of topics.
  int32_t K_;

  // Number of vocabs.
  int32_t V_;

  // Dirichlet prior for word-topic vectors.
  float beta_;

  // V_*beta_
  float beta_sum_;

  // Dirichlet prior for doc-topic vectors.
  float alpha_;

  // K_*alpha__
  float alpha_sum_;

  // ================ Precompute/Cached LLH Parameters =================
  // Log of normalization constant (per docoument) from eq.[3].
  double log_doc_normalizer_;

  // Log of normalization constant (per topic) from eq.[2].
  double log_topic_normalizer_;

  // Pre-compute loggamma.
  std::vector<double> loggamma_alpha_offset_;
  std::vector<double> loggamma_alpha_sum_offset_;
  std::vector<double> loggamma_beta_offset_;

  // Number of caches.
  // About # of tokens in a topic in a doc.
  static const int kNumLogGammaAlpha_;
  // About # of words in a doc.
  static const int kNumLogGammaAlphaSum_;
  // About # of tokens in a topic.
  static const int kNumLogGammaBeta_;

  // ============== Global Parameters from Petuum Server =================
  // A table containing just one summary row of [K x 1] dimension. The k-th
  // entry in the table is the # of tokens assigned to topic k.
  petuum::Table<int32_t> summary_table_;

  // Word topic table of V_ rows, each of which is a [K x 1] dim sparse sorted
  // row.
  petuum::Table<int32_t> word_topic_table_;

  // Log-likelihood table. Each row (say row i) has only one column storing
  // the complete log-likelihood of the i-th likelihood computation.
  petuum::Table<double> llh_table_;

  std::vector<petuum::Entry<int32_t> > word_topic_row_buff_;

};

}  // namespace lda
