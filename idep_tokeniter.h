#ifndef IDEP_TOKENITER_H_
#define IDEP_TOKENITER_H_

#include <istream>

// Iterate over the tokens in an input stream.
class idep_TokenIter {
 public:
  // Create a token iterator for the specified stream.  A "token" is
  // either a newline ('\n') or a "word" consisting of a contiguous
  // sequence of non-white-space characters.  The stream object must
  // continue to exist while the iterator is in use.
  idep_TokenIter(std::istream& in);
  ~idep_TokenIter();

  // Advance to next token (i.e., "word" or newline).  The behavior is
  // undefined if the iteration state is not valid.
  void operator++();

  // Return non-zero if current token is valid; else 0.
  operator const void *() const;

  // Return the current token (i.e., "word" or newline).  The behavior
  // is undefined if the iteration state is not valid.
  const char *operator()() const;

 private:
  class idep_TokenIter_i;
  idep_TokenIter_i *d_this;

  // Disallow copy and assign.
  idep_TokenIter(const idep_TokenIter&);
  idep_TokenIter& operator=(const idep_TokenIter&);
};

#endif  // IDEP_TOKENITER_H_
