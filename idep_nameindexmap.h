#ifndef IDEP_NAMEINDEXMAP_H_
#define IDEP_NAMEINDEXMAP_H_

// This component defines 1 fully insulated class:
//   idep_NameIndexMap: efficient two-way mapping between strings and indices

#include <ostream>

using namespace std;


class idep_NameIndexMap_i;
class idep_NameIndexMap {
    idep_NameIndexMap_i *d_this;

  private:
    idep_NameIndexMap(const idep_NameIndexMap&);            // not implemented
    idep_NameIndexMap& operator=(const idep_NameIndexMap&); // not implemented

  public:
    // CREATORS
    idep_NameIndexMap(int maxEntriesHint = 0);
        // Create a new mapping; optionally specify the expected number of
        // entires.  By default, a moderately large hash table will be created.
    ~idep_NameIndexMap();

    // MANIPULATORS
    int add(const char* name);
        // Add a name to the mapping and return its index only if the name is
        // not already present; otherwise return -1.

    int entry(const char* name);
        // Add a name to the table if necessary; always return a valid index.
        // Note: entry() is usually more efficient than lookup() followed by
        // an occasional add().

    // ACCESSORS
    const char *operator[](int) const;
        // Return the name associated with the specified index or 0 if the
        // specified index is out of the range [0 .. N], where N = length - 1.

    int length() const;
        // Return the number of unique names in this mapping.

    int lookup(const char *name) const;
        // Return the index of the specified name, or -1 if not found.
};

ostream& operator<<(ostream& out, const idep_NameIndexMap& map);
        // Print the logical contents of this mapping to the specified
        // output stream (out) in some reasonable format.

#endif  // IDEP_NAMEINDEXMAP_H_
