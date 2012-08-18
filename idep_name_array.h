#ifndef IDEP_NAME_ARRAY_H_
#define IDEP_NAME_ARRAY_H_

#include <ostream>

#include "basictypes.h"

namespace idep {

// This leaf component defines 1 class:
// Extensible array of managed character string names.
class NameArray {
 public:
  // Create a variable length array of const character strings.
  // The array will be allocated assuming a maximum expected number
  // of entries specified by |max_entries_hint|.
  // By default a fairly small array will be allocated.
  explicit NameArray(int max_entries_hint = 0);

  // Destroy this array including its contained copies of string names.
  ~NameArray();

  // Append a copy of the specified string to the end of the array.
  // The value of the new index is returned.  No attempt is made to
  // check for repeated string values.
  int Append(const char* new_name);

  // Return a pointer to the specified string.  Strings are stored at
  // at consecutive non-negative index locations beginning with 0 up
  // to one less than the current length.  If the index is out of range,
  // a null pointer will be returned.
  const char* operator[](int index) const;

  // Return the number of names currently stored in this array.
  int Length() const;

 private:
  // Array of dynamically allocated character strings.
  char** array_;

  // Physical size of array.
  int size_;

  // Logical size of array.
  int length_;

  DISALLOW_COPY_AND_ASSIGN(NameArray);
};

// Print the logical contents of this name array to the specified
// output stream (out) in some suitable format.
std::ostream& operator<<(std::ostream& out, const NameArray& array);

}  // namespace idep

#endif  // IDEP_NAME_ARRAY_H_
