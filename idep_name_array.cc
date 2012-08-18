#include "idep_name_array.h"

#include <assert.h>
#include <memory.h>     // memcpy()
#include <string.h>     // strlen()

#include <iostream>

enum { START_SIZE = 1, GROW_FACTOR = 2 };

static char* newStrCpy(const char* oldStr) {
  int size = strlen(oldStr) + 1;
  char *newStr = new char[size];
  assert(newStr);
  memcpy(newStr, oldStr, size);
  return newStr;
}

namespace idep {

NameArray::NameArray(int max_entries_hint)
    : size_(max_entries_hint> 0 ? max_entries_hint: START_SIZE),
      length_(0) {
  array_ = new char *[size_];
}

NameArray::~NameArray() {
  for (int i = 0; i < length_; ++i)
    delete [] array_[i];

  delete [] array_;
}

int NameArray::Append(const char* name) {
  if (length_ >= size_) {
    int oldSize = size_;
    size_ *= GROW_FACTOR;
    char **tmp = array_;
    array_ = new char *[size_];
    assert (array_);
    memcpy (array_, tmp, oldSize * sizeof *array_);
    delete [] tmp;
  }
  assert(length_ < size_);
  array_[length_++] = newStrCpy(name);
  return length_ - 1;
}

const char* NameArray::operator[](int i) const {
  return i < length_ && i >= 0 ? array_[i] : 0;
}

int NameArray::Length() const {
  return length_;
}

std::ostream& operator<<(std::ostream& out, const NameArray& array) {
  int fieldWidth = 10;
  int maxIndex = array.Length() - 1;
  assert (sizeof (long int) >= 4);
  long int x = 1000 * 1000 * 1000;    // requires 4-byte integer.
  while (fieldWidth > 1 && 0 == maxIndex / x) {
    --fieldWidth;
    x /= 10;
  }

  for (int i = 0; i < array.Length(); ++i) {
    out.width(fieldWidth);
    out << i << ". " << array[i] << std::endl;
  }

  return out;
}

}  // namespace idep
