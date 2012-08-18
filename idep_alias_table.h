#ifndef IDEP_ALIAS_TABLE_H_
#define IDEP_ALIAS_TABLE_H_

#include <ostream>

class idep_AliasTableLink;

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
  const char* lookup(const char* alias) const;

 private:
  friend class AliasTableIterator;

  // Hash Table.
  idep_AliasTableLink **d_table_p;

  // Size of hash table.
  int d_size;

  // Disallow copy and assign.
  AliasTable(const AliasTable&);
  AliasTable& operator=(const AliasTable&);
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
  const char* alias() const;

  // Return the (original) name corresponding to current alias name.
  const char* originalName() const;

 private:
  // Reference to const alias table.
  const AliasTable& d_table;

  // Pointer to current link in table.
  idep_AliasTableLink *d_link_p;

  // Index of current slot.
  int d_index;

  // Disallow copy and assign.
  AliasTableIterator(const AliasTableIterator&);
  AliasTableIterator& operator=(const AliasTableIterator&);
};

#endif  // IDEP_ALIAS_TABLE_H_
