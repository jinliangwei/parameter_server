// author: jinliang

#pragma once

#include <glog/logging.h>
#include <cstdlib>
#include <vector>
#include <cstdint>
#include <list>
#include <string>
#include <string.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

namespace lda {
typedef boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
  rng_t;
using WordId = int64_t;
// The structure representing a document as a bag of words and the
// topic assigned to each occurrence of a word.  In term of Bayesian
// learning and LDA, the bag of words are ``observable'' data; the
// topic assignments are ``hidden'' data.

// A DocumentWordTopics object created via default constructor can be
// initialized in 2 ways:
// 1) Append a series of word-count pairs and call RandomInitWordTopics() if
// topics are needed.
// 2) Deserialize and call RandomInitWordTopics().
class DocumentWordTopics {
public:
  DocumentWordTopics() { }

  DocumentWordTopics(const DocumentWordTopics &other):
      words_(other.words_),
      accum_token_count_(other.accum_token_count_),
      word_topics_(other.word_topics_),
      num_tokens_(other.num_tokens_) { }

  DocumentWordTopics & operator = (const DocumentWordTopics &other) {
    words_ = other.words_;
    accum_token_count_ = other.accum_token_count_;
    word_topics_ = other.word_topics_;
    num_tokens_ = other.num_tokens_;
    return *this;
  }

  size_t NumTokens() const {
    return num_tokens_;
  }

  size_t NumUniqueWords() const {
    return words_.size();
  }

  // Number of occurrence of word word_index.
  size_t NumOfOccurences(int32_t word_index) const {
    return accum_token_count_[word_index + 1] -
      accum_token_count_[word_index];
  }

  int32_t WordLastTopicIndex(int32_t word_index) const {
    return accum_token_count_[word_index + 1] - 1;
  }

  int32_t WordFirstTopicIndex(int32_t word_index) const {
    return accum_token_count_[word_index];
  }

  WordId Word(int32_t word_index) const { return words_[word_index]; }
  int32_t WordTopics(int32_t index) const { return word_topics_[index]; }
  int32_t &WordTopics(int32_t index) { return word_topics_[index]; }

  void RandomInitWordTopics(rng_t *one_K_rng) {
    word_topics_.clear();
    for (size_t i = 0; i < words_.size(); ++i) {
      for (size_t j = NumOfOccurences(i); j > 0; --j) {
        word_topics_.push_back((*one_K_rng)() - 1);
      }
    }
  }

  // Serialized Doc format
  size_t SerializedDocSize() const {
    return sizeof(size_t) + words_.size()*sizeof(int32_t)
        + accum_token_count_.size()*sizeof(size_t);
  }

  void SerializeDoc(void *buf) const {
    uint8_t *buf_ptr = reinterpret_cast<uint8_t*>(buf);

    *(reinterpret_cast<size_t*>(buf_ptr)) = words_.size();
    buf_ptr += sizeof(size_t);

    memcpy(buf_ptr, words_.data(),
           words_.size() * sizeof(WordId));
    buf_ptr += words_.size()*sizeof(WordId);

    memcpy(buf_ptr, accum_token_count_.data(),
           accum_token_count_.size() * sizeof(size_t));
  }

  // #1 way of creating a document.
  void DeserializeDoc(const void *buf) {
    const uint8_t *buf_ptr = reinterpret_cast<const uint8_t*>(buf);
    size_t num_unique_words = *(reinterpret_cast<const size_t*>(buf_ptr));
    buf_ptr += sizeof(size_t);

    words_.resize(num_unique_words);

    memcpy(words_.data(), buf_ptr, num_unique_words*sizeof(WordId));
    buf_ptr += num_unique_words*sizeof(WordId);

    accum_token_count_.resize(num_unique_words + 1);
    memcpy(accum_token_count_.data(), buf_ptr,
           (num_unique_words + 1)*sizeof(size_t));

    int32_t last_count_index = words_.size();
    num_tokens_ = accum_token_count_[last_count_index];
  }

  // #2 way of creating a document.
  void InitAppendWord() {
    words_.clear();
    accum_token_count_.clear();
    accum_token_count_.push_back(0);
    num_tokens_ = 0;
  }

  void AppendWord(WordId word_id, size_t count) {
    int32_t word_idx = words_.size();
    words_.push_back(word_id);

    size_t accum_count = accum_token_count_[word_idx] + count;
    accum_token_count_.push_back(accum_count);

    num_tokens_ += count;
  }

private:
    // The document unique words list.
  std::vector<WordId> words_;

  // Accumulative count of the tokens. Length is words_.size() + 1.
  // All unique words are ordered as in the above words_ list.
  // I-th entry represents the number of occurrences of words [0, i-1]
  // in words_ (or number of occurrences of words before word i).
  // Two usage:
  // 1. number of occurences belonging to word i
  //    = accum_token_count_[i + 1] - accum_token_count_[i];
  // 2. topics for word i start with index accum_token_count_[i] in word_topics_
  std::vector<size_t> accum_token_count_;

  // Each word occurrance's topic. Words are ordered as in words_.
  std::vector<int32_t> word_topics_;

  size_t num_tokens_;
};

class WordOccurrenceIterator {
 public:
  // Intialize the WordOccurrenceIterator for a document.
  explicit WordOccurrenceIterator(DocumentWordTopics* doc):
      doc_(doc),
      word_index_(0),
      word_occur_index_(0){
    SkipWordsWithZeroOccurrences();
  }

  ~WordOccurrenceIterator() { }

  // Returns true if we are done iterating.
  bool End() {
    //CHECK_GT(doc_->NumUniqueWords(), 0);
    //LOG(INFO) << "num unique words = " << doc_->NumUniqueWords();
    return word_index_ == doc_->NumUniqueWords();
  }

  // Advances to the next word occurrence.
  void Next() {
    ++word_occur_index_;
    if (word_occur_index_ > doc_->WordLastTopicIndex(word_index_)) {
      ++word_index_;
      SkipWordsWithZeroOccurrences();
    }
  }

  // Returns the topic of the current occurrence.
  int Topic() {
    return doc_->WordTopics(word_occur_index_);
  }

  // Changes the topic of the current occurrence.
  void SetTopic(int new_topic) {
    doc_->WordTopics(word_occur_index_) = new_topic;
  }

  // Returns the word of the current occurrence.
  WordId Word() {
    return doc_->Word(word_index_);
  }

 private:
  // If the current word has no occurrences, advance until reaching a word
  // that does have occurrences or the end of the document.
  void SkipWordsWithZeroOccurrences() {
    // The second part of the condition means "while the current word has no
    // occurrences" (and thus no topic assignments).
    while (!End() && doc_->NumOfOccurences(word_index_) == 0) {
      ++word_index_;
    }
  }

  DocumentWordTopics* doc_;
  int word_index_;
  int word_occur_index_;
};
}
