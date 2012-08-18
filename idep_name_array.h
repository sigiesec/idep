#ifndef IDEP_NAME_ARRAY_H_
#define IDEP_NAME_ARRAY_H_

#include <ostream>
#include <istream>

// This leaf component defines 1 class:
//   idep_NameDep: extensible array of managed character string names.
class idep_NameArray {
 public:
  // Create a variable length array of const character strings.
  // The array will be allocated assuming a maximum expected number
  // of entries specified by the optional maxEntriesHint argument.
  // By default a fairly small array will be allocated.
  idep_NameArray(int maxEntriesHint = 0);

  // Destroy this array including its contained copies of string names.
  ~idep_NameArray();

  // Append a copy of the specified string to the end of the array.
  // The value of the new index is returned.  No attempt is made to
  // check for repeated string values.
  int append(const char *newName);

  // Return a pointer to the specified string.  Strings are stored at
  // at consecutive non-negative index locations beginning with 0 up
  // to one less than the current length.  If the index is out of range,
  // a null pointer will be returned.
  const char* operator[](int index) const;

  // Return the number of names currently stored in this array.
  int length() const;

 private:
  char** d_array_p;   // Array of dynamically allocated character strings.
  int d_size;         // Physical size of array.
  int d_length;       // Logical size of array.

  // Disallow copy and assign.
  idep_NameArray(const idep_NameArray&);
  idep_NameArray& operator=(const idep_NameArray&);
};

// Print the logical contents of this name array to the specified
// output stream (out) in some suitable format.
std::ostream& operator<<(std::ostream& out, const idep_NameArray& array);

#endif  // IDEP_NAME_ARRAY_H_
