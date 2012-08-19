#include "idep_name_array.h"

#include <assert.h>
#include <memory.h>
#include <string.h>

#include <iostream>

enum { START_SIZE = 1, GROW_FACTOR = 2 };

static char* NewStrCpy(const char* old_str) {
  int size = strlen(old_str) + 1;
  char* new_str = new char[size];
  assert(new_str);
  memcpy(new_str, old_str, size);
  return new_str;
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
    char** tmp = array_;
    array_ = new char *[size_];
    assert (array_);
    memcpy (array_, tmp, oldSize * sizeof *array_);
    delete[] tmp;
  }
  assert(length_ < size_);
  array_[length_++] = NewStrCpy(name);
  return length_ - 1;
}

const char* NameArray::operator[](int index) const {
  return (index < length_ && index >= 0) ? array_[index] : 0;
}

int NameArray::Length() const {
  return length_;
}

std::ostream& operator<<(std::ostream& out, const NameArray& array) {
  int field_width = 10;
  int max_index = array.Length() - 1;
  assert(sizeof (long int) >= 4);
  long int x = 1000 * 1000 * 1000;    // requires 4-byte integer.
  while (field_width > 1 && 0 == max_index / x) {
    --field_width;
    x /= 10;
  }

  for (int i = 0; i < array.Length(); ++i) {
    out.width(field_width);
    out << i << ". " << array[i] << std::endl;
  }

  return out;
}

}  // namespace idep
