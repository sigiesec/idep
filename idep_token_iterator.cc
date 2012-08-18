#include "idep_token_iterator.h"

#include <assert.h>
#include <ctype.h>      // isspace()
#include <memory.h>     // memcpy()

#include <iostream>

enum { START_SIZE = 1, GROW_FACTOR = 2 };

const char kNewLineChar = '\n';
const char kNullChar= '\0';

namespace idep {

struct TokenIteratorImpl {
  TokenIteratorImpl(std::istream& in);
  ~TokenIteratorImpl();

  void Grow();
  void AddChar(char ch);
  void Advance();

  std::istream& in_;
  char *buf_;
  int size_;
  int length_;
  int newline_flag_;

};

TokenIteratorImpl::TokenIteratorImpl(std::istream& in)
    : in_(in),
      buf_(new char[START_SIZE]),
      size_(START_SIZE),
      length_(0),
      newline_flag_(0) {
    assert(buf_);
}

TokenIteratorImpl::~TokenIteratorImpl() {
  delete buf_;
}

void TokenIteratorImpl::Grow() {
  int new_size = size_ * GROW_FACTOR;
  char* tmp = buf_;
  buf_ = new char[new_size];
  assert(buf_);
  memcpy(buf_, tmp, size_);
  size_ = new_size;
  delete[] tmp;
}

void TokenIteratorImpl::AddChar(char ch) {
  if (length_ >= size_)
    Grow();

  assert(length_ < size_);
  buf_[length_++] = ch;
}

TokenIterator::TokenIterator(std::istream& in)
    : impl_(new TokenIteratorImpl(in)) {
  ++*this; // load first occurrence.
}

TokenIterator::~TokenIterator() {
  delete impl_;
}

void TokenIterator::operator++() {
    assert(*this);

    impl_->length_ = 0;

    if (impl_->newline_flag_) {                   // left over newline
        impl_->newline_flag_ = 0;
        impl_->AddChar(kNewLineChar);
    }
    else {
        char c;
        while (impl_->in_ && !impl_->in_.get(c).eof()) {
            if (impl_->length_ > 0) {            // "word" in progress
                if (isspace(c)) {
                    if (kNewLineChar == c) {
                        impl_->newline_flag_ = 1; // note newline for later
                    }
                    break;                         // end of "word" in any case
                }
                impl_->AddChar(c);                // start of "word"
            }
            else {                                 // nothing found yet
                if (isspace(c)) {
                    if (kNewLineChar== c) {
                        impl_->AddChar(kNewLineChar);
                        break;                     // found a newline
                    }
                    continue;                      // found an ordinary space
                }
                impl_->AddChar(c);                // add character to "word"
            }
        }
    }

    if (impl_->length_ > 0)
        impl_->AddChar(kNullChar);               // always append a null char
    else
        impl_->length_ = -1;                     // or make iterator invalid
}

TokenIterator::operator const void *() const {
  return impl_->length_ >= 0 ? this : 0;
}

const char* TokenIterator::operator()() const {
  return impl_->buf_;
}

}  // namespace idep
