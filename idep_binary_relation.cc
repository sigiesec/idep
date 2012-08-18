#include "idep_binary_relation.h"

#include <assert.h>
#include <memory.h>     // memcpy() memset() memcmp()

#include <iostream>

// IMPLEMENTATION NOTE: MEMORY LAYOUT
// +---------+          +---------+             +---+---+---+---+
// |         |--------->|         |------------>|   |   |   |   |
// +---------+          +---------+             +---+---+---+---+
//    char**            |         |------------>|   |   |   |   |
//                      +---------+             +---+---+---+---+
//                      |         |------------>|   |   |   |   |
//                      +---------+             +---+---+---+---+
//                      |         |------------>|   |   |   |   |
//                      +---------+             +---+---+---+---+
//                      char*[size]             contiguous memory
//                                              char[size * size]

enum { START_SIZE = 1, GROW_FACTOR = 2 };

static void clean(char **p)  {
  /* crash, bt:
#0  0x00002aaaab08011d in raise () from /lib/libc.so.6
#1  0x00002aaaab08184e in abort () from /lib/libc.so.6
#2  0x00002aaaab0b4e41 in __fsetlocking () from /lib/libc.so.6
#3  0x00002aaaab0ba90e in malloc_usable_size () from /lib/libc.so.6
#4  0x00002aaaab0bac56 in free () from /lib/libc.so.6
#5  0x000000000040a0ab in clean (p=0x526a50) at idep_binary_relation.cc:28
#6  0x000000000040a46f in ~BinaryRelation (this=0x527b60) at idep_binary_relation.cc:124
#7  0x000000000040254b in ~idep_LinkDep_i (this=0x523460) at idep_ldep.cxx:150
#8  0x00000000004042dc in ~idep_LinkDep (this=0x7fffffb944a0) at idep_ldep.cxx:700
#9  0x0000000000401dbf in main (argc=5, argv=0x7fffffb94608) at ldep.cxx:199
*/
    delete [] *p;               // only one 2-d block is allocated
    delete [] p;                // delete single block
}

static char **alloc(int size) {
    register int s = size;
    char **rel = new char *[s];
    register char *p = new char[s * s];
    for (register int i = 0; i < s; ++i, p += s) {
        rel[i] = p;
    }
    return rel;
}

static void clear(char *const *rel, int size) 
{
    memset(*rel, 0, size * size);
}

static void copy(char **left, const char *const *right, int size)
{
    memcpy(*left, *right, size * size);
}

namespace idep {

void BinaryRelation::grow()
{
    int newSize = d_size * GROW_FACTOR;
    char **tmp = d_rel_p;
    d_rel_p = alloc(newSize);
    clear(d_rel_p, newSize);

    for (int i = 0; i < d_size; ++i) {
        memcpy(d_rel_p[i], tmp[i], d_size);
    }

    d_size = newSize;
    clean(tmp);
}

void BinaryRelation::compress()
{
    if (d_size > d_length && d_length > 0) {
        d_size = d_length;
        char **tmp = d_rel_p;
        d_rel_p = alloc(d_size);
        clear(d_rel_p, d_size);
        for (int i = 0; i < d_size; ++i) {
            memcpy(d_rel_p[i], tmp[i], d_size);
        }
        clean(tmp);
    }
}

                // -*-*-*- public functions -*-*-*-

BinaryRelation::BinaryRelation(int initialEntries, int max_entries_hint) 
: d_size(max_entries_hint > 0 ? max_entries_hint : START_SIZE) 
, d_length(initialEntries > 0 ? initialEntries : 0)
{
    if (d_size < d_length) {
        d_size = d_length;
    }
    d_rel_p = alloc(d_size);
    clear(d_rel_p, d_size);
}

BinaryRelation::BinaryRelation(const BinaryRelation& rel) 
: d_size(rel.d_size)
, d_length(rel.d_length)
, d_rel_p(alloc(rel.d_size))
{ 
  // TODO 
  copy(d_rel_p, d_rel_p + d_size, rel.d_size);
}

BinaryRelation& BinaryRelation::operator=(const BinaryRelation& rel) 
{  
    if (&rel != this) {
        if (d_size != rel.d_size) { 
            clean(d_rel_p);
            d_size = rel.d_size;
            d_rel_p = alloc(d_size);
        }
        //TODO
	copy(d_rel_p, d_rel_p + d_size, rel.d_size);
	//	copy(d_rel_p, rel.d_rel_p, d_size);
        d_length = rel.d_length;
    }
    return *this;
}

BinaryRelation::~BinaryRelation() 
{
  clean(d_rel_p); // part of crash
}

int BinaryRelation::cmp(const BinaryRelation& rel) const
{
    enum { SAME = 0, DIFFERENT = 1 };

    if (d_length != rel.d_length) {
        return DIFFERENT;
    }

    if (d_size == rel.d_size && 
                            memcmp(*d_rel_p, *rel.d_rel_p, d_size * d_size)) {
        return DIFFERENT;
    }

    for (int i = 0; i < d_length; ++i) {
        if (memcmp(d_rel_p[i], rel.d_rel_p[i], d_length)) {
            return DIFFERENT;
        }
    }

    return SAME;
}

void BinaryRelation::warshall(int bit) 
{
    // DEFINITION: Transitive Closure
    //
    // Given: R is a relation (matrix) indicating transitions in 1 step.
    // TransitiveClosure(R) = R + R^2 + R^3 + ... + R^N 
    //      where N is the number of rows and columns in the square matrix R.
    //
    // Warshall's algorithm to construct the transitive closure:
    // foreach index k
    //     foreach row i
    //         foreach column j
    //             if (A[i][k] && A[k][j]) 
    //                 A[i][j] = 1;      
    // 
    // See Aho, Hopcroft, & Ullman, "Data Structures And Algorithms,"
    // Addison-Wesley, Reading MA, pp. 212-213.  Also see, Warshall, S. [1962].
    // "A theorem on Boolean matrices," Journal of the ACM, 9:1, pp. 11-12.

    compress();
    assert(d_size == d_length || 0 == d_length);

    register const int VALUE = !!bit;
    register int s = d_length;  // size and length are the same or length is 0
    register char *top_row = d_rel_p[0] + s * s;

    for (int k = 0; k < s; ++k) {
        register char *row_k = d_rel_p[k];
        for (register char *row_r = d_rel_p[0]; row_r < top_row; row_r += s) {
            if (!row_r[k]) { 
                continue;                   // huge optimization
            }
            if (d_rel_p[k] == row_r) {  
                continue;                   // note: ignore self dependency
            }
            for (register int c = 0; c < s; ++c) {
                if (row_k[c] && row_r[k]) { // note: both conditions are needed
                                            // in reverse (consider k == c).
                    if (c != k) {           // note: ignore self dependency
                        row_r[c] = VALUE;   
                    }
                }
            }
        }
    }
}

void BinaryRelation::makeTransitive() 
{
    warshall(1);
}

void BinaryRelation::makeNonTransitive() 
{
    warshall(0);
    // make non-reflexive too -- i.e., subtract the identity matrix.
    for (int i = 0; i < Length(); ++i) {
        d_rel_p[i][i] = 0;
    }
}

std::ostream& operator<<(std::ostream& o, const BinaryRelation& rel) 
{
    int r, c;
    const int GAP_GRID = 10;
    const char *SP = rel.Length() < 30 ? " " : "";
    for (r = 0; r < rel.Length(); ++r) {
        if (r && 0 == r % GAP_GRID) {
            o << std::endl;
        }

        for (c = 0; c < rel.Length(); ++c) {
            if (c && 0 == c % GAP_GRID) { 
                o << ' ' << SP;
            }
            o << (rel.get(r,c) ? '1' : '_') << SP;
        }
        o << std::endl;
    }
    return o;
}

}  // namespace idep
