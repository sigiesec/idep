#include "idep_name_index_map.h"

#include "idep_name_array.h"

#include <assert.h>
#include <memory.h>     // memcpy() memset()
#include <string.h>     // strcmp()

#include <iostream>

enum { DEFAULT_TABLE_SIZE = 521 };
enum { BAD_INDEX = -1 };

static unsigned hash(register const char* name) {
    register unsigned sum = 1000003; // 1,000,003 is the 78,498th prime number
    while (*name) {
        sum *= *name++; // integer multiplication is a subroutine on a SPARC
    }
    return sum; // unsigned ensures positive value for use with (%) operator.
}

struct NameIndexMapLink {
 const char *d_name_p;                       // name
 int d_index;                                // index of name
 NameIndexMapLink *d_next_p;            // pointer to next link

 NameIndexMapLink(const char* name,
                       int index,
                       NameIndexMapLink* d_next_p);
};

NameIndexMapLink::NameIndexMapLink(const char* name,
                                             int index,
                                             NameIndexMapLink *next)
    : d_name_p(name),
      d_index(index),
      d_next_p(next) {
}

static const NameIndexMapLink *find(const NameIndexMapLink *p,
                                         const char *name)
{
    while (p && 0 != strcmp(p->d_name_p, name)) {
        p = p->d_next_p;
    }
    return p;
}

struct NameIndexMapImpl {
    idep::NameArray d_array;                     // array of names
    NameIndexMapLink **d_table_p;          // hash table of names
    int d_tableSize;                            // size of hash table

    NameIndexMapImpl(int size);
        // create a map representation assuming the specified (max) size

    ~NameIndexMapImpl();

    NameIndexMapLink *& findSlot(const char *name);
        // find the appropriate slot for this name

    int insert(NameIndexMapLink *& slot, const char *name);
        // insert name into specified slot
};

NameIndexMapImpl::NameIndexMapImpl(int size)
: d_array(size)
, d_tableSize(size > 0 ? size : DEFAULT_TABLE_SIZE)
{
    d_table_p = new NameIndexMapLink *[d_tableSize];
    assert(d_table_p);
    memset(d_table_p, 0, d_tableSize * sizeof *d_table_p);
}

NameIndexMapImpl::~NameIndexMapImpl()
{
    for (int i = 0; i < d_tableSize; ++i) {
        NameIndexMapLink *p = d_table_p[i];
        while (p) {
        NameIndexMapLink *q = p;
            p = p->d_next_p;
            delete q;
        }
    }
    delete [] d_table_p;
}

NameIndexMapLink *& NameIndexMapImpl::findSlot(const char *name)
{
    int index = hash(name) % d_tableSize;
    assert(index >= 0 && index < d_tableSize);
    return d_table_p[index];
}

int NameIndexMapImpl::insert(NameIndexMapLink *& slot, const char *nm)
{
    int index = d_array.Append(nm); // index is into a managed string array
    slot = new NameIndexMapLink(d_array[index], index, slot);
    return index;
}

                // -*-*-*- idep_NameIndexMap -*-*-*-

idep_NameIndexMap::idep_NameIndexMap(int max_entries_hint)
: impl_(new NameIndexMapImpl(max_entries_hint))
{
}

idep_NameIndexMap::~idep_NameIndexMap()
{
    delete impl_;
}

int idep_NameIndexMap::add(const char* name)
{
    NameIndexMapLink *& slot = impl_->findSlot(name);
    return find(slot, name) ? BAD_INDEX : impl_->insert(slot, name);
}

int idep_NameIndexMap::entry(const char* name)
{
    NameIndexMapLink *& slot = impl_->findSlot(name);
    const NameIndexMapLink *link = find(slot, name);
    return link ? link->d_index : impl_->insert(slot, name);
}

const char *idep_NameIndexMap::operator[](int i) const
{
    return impl_->d_array[i];
}

int idep_NameIndexMap::length() const
{
    return impl_->d_array.Length();
}

int idep_NameIndexMap::Lookup(const char* name) const
{
    NameIndexMapLink *& slot = impl_->findSlot(name);
    const NameIndexMapLink* link = find(slot, name);
    return link ? link->d_index : BAD_INDEX;
}

std::ostream& operator<<(std::ostream& out, const idep_NameIndexMap& map)
{
    int fieldWidth = 10;
    int maxIndex = map.length() - 1;
    assert (sizeof (long int) >= 4);
    long int x = 1000 * 1000 * 1000;    // requires 4-byte integer.
    while (fieldWidth > 1 && 0 == maxIndex / x) {
        --fieldWidth;
        x /= 10;
    }

    for (int i = 0; i < map.length(); ++i) {
        out.width(fieldWidth);
        out << i << ". " << map[i] << std::endl;
    }

    return out;
}
