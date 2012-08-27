#include "idep_name_index_map.h"

#include <assert.h>
#include <memory.h>
#include <string.h>

#include <iostream>

#include "idep_name_array.h"

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
  const char* name_;
  int index_;                  // index of name
  NameIndexMapLink* next_;  // pointer to next link

  NameIndexMapLink(const char* name,
                   int index,
                   NameIndexMapLink* next);
};

NameIndexMapLink::NameIndexMapLink(const char* name,
                                   int index,
                                   NameIndexMapLink* next)
    : name_(name),
      index_(index),
      next_(next) {
}

static const NameIndexMapLink* find(const NameIndexMapLink* p,
                                    const char* name) {
  while (p && 0 != strcmp(p->name_, name))
    p = p->next_;
  return p;
}

struct NameIndexMapImpl {
    NameArray array_;
    NameIndexMapLink** table_;  // hash table of names
    int table_size_;            // size of hash table

    // Create a map representation assuming the specified (max) size.
    NameIndexMapImpl(int size);

    ~NameIndexMapImpl();

    // Find the appropriate slot for this name.
    NameIndexMapLink *& findSlot(const char* name);

    // Insert name into specified slot.
    int insert(NameIndexMapLink *& slot, const char* name);
};

NameIndexMapImpl::NameIndexMapImpl(int size)
    : array_(size),
      table_size_(size > 0 ? size : DEFAULT_TABLE_SIZE) {
    table_ = new NameIndexMapLink *[table_size_];
    assert(table_);
    memset(table_, 0, table_size_ * sizeof *table_);
}

NameIndexMapImpl::~NameIndexMapImpl() {
  for (int i = 0; i < table_size_; ++i) {
    NameIndexMapLink* p = table_[i];
    while (p) {
      NameIndexMapLink* q = p;
      p = p->next_;
      delete q;
    }
  }
  delete[] table_;
}

NameIndexMapLink *& NameIndexMapImpl::findSlot(const char* name) {
  int index = hash(name) % table_size_;
  assert(index >= 0 && index < table_size_);
  return table_[index];
}

int NameIndexMapImpl::insert(NameIndexMapLink *& slot, const char* nm) {
  int index = array_.Append(nm); // index is into a managed string array
  slot = new NameIndexMapLink(array_[index], index, slot);
  return index;
}

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
  const NameIndexMapLink* link = find(slot, name);
  return link ? link->index_ : impl_->insert(slot, name);
}

const char* NameIndexMap::operator[](int index) const {
  return impl_->array_[index];
}

int NameIndexMap::Length() const {
  return impl_->array_.Length();
}

int NameIndexMap::GetIndexByName(const char* name) const {
  NameIndexMapLink*& slot = impl_->findSlot(name);
  const NameIndexMapLink* link = find(slot, name);
  return link ? link->index_ : BAD_INDEX;
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
