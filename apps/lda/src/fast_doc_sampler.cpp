// Author: Dai Wei (wdai@cs.cmu.edu)
// Date: 2014.03.28

#include "fast_doc_sampler.hpp"
#include "context.hpp"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <glog/logging.h>
#include <algorithm>
#include <time.h>
#include <sstream>
#include <string>
#include <algorithm>

namespace lda {

FastDocSampler::FastDocSampler() :
    rng_engine_(time(NULL)),
    uniform_zero_one_dist_(0, 1),
    zero_one_rng_(new rng_t(rng_engine_, uniform_zero_one_dist_)) {
  Context& context = Context::get_instance();
  word_topic_row_buff_.resize(context.get_int32("num_topics"));

  // Topic model parameters.
  K_ = context.get_int32("num_topics");
  V_ = context.get_int32("num_vocabs");
  CHECK_NE(-1, V_);
  beta_ = context.get_double("beta");
  beta_sum_ = beta_ * V_;
  alpha_ = context.get_double("alpha");

  // PS tables.
  int32_t summary_table_id = context.get_int32("summary_table_id");
  int32_t word_topic_table_id = context.get_int32("word_topic_table_id");
  summary_table_ = petuum::PSTableGroup::GetTableOrDie<int>(summary_table_id);
  word_topic_table_ = petuum::PSTableGroup::GetTableOrDie<int>(
      word_topic_table_id);

  // Fast sampler variables
  q_coeff_.resize(K_);
  nonzero_q_terms_.resize(K_);
  nonzero_q_terms_topic_.resize(K_);
  doc_topic_vec_.resize(K_);

  nonzero_doc_topic_idx_ = new int32_t[K_];

  summary_table_.ThreadGet(0, &summary_row_accessor_);
}

FastDocSampler::~FastDocSampler() {
  delete[] nonzero_doc_topic_idx_;
}

void FastDocSampler::RefreshCachedSummaryRow(bool timer) {
  petuum::HighResolutionTimer flush_timer;
  summary_table_.FlushThreadCache();
  double flush_sec = flush_timer.elapsed();

  petuum::HighResolutionTimer thread_get_timer;
  summary_table_.ThreadGet(0, &summary_row_accessor_);
  double thread_get_sec = thread_get_timer.elapsed();
  if (timer)
    LOG(INFO) << "flush_sec = " << flush_sec
              << " thread_get_sec = " << thread_get_sec;
}

void FastDocSampler::SampleOneDoc(DocumentWordTopics* doc) {
  // Preparations before sampling a document.
  ComputeDocTopicVector(doc);
  ComputeAuxVariables();

  const petuum::DenseRow<int32_t> &summary_row
      = summary_row_accessor_.Get<petuum::DenseRow<int32_t> >();

  // Start sampling.
  for (WordOccurrenceIterator it(doc); !it.End(); it.Next()) {
    int32_t old_topic = it.Topic();
    CHECK_LT(old_topic, K_);

    // Remove this token from corpus. It requires following updates:
    //
    // 1. Update terms in s, r and q bucket.
    real_t denom = summary_row[old_topic] + beta_sum_;
    s_sum_ -= (alpha_ * beta_) / denom;
    s_sum_ += (alpha_ * beta_) / (denom - 1);
    r_sum_ -= (doc_topic_vec_[old_topic] * beta_) / denom;
    r_sum_ += ((doc_topic_vec_[old_topic] - 1) * beta_) / (denom - 1);
    q_coeff_[old_topic] =
      (alpha_ + doc_topic_vec_[old_topic] - 1) / (denom - 1);

    // 2. doc_topic_vec_. If old_topic token count goes to 0, update the
    // nonzero_doc_topic_idx_, etc.
    --doc_topic_vec_[old_topic];
    if (doc_topic_vec_[old_topic] == 0) {
      int32_t* zero_idx =
        std::lower_bound(nonzero_doc_topic_idx_,
            nonzero_doc_topic_idx_ + num_nonzero_doc_topic_idx_, old_topic);
      memmove(zero_idx, zero_idx + 1,
          (nonzero_doc_topic_idx_ + num_nonzero_doc_topic_idx_ - zero_idx - 1)
          * sizeof(int32_t));
      --num_nonzero_doc_topic_idx_;
    }

    summary_table_.ThreadInc(0, old_topic, -1);

    {
      // Compute the mass in q bucket by iterating through the sparse word-topic
      // vector.
      q_sum_ = 0.;
      petuum::RowAccessor word_topic_row_acc;
      const auto & word_topic_row
          = word_topic_table_.Get<petuum::SortedVectorMapRow<int32_t> >(
              it.Word(), &word_topic_row_acc);

      num_nonzero_q_terms_ = 0;
      word_topic_row.CopyToVector(&word_topic_row_buff_);

      STATS_APP_DEFINED_ACCUM_VAL_INC(word_topic_row_buff_.size());

      for (auto & wt_it : word_topic_row_buff_) {
        int32_t topic = wt_it.first;
        int32_t count = std::max(wt_it.second, 0);

        nonzero_q_terms_[num_nonzero_q_terms_] = (topic == old_topic) ?
          (q_coeff_[topic] * (count - 1)) : (q_coeff_[topic] * count);
        nonzero_q_terms_topic_[num_nonzero_q_terms_] = topic;
        q_sum_ += nonzero_q_terms_[num_nonzero_q_terms_];
        ++num_nonzero_q_terms_;
      }
      // Release word_topic_row_acc.
    }

    // Sample the topic for token 'it' using the aux variables.
    int32_t new_topic = Sample();
    CHECK_LT(new_topic, K_) << "word = " << it.Word();

    // Add this token with new topic back to corpus using following steps:
    //
    // 1. Update s, r and q bucket.
    denom = summary_row[new_topic] + beta_sum_;
    s_sum_ -= (alpha_ * beta_) / denom;
    s_sum_ += (alpha_ * beta_) / (denom + 1);
    r_sum_ -= (doc_topic_vec_[new_topic] * beta_) / denom;
    r_sum_ += ((doc_topic_vec_[new_topic] + 1) * beta_) / (denom + 1);
    q_coeff_[new_topic] =
      (alpha_ + doc_topic_vec_[new_topic] + 1) / (denom + 1);

    // 2. doc_topic_vec_. If new_topic token count goes to 1, we need to add it
    // to the nonzero_doc_topic_idx_, etc.
    ++doc_topic_vec_[new_topic];
    if (doc_topic_vec_[new_topic] == 1) {
      int32_t* insert_idx =
        std::lower_bound(nonzero_doc_topic_idx_,
            nonzero_doc_topic_idx_ + num_nonzero_doc_topic_idx_, new_topic);
      memmove(insert_idx + 1, insert_idx, (nonzero_doc_topic_idx_ +
            num_nonzero_doc_topic_idx_ - insert_idx) * sizeof(int32_t));
      *insert_idx = new_topic;
      ++num_nonzero_doc_topic_idx_;
    }

    summary_table_.ThreadInc(0, new_topic, 1);

    // Finally, update the topic assignment z in doc, and update word-topic
    // table and summary row.
    if (old_topic != new_topic) {
      it.SetTopic(new_topic);

      petuum::UpdateBatch<int32_t> word_topic_updates(2);
      word_topic_updates.UpdateSet(0, old_topic, -1);
      word_topic_updates.UpdateSet(1, new_topic, 1);
      word_topic_table_.BatchInc(it.Word(), word_topic_updates);
    }
  }
}

// ====================== Private Functions ===================

void FastDocSampler::ComputeDocTopicVector(DocumentWordTopics* doc) {
  // Zero out doc_topic_vec_
  std::fill(doc_topic_vec_.begin(), doc_topic_vec_.end(), 0);

  for (WordOccurrenceIterator it(doc); !it.End(); it.Next()) {
    ++doc_topic_vec_[it.Topic()];
  }
}

void FastDocSampler::ComputeAuxVariables() {
  // zero out.
  s_sum_ = 0.;
  r_sum_ = 0.;
  num_nonzero_doc_topic_idx_ = 0;
  real_t alpha_beta = alpha_ * beta_;

  const petuum::DenseRow<int32_t> &summary_row
    = summary_row_accessor_.Get<petuum::DenseRow<int32_t> >();

  for (int k = 0; k < K_; ++k) {
    real_t denom = summary_row[k] + beta_sum_;
    q_coeff_[k] = (alpha_ + doc_topic_vec_[k]) / denom;
    s_sum_ += alpha_beta / denom;

    if (doc_topic_vec_[k] != 0) {
      r_sum_ += (doc_topic_vec_[k] * beta_) / denom;

      // Populate nonzero_doc_topic_idx_.
      nonzero_doc_topic_idx_[num_nonzero_doc_topic_idx_++] = k;
    }
  }
}

int32_t FastDocSampler::Sample() {
  // Shooting a dart on [q_sum_ | r_sum_ | s_sum_] interval.
  real_t total_mass = q_sum_ + r_sum_ + s_sum_;
  real_t sample = (*zero_one_rng_)() * total_mass;

  const petuum::DenseRow<int32_t> &summary_row
    = summary_row_accessor_.Get<petuum::DenseRow<int32_t> >();

  if (sample < q_sum_) {
    // The dart falls in [q_sum_ interval], which consists of [large_q_term |
    // ... | small_q_term]. ~90% should fall in the q bucket.
    for (int i = 0; i < num_nonzero_q_terms_; ++i) {
      sample -= nonzero_q_terms_[i];
      if (sample <= 0.) {
        return nonzero_q_terms_topic_[i];
      }
    }
    // Overflow.
    LOG(INFO) << "sample = " << sample << " has overflowed in q bucket.";
    return nonzero_q_terms_topic_[num_nonzero_q_terms_ - 1];
  } else {
    sample -= q_sum_;
    if (sample < r_sum_) {
      // r bucket.
      sample /= beta_;  // save multiplying beta_ later on.
      for (int i = 0; i < num_nonzero_doc_topic_idx_; ++i) {
        int32_t nonzero_topic = nonzero_doc_topic_idx_[i];
        sample -= doc_topic_vec_[nonzero_topic]
          / (summary_row[nonzero_topic] + beta_sum_);
        if (sample <= 0.) {
          return nonzero_topic;
        }
      }
      //LOG(INFO) << "sample = " << sample << " has overflowed in r bucket.";
      return nonzero_doc_topic_idx_[num_nonzero_doc_topic_idx_ - 1];
    } else {
      // s bucket.
      sample -= r_sum_;
      sample /= alpha_ * beta_;

      // There's no sparsity here to exploit. Just go through each term.
      for (int k = 0; k < K_; ++k) {
        sample -= 1. / (summary_row[k] + beta_sum_);
        if (sample <= 0.) {
          return k;
        }
      }
      //LOG(INFO) << "sample = " << sample << " has overflowed in s bucket.";
      return (K_ - 1);
    }
  }
  LOG(FATAL) << "Sampler overflow.";
  return -1;
}

}  // namespace lda
