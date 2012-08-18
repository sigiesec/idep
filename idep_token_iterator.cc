#include "idep_token_iterator.h"

#include <ctype.h>      // isspace()
#include <memory.h>     // memcpy()
#include <iostream>
#include <assert.h>

using namespace std;

enum { START_SIZE = 1, GROW_FACTOR = 2 };

const char NEWLINE_CHAR = '\n';
const char NULL_CHAR = '\0';

namespace idep {

struct TokenIteratorImpl {
    istream& d_in;
    char *d_buf_p;
    int d_size;
    int d_length;
    int d_newlineFlag;

    TokenIteratorImpl(std::istream& in);
    ~TokenIteratorImpl();
    void grow();
    void addChar(char ch);
    void advance();
};

TokenIteratorImpl::TokenIteratorImpl(std::istream& in)
: d_in(in)
, d_buf_p(new char[START_SIZE])
, d_size(START_SIZE)
, d_length(0)
, d_newlineFlag(0) {
    assert(d_buf_p);
}

TokenIteratorImpl::~TokenIteratorImpl() {
    delete d_buf_p;
}

void TokenIteratorImpl::grow() {
    int newSize = d_size * GROW_FACTOR;
    char *tmp = d_buf_p;
    d_buf_p = new char[newSize];
    assert(d_buf_p);
    memcpy(d_buf_p, tmp, d_size);
    d_size = newSize;
    delete [] tmp;
}

void TokenIteratorImpl::addChar(char ch) {
    if (d_length >= d_size)
        grow();

    assert(d_length < d_size);
    d_buf_p[d_length++] = ch;
}

TokenIterator::TokenIterator(std::istream& in)
    : impl_(new TokenIteratorImpl(in)) {
    ++*this; // load first occurrence.
}

TokenIterator::~TokenIterator() {
    delete impl_;
}

void TokenIterator::operator++()
{
    assert(*this);

    impl_->d_length = 0;

    if (impl_->d_newlineFlag) {                   // left over newline
        impl_->d_newlineFlag = 0;
        impl_->addChar(NEWLINE_CHAR);
    }
    else {
        char c;
        while (impl_->d_in && !impl_->d_in.get(c).eof()) {
            if (impl_->d_length > 0) {            // "word" in progress
                if (isspace(c)) {
                    if (NEWLINE_CHAR == c) {
                        impl_->d_newlineFlag = 1; // note newline for later
                    }
                    break;                         // end of "word" in any case
                }
                impl_->addChar(c);                // start of "word"
            }
            else {                                 // nothing found yet
                if (isspace(c)) {
                    if (NEWLINE_CHAR == c) {
                        impl_->addChar(NEWLINE_CHAR);
                        break;                     // found a newline
                    }
                    continue;                      // found an ordinary space
                }
                impl_->addChar(c);                // add character to "word"
            }
        }
    }

    if (impl_->d_length > 0)
        impl_->addChar(NULL_CHAR);                // always append a null char
    else
        impl_->d_length = -1;                     // or make iterator invalid
}

TokenIterator::operator const void *() const {
    return impl_->d_length >= 0 ? this : 0;
}

const char* TokenIterator::operator()() const {
    return impl_->d_buf_p;
}

}  // namespace idep
