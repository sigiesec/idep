#ifndef IDEP_TOKEN_ITERATOR_H_
#define IDEP_TOKEN_ITERATOR_H_

#include <istream>

#include "basictypes.h"

namespace idep {

class TokenIteratorImpl;

// Iterate over the tokens in an input stream.
class TokenIterator {
 public:
  // Create a token iterator for the specified stream.  A "token" is
  // either a newline ('\n') or a "word" consisting of a contiguous
  // sequence of non-white-space characters.  The stream object must
  // continue to exist while the iterator is in use.
  TokenIterator(std::istream& in);
  ~TokenIterator();

  // Advance to next token (i.e., "word" or newline).  The behavior is
  // undefined if the iteration state is not valid.
  void operator++();

  // Return non-zero if current token is valid; else 0.
  operator const void *() const;

  // Return the current token (i.e., "word" or newline).  The behavior
  // is undefined if the iteration state is not valid.
  const char* operator()() const;

 private:
  TokenIteratorImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(TokenIterator);
};

}  // namespace idep

#endif  // IDEP_TOKEN_ITERATOR_H_
