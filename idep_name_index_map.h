#ifndef IDEP_NAME_INDEX_MAP_H_
#define IDEP_NAME_INDEX_MAP_H_

#include <ostream>

#include "basictypes.h"

class NameIndexMapImpl;

// This component defines 1 fully insulated class:
// Efficient two-way mapping between strings and indices.
class idep_NameIndexMap {
 public:
  // Create a new mapping; optionally specify the expected number of
  // entires.  By default, a moderately large hash table will be created.
  explicit idep_NameIndexMap(int max_entries_hint = 0);
  ~idep_NameIndexMap();

  // Add a name to the mapping and return its index only if the name is
  // not already present; otherwise return -1.
  int add(const char* name);

  // Add a name to the table if necessary; always return a valid index.
  // Note: entry() is usually more efficient than Lookup() followed by
  // an occasional add().
  int entry(const char* name);

  // Return the name associated with the specified index or 0 if the
  // specified index is out of the range [0 .. N], where N = length - 1.
  const char* operator[](int index) const;

  // Return the number of unique names in this mapping.
  int Length() const;

  // Return the index of the specified name, or -1 if not found.
  int Lookup(const char* name) const;

 private:
  NameIndexMapImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(idep_NameIndexMap);
};

// Print the logical contents of this mapping to the specified output stream
// (out) in some reasonable format.
std::ostream& operator<<(std::ostream& out, const idep_NameIndexMap& map);

#endif  // IDEP_NAME_INDEX_MAP_H_
