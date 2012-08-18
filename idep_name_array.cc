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
    : d_size(max_entries_hint> 0 ? max_entries_hint: START_SIZE),
      d_length(0) {
  d_array_p = new char *[d_size];
}

NameArray::~NameArray() {
  for (int i = 0; i < d_length; ++i)
    delete [] d_array_p[i];

  delete [] d_array_p;
}

int NameArray::Append(const char* name) {
  if (d_length >= d_size) {
    int oldSize = d_size;
    d_size *= GROW_FACTOR;
    char **tmp = d_array_p;
    d_array_p = new char *[d_size];
    assert (d_array_p);
    memcpy (d_array_p, tmp, oldSize * sizeof *d_array_p);
    delete [] tmp;
  }
  assert(d_length < d_size);
  d_array_p[d_length++] = newStrCpy(name);
  return d_length - 1;
}

const char* NameArray::operator[](int i) const {
  return i < d_length && i >= 0 ? d_array_p[i] : 0;
}

int NameArray::Length() const {
  return d_length;
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
