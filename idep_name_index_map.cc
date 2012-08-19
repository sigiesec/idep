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

namespace idep {

struct NameIndexMapLink {
 const char *d_name_p;
 int d_index;                 // index of name
 NameIndexMapLink *d_next_p;  // pointer to next link

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

static const NameIndexMapLink* find(const NameIndexMapLink* p,
                                    const char* name) {
    while (p && 0 != strcmp(p->d_name_p, name))
        p = p->d_next_p;
    return p;
}

struct NameIndexMapImpl {
    idep::NameArray d_array;       // array of names
    NameIndexMapLink **d_table_p;  // hash table of names
    int d_tableSize;               // size of hash table

    // Create a map representation assuming the specified (max) size.
    NameIndexMapImpl(int size);

    ~NameIndexMapImpl();

    // Find the appropriate slot for this name.
    NameIndexMapLink *& findSlot(const char* name);

    // Insert name into specified slot.
    int insert(NameIndexMapLink *& slot, const char* name);
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

                // -*-*-*- NameIndexMap -*-*-*-

NameIndexMap::NameIndexMap(int max_entries_hint)
    : impl_(new NameIndexMapImpl(max_entries_hint)) {
}

NameIndexMap::~NameIndexMap() {
    delete impl_;
}

int NameIndexMap::Add(const char* name) {
    NameIndexMapLink *& slot = impl_->findSlot(name);
    return find(slot, name) ? BAD_INDEX : impl_->insert(slot, name);
}

int NameIndexMap::Entry(const char* name) {
    NameIndexMapLink *& slot = impl_->findSlot(name);
    const NameIndexMapLink *link = find(slot, name);
    return link ? link->d_index : impl_->insert(slot, name);
}

const char* NameIndexMap::operator[](int index) const {
    return impl_->d_array[index];
}

int NameIndexMap::Length() const {
    return impl_->d_array.Length();
}

int NameIndexMap::Lookup(const char* name) const {
    NameIndexMapLink *& slot = impl_->findSlot(name);
    const NameIndexMapLink* link = find(slot, name);
    return link ? link->d_index : BAD_INDEX;
}

std::ostream& operator<<(std::ostream& out, const NameIndexMap& map) {
    int field_width = 10;
    int max_index = map.Length() - 1;
    assert (sizeof (long int) >= 4);
    long int x = 1000 * 1000 * 1000;    // requires 4-byte integer.
    while (field_width > 1 && 0 == max_index / x) {
        --field_width;
        x /= 10;
    }

    for (int i = 0; i < map.Length(); ++i) {
        out.width(field_width);
        out << i << ". " << map[i] << std::endl;
    }

    return out;
}

}  // namespace idep
