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

                // -*-*-*- idep_AliasTableLink -*-*-*-

struct idep_AliasTableLink {
    char *d_alias_p;                            // "from" (alias) name
    char *d_originalName_p;                     // "to" (original) name
    idep_AliasTableLink *d_next_p;              // pointer to next link
    
    idep_AliasTableLink(const char *alias, const char *orignalName, 
                                                idep_AliasTableLink *next);
    ~idep_AliasTableLink();
};

idep_AliasTableLink::idep_AliasTableLink(const char *alias, 
                         const char *originalName, idep_AliasTableLink* next) 
: d_alias_p(newStrCpy(alias)) 
, d_originalName_p(newStrCpy(originalName)) 
, d_next_p(next) 
{ 
}

idep_AliasTableLink::~idep_AliasTableLink() 
{
    delete [] d_alias_p;
    delete [] d_originalName_p;
}

                // -*-*-*- idep_AliasTable -*-*-*-


AliasTable::AliasTable(int size) 
: d_size(size > 0 ? size : DEFAULT_TABLE_SIZE)
{
    d_table_p = new idep_AliasTableLink *[d_size];
    assert (d_table_p);
    memset (d_table_p, 0, d_size * sizeof *d_table_p);
};

AliasTable::~AliasTable() 
{
    for (int i = 0; i < d_size; ++i) {
        idep_AliasTableLink *p = d_table_p[i];
        while (p) {
            idep_AliasTableLink *q = p;
            p = p->d_next_p;
            delete q;
        }
    }
    delete [] d_table_p;
}

int AliasTable::add(const char *alias, const char *originalName) 
{
    enum { FOUND_DIFFERENT = -1, NOT_FOUND = 0, FOUND_IDENTICAL = 1 }; 

    idep_AliasTableLink *&slot = d_table_p[hash(alias) % d_size];
    idep_AliasTableLink *p = slot;

    while (p && 0 != strcmp(p->d_alias_p, alias)) {
        p = p->d_next_p;
    }
    if (!p) {
        slot = new idep_AliasTableLink(alias, originalName, slot);
        return NOT_FOUND;
    } 
    else if (0 == strcmp(p->d_originalName_p, originalName)) {
        return FOUND_IDENTICAL;
    }
    else {
        return FOUND_DIFFERENT;
    }
}

const char* AliasTable::lookup(const char *alias) const
{
    idep_AliasTableLink *p = d_table_p[hash(alias) % d_size];
    while (p && 0 != strcmp(p->d_alias_p, alias)) {
        p = p->d_next_p;
    }
    return p ? p->d_originalName_p : 0;
}

std::ostream& operator<<(std::ostream &o, const AliasTable& table)
{
    int fieldWidth = 0;
    AliasTableIterator it(table);
    for (it.reset(); it; ++it) {
        int len = strlen(it.alias());
        if (fieldWidth < len) {
            fieldWidth = len;
        }
    }
    for (it.reset(); it; ++it) {
        o.width(fieldWidth);
        o << it.alias() << " -> " << it.originalName() << std::endl;
    }
    return o;
}

AliasTableIterator::AliasTableIterator(const AliasTable& t)
: d_table(t)
{
    reset();
}

AliasTableIterator::~AliasTableIterator()
{
}

void AliasTableIterator::reset()
{
    d_link_p = 0;
    d_index = -1;
    ++*this;
}

void AliasTableIterator::operator++()
{
    if (d_link_p) {
        d_link_p = d_link_p->d_next_p;
    }
    while (!d_link_p && *this) {
        ++d_index;
        if (*this) {
            d_link_p = d_table.d_table_p[d_index];
        }
    }
}

AliasTableIterator::operator const void *() const
{
    return d_index < d_table.d_size ? this : 0;
}

const char* AliasTableIterator::alias() const
{
    return d_link_p->d_alias_p;
}

const char* AliasTableIterator::originalName() const
{
    return d_link_p->d_originalName_p;
}
