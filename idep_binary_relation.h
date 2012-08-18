#ifndef IDEP_BINARY_RELATION_H_
#define IDEP_BINARY_RELATION_H_

#include <ostream>

namespace idep {

// This leaf component defines 1 class:
// Square matrix of bits with transitive closure capability.
class BinaryRelation {
  public:
    // CREATORS
    BinaryRelation(int initialEntries = 0, int max_entries_hint = 0);
        // Create a binary relation that can be extended as needed.
        // By default, the initial number of entires in the relation
        // is 0.  If the final number of entries is known and is not
        // the same as initialEntries, that value may optionally be
        // specified as the second argument (as a "hint").

    BinaryRelation(const BinaryRelation& rel);
    ~BinaryRelation();

    // MANIPULATORS
    BinaryRelation& operator=(const BinaryRelation& rel);

    void set(int row, int col, int bit);
        // Set the specified row/col of this relation to the specified 
        // binary value.

    void set(int row, int col);
        // Set the specified row/col of this relation to 1.

    void clr(int row, int col);
        // Set specified row/col of this relation to 0.

    void makeTransitive();
        // Apply Warshall's algorithm to this relation.  The result is the
        // reflexive transitive closure of the original relation.

    void makeNonTransitive();
        // Remove all redundant relationships such that the transitive
        // closure would be left unaffected.  In cases where there is
        // a cyclic dependency, the solution is not unique.  To work 
        // properly, the relation must already be fully transitive 
        // (see makeTransitive).

    int appendEntry();
        // Append an entry to this relation and return its integer index.
        // The logical size is increased by 1 with all new entries 0'ed.

    // ACCESSORS
    int get(int row, int col) const;
        // Get the boolean value at the specified row/col of this relation.

    int cmp(const BinaryRelation& rel) const;
        // Return 0 if and only if the specified relation has the same
        // length and logical values as this relation.

    int Length() const;
        // Return number of rows and columns in this relation.  The length
        // represents the cardinality of the set on which this relation is
        // defined.

  private:
    void grow();
      // Increase the physical size of this relation.

    void compress();
      // Make the physical size same as Logical size (unless that is 0).

    void warshall(int bit);
      // Perform Warshall's algorithm either forward or backward. 

    char **d_rel_p;     // array of pointers into a contiguous byte array
    int d_size;         // physical size of array
    int d_length;       // logical size of array
};

std::ostream& operator<<(std::ostream& out, const BinaryRelation& rel);
   // Output this binary relation in row/column format with the upper left 
   // corner as the origin to the specified output stream (out).

int operator==(const BinaryRelation& left, const BinaryRelation& right);
   // Return 1 if two relations are equal, and 0 otherwise (see cmp).

int operator!=(const BinaryRelation& left, const BinaryRelation& right);
   // Return 1 if two relations are not equal, and 0 otherwise (see cmp).


// #########################################################################
// The following consists of inline function definitions for this component. 
// #########################################################################

inline int BinaryRelation::appendEntry() 
{
    if (d_length >= d_size) {
        grow();
    }
    return d_length++;
}

inline void BinaryRelation::set(int row, int col, int bit) 
{
    d_rel_p[row][col] = !!bit;
}

inline void BinaryRelation::set(int row, int col) 
{
    d_rel_p[row][col] = 1;
}

inline void BinaryRelation::clr(int row, int col) 
{
    d_rel_p[row][col] = 0;
}

inline int BinaryRelation::get(int row, int col) const
{
    return d_rel_p[row][col];
}

inline int BinaryRelation::Length() const {
    return d_length;
}

inline int operator==(const BinaryRelation& left, const BinaryRelation& right) {
    return left.cmp(right) == 0;
}

inline int operator!=(const BinaryRelation& left, const BinaryRelation& right) {
    return left.cmp(right) != 0;
}

}  // namespace idep

#endif  // IDEP_BINARY_RELATION_H_
