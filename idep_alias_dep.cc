#include "idep_alias_dep.h"

#include <assert.h>
#include <ctype.h>      // isascii() isspace()
#include <memory.h>     // memcpy()
#include <string.h>     // strlen() strrchr()

#include <fstream>
#include <iostream>

#include "idep_alias_table.h"
#include "idep_alias_util.h"
#include "idep_file_dep_iterator.h"
#include "idep_name_array.h"
#include "idep_name_index_map.h"
#include "idep_token_iterator.h"

namespace idep {

std::ostream& warn(std::ostream& ing)
{
    return ing << "Warning: ";
}

static std::ostream & err(std::ostream& ors)
{
    return ors << "Error: ";
}

static const char *stripDotSlash(const char *originalPath)
{
    if (originalPath) {
        while ('.' == originalPath[0] && '/' == originalPath[1]) {
            originalPath += 2;
        }
    }
    return originalPath;
}

static const char *stripDir(const char *s)
{
    if (s) {
        const char *slash = strrchr(s, '/');
        return slash ? slash + 1 : s;
    }
    return s;   // input was null
}

static int IsAsciiFile(const char* file_name) {
    std::ifstream in(file_name);
    if (!in)
        return false;

    // Check for non-ascii characters.
    char c;
    while (in && !in.get(c).eof()) {
       if (!isascii(c))
          return false;
    }
    return true;
}

typedef void (AliasDep::*Func)(const char *);
static void loadFromStream(std::istream& in, AliasDep *dep, Func add)
{
    assert(in);
    for (TokenIterator it(in); it; ++it) {
        if ('#' == *it()) {                     // strip comment if any
             while (it && '\n' != *it()) {
                ++it;
            }
            continue;                           // either !it or '\n'
        }
        if ('\n' != *it()) {
            (dep->*add)(it());
        }
    }
}

static int loadFromFile(const char *file, AliasDep *dep, Func add)
{
    enum { BAD = -1, GOOD = 0 };
    if (!IsAsciiFile(file)) {
        return BAD;
    }
    std::ifstream in(file);
    assert(in);
    loadFromStream(in, dep, add);
    return GOOD;
}

static char* NewStrCpy(const char* old_str) {
  int size = strlen(old_str) + 1;
  char* new_str = new char[size];
  memcpy(new_str, old_str, size);
  return new_str;
}

static char *removeSuffix(char *dirPath)
{
    char *dot = strrchr(dirPath, '.');
    if (dot && !strchr(dot, '/')) {     // if '.' found in final path segment
        dot[0] = '\0';                  // eliminate suffix ("a/.b" -> "a/")
    }
    return dirPath;
}

                // -*-*-*- AliasDepIntArray -*-*-*-

class AliasDepIntArray {   // auxiliary class to manage array memory
    int *d_array_p;
    int length_;
    AliasDepIntArray(const AliasDepIntArray&);
    AliasDepIntArray& operator=(const AliasDepIntArray&);
  public:
    AliasDepIntArray(int length) : // does not zero the memory!
			d_array_p(new int[length]), length_(length) {}
    ~AliasDepIntArray() { delete [] d_array_p; }
    int& operator[](int i) { return d_array_p[i]; }
    int Length() const { return length_; }
};

static void zero(AliasDepIntArray *a) // non-primitive operation on array
{
    for (int i = 0; i < a->Length(); ++i) {
        (*a)[i] = 0;
    }
}

                // -*-*-*- AliasDepString -*-*-*-

class AliasDepString {     // auxiliary class to manage modifiable char *
    char *d_string_p;
    AliasDepString(const AliasDepString&);
    AliasDepString& operator=(const AliasDepString&);
  public:
    AliasDepString(const char *s) : d_string_p(NewStrCpy(s)){}
    ~AliasDepString() { delete [] d_string_p; }
    operator char *() { return d_string_p; }
};

struct AliasDepImpl {
    NameIndexMap d_ignoreNames;          // e.g., idep_compile_dep_unittest.cc
    AliasTable d_aliases;                // e.g., my_inta -> my_intarray
    NameIndexMap d_fileNames;            // files to be analyzed
};

                // -*-*-*- AliasDep -*-*-*-

AliasDep::AliasDep()
: impl_(new AliasDepImpl)
{
}

AliasDep::~AliasDep()
{
    delete impl_;
}

void AliasDep::addIgnoreName(const char *fileName)
{
    impl_->d_ignoreNames.Add(fileName);
}

int AliasDep::readIgnoreNames(const char *file) {
  return loadFromFile(file, this, &AliasDep::addIgnoreName);
}

const char *AliasDep::addAlias(const char *alias, const char *component) {
  return impl_->d_aliases.Add(alias, component) < 0 ?
      impl_->d_aliases.Lookup(alias) : 0;
}

int AliasDep::readAliases(std::ostream& orf, const char *file)
{
    return AliasUtil::ReadAliases(&impl_->d_aliases, orf, file);
}

void AliasDep::addFileName(const char *fileName)
{
    impl_->d_fileNames.Add(fileName);
}

int AliasDep::readFileNames(const char *file) {
  return loadFromFile(file, this, &AliasDep::addFileName);
}

void AliasDep::inputFileNames()
{
    if (std::cin) {
      //        loadFromStream(cin, this, AliasDep::addFileName);
      //        cin.clear(0);             // reset eof for standard input
    }
}

int AliasDep::unpaired(std::ostream& out, std::ostream& ing, int suffixFlag) const
{
    int maxLength = impl_->d_fileNames.Length();
    AliasDepIntArray hits(maxLength);  // records num files per component
    AliasDepIntArray cmap(maxLength);  // map component to (last) file
    zero(&hits);
    NameIndexMap components;
    int numComponents = 0;

    NameIndexMap printNames;   // Used to sort names for ease of use
                               // during cut and past in the editor.

    for (int i = 0; i < maxLength; ++i) {
        AliasDepString s(impl_->d_fileNames[i]);

        if (impl_->d_ignoreNames.GetIndexByName(s) >= 0) {
            continue; // ignore this file
        }
        removeSuffix(s);

        const char *componentName = impl_->d_aliases.Lookup(s);
        if (!componentName) {
            componentName = s;
        }

        int componentIndex = components.Entry(componentName);
        if (components.Length() > numComponents) {      // new component
            ++numComponents;

        }

        assert(components.Length() == numComponents);

        ++hits[componentIndex];
        cmap[componentIndex] = i; // overwrite with most recent index
    }

    for (int i = 0; i < numComponents; ++i) {
        assert(hits[i] > 0);
        if (1 == hits[i]) {
            printNames.Add(suffixFlag ? impl_->d_fileNames[cmap[i]]
                                      : components[i]);
        }
        if (hits[i] > 2) {
            warn(ing) << "component \"" << components[i]
                      << "\" consists of " << hits[i] << " files." << std::endl;
        }
    }

    // Because of library .o file name-length limitations it is often the
    // header which has the longer name representing the true name of the
    // component.  If the suffixFlag is 0, we will sort into almost
    // lexicographic order except that the shorter of two initially identical
    // names will *follow* rather than precede longer.  This ordering will
    // facilitate cut and past when creating an alias file by hand in a
    // text editor.

    int numUnpaired = printNames.Length();
    AliasDepIntArray smap(numUnpaired);
    for (int i = 0; i < numUnpaired; ++i) {
        smap[i] = i;                            // identity mapping to start
    }

    for (int i = 1; i < numUnpaired; ++i) {
        for (int j = 0; j < numUnpaired; ++j) {
            int swap;
            if (suffixFlag) {
                swap = strcmp(printNames[smap[i]], printNames[smap[j]]) < 0;
            }
            else {
                int li = strlen(printNames[smap[i]]);
                int lj = strlen(printNames[smap[j]]);
                int len = li < lj ? li : lj;    // min length
                int cmp = strncmp(printNames[smap[i]],
                                  printNames[smap[j]], len);
                swap = (cmp < 0 || 0 == cmp) && li > lj;  // longer first if tie
            }
            if (swap) {                                 // swap if necessary
                int tmp = smap[i];
                smap[i] = smap[j];
                smap[j] = tmp;
            }
        }
    }

    // print out names in (almost) lexicographic order (if suffixFlag set to 0)

    for (int i = 0; i < numUnpaired; ++i) {
        out << printNames[smap[i]] << std::endl;
    }

    return printNames.Length();
}

static const char *th(int n)
{
    return 1 == n ? "st" : 2 == n ? "nd" : 3 == n ? "rd" : "th";
}

int AliasDep::verify(std::ostream& orf) const
{
    enum { IOERROR = -1, GOOD = 0 } status = GOOD;
    int errorCount = 0; // keep track of the number of readable faulty files

    int length = impl_->d_fileNames.Length();
    for (int i = 0; i < length; ++i) {
        const char *path = impl_->d_fileNames[i];
        AliasDepString c(path);

        if (impl_->d_ignoreNames.GetIndexByName(c) >= 0) {
            continue; // ignore this file
        }

        // strip off suffix and path from component file name and check aliases
        removeSuffix(c);
        const char *actualComponent = stripDir(c);

	char temp[355];
	//	#= actualComponent;
	strcpy (temp,actualComponent);
        const char *compAlias = impl_->d_aliases.Lookup(temp);

        const char *component = compAlias ? compAlias : actualComponent;

        int directiveIndex = 0;

	FileDepIterator it(path);

        for (it; it; ++it) {

            ++directiveIndex;

            // strip off suffix and path from header name and check aliases
            AliasDepString h(it());
            removeSuffix(h);
            const char *actualHeader = stripDir(h);
            const char *headerAlias = impl_->d_aliases.Lookup(actualHeader);
            const char *header = headerAlias ? headerAlias : actualHeader;

            if (0 == strcmp(component, header)) { // if the same, we found it
                break;
            }
        }

        if (!it.IsValidFile()) { // if the file was never valid to begin with
            err(orf) << "unable to open file \""
                    << path << "\" for read access." << std::endl;
            status = IOERROR;
        }
        else if (!it) {                         // header not found
            err(orf) << "corresponding include directive for \"" << path
                    << "\" not found."
                    << std::endl;
            ++errorCount;
        }
        else if (1 != directiveIndex) {         // header found but not first
            err(orf) << '"' << path
                    << "\" contains corresponding include as "
                    << directiveIndex << th(directiveIndex)
                    << " directive." << std::endl;
            ++errorCount;
        }

        // else there is nothing wrong here
    }

    return status == GOOD ? errorCount : status;
}


int AliasDep::extract(std::ostream& out, std::ostream& orf) const
{
    enum { IOERROR = -1, GOOD = 0 } status = GOOD;
    enum { INVALID_INDEX = -1 };
    int errorCount = 0; // keep track of number of readable faulty files

    NameIndexMap uniqueHeaders;       // used to detect multiple .c files
    int length = impl_->d_fileNames.Length();
    AliasDepIntArray hits(length);    // records frequency of headers
    zero(&hits);
    AliasDepIntArray hmap(length);    // index header file index in table
    AliasDepIntArray verified(length);// verifies that guess was correct
    zero(&verified);

    for (int i = 0; i < length; ++i) {
        hmap[i] = INVALID_INDEX;   // set valid when a suitable header is found

        const char *path = impl_->d_fileNames[i];
        AliasDepString c(path);

        if (impl_->d_ignoreNames.GetIndexByName(c) >= 0) {
            continue; // ignore this file
        }

        // strip off suffix and path from component file name and check aliases
        removeSuffix(c);
        const char *actualComponent = stripDir(c);
        const char *compAlias = impl_->d_aliases.Lookup(actualComponent);
        const char *component = compAlias ? compAlias : actualComponent;

        FileDepIterator it(path);      // hook up with first dependency.

        if (!it.IsValidFile()) {        // unable to read file
            err(orf) << "unable to open file \""
                    << path << "\" for read access." << std::endl;
            status = IOERROR;
            continue;                   // nothing more we can do here
        }

        if (!it) {                      // no include directives
            err(orf) << '"' << path
                    << "\" contains no include directives." << std::endl;
            ++errorCount;
            continue;                   // nothing more we can do here
        }

        // strip off suffix and path from header name and check aliases
        AliasDepString h(it());
        removeSuffix(h);
        const char *actualHeader = stripDir(h);
        const char *headerAlias = impl_->d_aliases.Lookup(actualHeader);
        const char *header = headerAlias ? headerAlias : actualHeader;

        if (0 == strcmp(component, header)) {

            // At this point, we have the component name and header name
            // that match either because the root names were matching or
            // because we found an alias that made it work.  Record this
            // fact in the verified array.

            verified[i] = 1;
        }
        else {

            // We suspect this may be an alias pair, but we are not sure.
            // We will check to see if an alias involving this .c file
            // already exists.  If so, that will override and we will
            // not regenerate the alias.

            if (compAlias) {
                continue;               // nothing more we can do here
            }
        }

        // We have no reason *not* to think this is a valid match (yet).
        // Record this header as being associated with the this .c file.

        int hIndex = uniqueHeaders.Entry(header); // obtaine index of header
        ++hits[hIndex];                           // record frequency
        hmap[i] = hIndex;                         // set .c -> header index
    }

    const int FW = 25;
    const char *const ARROW = ">- probably correct -> ";

    // For each unique header, if more than one .c file names this header
    // int its first include directive, output a warning to the error stream.

    for (int i = 0; i < uniqueHeaders.Length(); ++i) {
        if (hits[i] > 1) {
            warn(orf) << hits[i] << " files specify \"" << uniqueHeaders[i]
                 << "\" as their first include directive:" << std::endl;
            for (int j = 0; j < length; ++j) {
                if (i ==  hmap[j]) {
                    orf.width(FW);
                    orf << (verified[j] ? ARROW : "");
                    orf << '"' << stripDir(impl_->d_fileNames[j])
                       << '"' << std::endl;
                }
            }
            orf << std::endl;
        }
    }

    // Print the non-redundant header / implementation file aliases to
    // the specified output stream.

    for (int i = 0; i < length; ++i) {
        if (hmap[i] >= 0 && !verified[i]) {
           // strip off suffix and path from component file name
           AliasDepString c(impl_->d_fileNames[i]);
           removeSuffix(c);
           out << uniqueHeaders[hmap[i]] << ' ' << c << std::endl;
        }
    }

    return status == GOOD ? errorCount : status;
}

}  // namespace idep
