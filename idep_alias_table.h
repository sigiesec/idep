#ifndef IDEP_ALIAS_TABLE_H_
#define IDEP_ALIAS_TABLE_H_

#include <ostream>

#include "basictypes.h"

namespace idep {

class AliasTableLink;

// This leaf component defines 2 classes:
// Supports efficient (hashed) name to name mapping.
class AliasTable {
 public:
  // Create a new table; optionally specify expected number of entries.
  explicit AliasTable(int size_hint = 0);
  ~AliasTable();

  // Add an alias to the table.  Returns 0 on success, 1 if the
  // identical alias/originalName was already present, -1 if an
  // alias with a different original name was present.  Under
  // no circumstances will an alias be overwritten with a new
  // original name.  (Neither alias nor originalName may be 0.)
  int add(const char* alias, const char* originalName);

  // Return the original name if the alias exists, else 0.
  const char* Lookup(const char* alias) const;

 private:
  friend class AliasTableIterator;

  // Hash Table.
  AliasTableLink** table_;

  // Size of hash table.
  int size_;

  DISALLOW_COPY_AND_ASSIGN(AliasTable);
};

// Write the entire logical contents of the specified alias table in some
// reasonable format to the specified output stream.
std::ostream& operator<<(std::ostream& output, const AliasTable& table);

// Iterate through the collection of name mappings.
class AliasTableIterator {
 public:
  // Create an iterator for the specified table.
  AliasTableIterator(const AliasTable& table);
  ~AliasTableIterator();

  // Reset this iterator to the start of the iteration.
  void reset();

  // Advance state of iteration to next alias/originalName pair.
  void operator++();

  // Return non-zero if current element is valid; else 0.
  operator const void *() const;

  // Return current alias name.
  const char* GetAlias() const;

  // Return the (original) name corresponding to current alias name.
  const char* GetOriginalName() const;

 private:
  // Reference to const alias table.
  const AliasTable& table_;

  // Pointer to current link in table.
  AliasTableLink* d_link_p;

  // Index of current slot.
  int d_index;

  DISALLOW_COPY_AND_ASSIGN(AliasTableIterator);
};

}  // namespace idep

#endif  // IDEP_ALIAS_TABLE_H_
