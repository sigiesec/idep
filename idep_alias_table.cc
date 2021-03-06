#include "idep_alias_table.h"

#include <assert.h>
#include <memory.h>     // memcpy()
#include <string.h>     // strcmp() strlen()

#include <iostream>

namespace {

enum { DEFAULT_TABLE_SIZE = 521 };

unsigned hash(register const char* name) {
    register unsigned sum = 1000003; // 1,000,003 is the 78,498th prime number
    while (*name) {
        sum *= *name++; // integer multiplication is a subroutine on a SPARC
    }
    return sum; // unsigned ensures positive value for use with (%) operator.
}

char* NewStrCpy(const char* old_str) {
  int size = strlen(old_str) + 1;
  char* new_str = new char[size];
  assert(new_str);
  memcpy(new_str, old_str, size);
  return new_str;
}

}  // namespace

namespace idep {

struct AliasTableLink {
  char* d_alias_p;                       // "from" (alias) name
  char* d_originalName_p;                // "to" (original) name
  AliasTableLink* d_next_p;              // pointer to next link

  AliasTableLink(const char* alias,
                 const char* original_name,
                 AliasTableLink* next);
  ~AliasTableLink();
};

AliasTableLink::AliasTableLink(const char* alias,
                               const char* original_name,
                               AliasTableLink* next)
    : d_alias_p(NewStrCpy(alias)),
      d_originalName_p(NewStrCpy(original_name)),
      d_next_p(next) {
}

AliasTableLink::~AliasTableLink()  {
  delete[] d_alias_p;
  delete[] d_originalName_p;
}

AliasTable::AliasTable(int size)
    : size_(size > 0 ? size : DEFAULT_TABLE_SIZE) {
  table_ = new AliasTableLink *[size_];
  assert(table_);
  memset(table_, 0, size_ * sizeof *table_);
};

AliasTable::~AliasTable() {
    for (int i = 0; i < size_; ++i) {
        AliasTableLink* p = table_[i];
        while (p) {
            AliasTableLink *q = p;
            p = p->d_next_p;
            delete q;
        }
    }
    delete[] table_;
}

int AliasTable::Add(const char* alias, const char* original_name)  {
    enum { FOUND_DIFFERENT = -1, NOT_FOUND = 0, FOUND_IDENTICAL = 1 };

    AliasTableLink *&slot = table_[hash(alias) % size_];
    AliasTableLink *p = slot;

    while (p && 0 != strcmp(p->d_alias_p, alias)) {
        p = p->d_next_p;
    }
    if (!p) {
        slot = new AliasTableLink(alias, original_name, slot);
        return NOT_FOUND;
    } else if (0 == strcmp(p->d_originalName_p, original_name)) {
        return FOUND_IDENTICAL;
    } else {
        return FOUND_DIFFERENT;
    }
}

const char* AliasTable::Lookup(const char* alias) const {
    AliasTableLink* p = table_[hash(alias) % size_];
    while (p && 0 != strcmp(p->d_alias_p, alias)) {
        p = p->d_next_p;
    }
    return p ? p->d_originalName_p : 0;
}

std::ostream& operator<<(std::ostream &o, const AliasTable& table) {
    int fieldWidth = 0;
    AliasTableIterator it(table);
    for (it.Reset(); it; ++it) {
        int len = strlen(it.GetAlias());
        if (fieldWidth < len) {
            fieldWidth = len;
        }
    }
    for (it.Reset(); it; ++it) {
        o.width(fieldWidth);
        o << it.GetAlias() << " -> " << it.GetOriginalName() << std::endl;
    }
    return o;
}

AliasTableIterator::AliasTableIterator(const AliasTable& t)
    : table_(t) {
  Reset();
}

AliasTableIterator::~AliasTableIterator() {
}

void AliasTableIterator::Reset() {
  d_link_p = 0;
  d_index = -1;
  ++*this;
}

void AliasTableIterator::operator++() {
    if (d_link_p) {
        d_link_p = d_link_p->d_next_p;
    }
    while (!d_link_p && *this) {
        ++d_index;
        if (*this) {
            d_link_p = table_.table_[d_index];
        }
    }
}

AliasTableIterator::operator const void *() const {
    return d_index < table_.size_ ? this : 0;
}

const char* AliasTableIterator::GetAlias() const {
    return d_link_p->d_alias_p;
}

const char* AliasTableIterator::GetOriginalName() const {
    return d_link_p->d_originalName_p;
}

}  // namespace idep
