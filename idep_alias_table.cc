#include "idep_alias_table.h"

#include <assert.h>
#include <memory.h>     // memcpy()
#include <string.h>     // strcmp() strlen()

#include <iostream>

enum { DEFAULT_TABLE_SIZE = 521 };

static unsigned hash(register const char* name) // Note: returns unsigned!
{
    register unsigned sum = 1000003; // 1,000,003 is the 78,498th prime number
    while (*name) {
        sum *= *name++; // integer multiplication is a subroutine on a SPARC
    }
    return sum; // unsigned ensures positive value for use with (%) operator.
}
 
static char *newStrCpy(const char *oldStr)
{
    int size = strlen(oldStr) + 1;
    char *newStr = new char[size];
    assert(newStr);
    memcpy(newStr, oldStr, size);
    return newStr;
}

namespace idep {

struct AliasTableLink {
    char *d_alias_p;                            // "from" (alias) name
    char *d_originalName_p;                     // "to" (original) name
    AliasTableLink *d_next_p;              // pointer to next link

    AliasTableLink(const char *alias, const char *orignalName, 
                                                AliasTableLink *next);
    ~AliasTableLink();
};

AliasTableLink::AliasTableLink(const char *alias, 
                         const char *originalName, AliasTableLink* next) 
: d_alias_p(newStrCpy(alias)) 
, d_originalName_p(newStrCpy(originalName)) 
, d_next_p(next) 
{ 
}

AliasTableLink::~AliasTableLink() 
{
    delete [] d_alias_p;
    delete [] d_originalName_p;
}

AliasTable::AliasTable(int size)
    : size_(size > 0 ? size : DEFAULT_TABLE_SIZE) {
  table_ = new AliasTableLink *[size_];
  assert(table_);
  memset(table_, 0, size_ * sizeof *table_);
};

AliasTable::~AliasTable() 
{
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

int AliasTable::add(const char *alias, const char *originalName) 
{
    enum { FOUND_DIFFERENT = -1, NOT_FOUND = 0, FOUND_IDENTICAL = 1 }; 

    AliasTableLink *&slot = table_[hash(alias) % size_];
    AliasTableLink *p = slot;

    while (p && 0 != strcmp(p->d_alias_p, alias)) {
        p = p->d_next_p;
    }
    if (!p) {
        slot = new AliasTableLink(alias, originalName, slot);
        return NOT_FOUND;
    } 
    else if (0 == strcmp(p->d_originalName_p, originalName)) {
        return FOUND_IDENTICAL;
    }
    else {
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
    for (it.reset(); it; ++it) {
        int len = strlen(it.GetAlias());
        if (fieldWidth < len) {
            fieldWidth = len;
        }
    }
    for (it.reset(); it; ++it) {
        o.width(fieldWidth);
        o << it.GetAlias() << " -> " << it.GetOriginalName() << std::endl;
    }
    return o;
}

AliasTableIterator::AliasTableIterator(const AliasTable& t)
    : table_(t) {
  reset();
}

AliasTableIterator::~AliasTableIterator() {
}

void AliasTableIterator::reset() {
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
