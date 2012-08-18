#include "idep_compile_dep.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include <fstream>
#include <iostream>

#include "idep_binary_relation.h"
#include "idep_file_dep_iterator.h"
#include "idep_name_array.h"
#include "idep_name_index_map.h"
#include "idep_token_iterator.h"

static std::ostream& err(std::ostream& orf) {
    return orf << "Error: ";
}

static const char *stripDotSlash(const char *originalPath) {
    if (originalPath) {
        while ('.' == originalPath[0] && '/' == originalPath[1]) {
            originalPath += 2;
        }
    }
    return originalPath;
}

static bool IsAbsolutePath(const char* original_path) {
    return '/' == *original_path;
}

static int isAsciiFile(const char *fileName) {
    enum { NO = 0, YES = 1 };
    std::ifstream in(fileName);
    if (!in) {
        return NO;
    }

    // check for non-ascii characters
    char c;
    while (in && !in.get(c).eof()) {
       if (!isascii(c)) {
          return NO;
       }
    }
    return YES;
}

//typedef void (idep_CompileDep::*Func)(const char *); // TODO 
template <typename Func> static void loadFromStream(std::istream& in, idep_CompileDep *dep) 
{ 
    assert(in);
    for (idep::TokenIterator it(in); it; ++it) {
        if ('#' == *it()) {                     // strip comment if any
             while (it && '\n' != *it()) { 
                ++it;
            }
            continue;                           // either !it or '\n' 
        }
        if ('\n' != *it()) {            
            Func::call(dep,it());
        }
    }
}

template <typename Func> static int loadFromFile(const char *file, idep_CompileDep *dep) 
{
    enum { BAD = -1, GOOD = 0 };
    if (!isAsciiFile(file)) {
        return BAD;
    }
    std::ifstream in(file);
    assert(in);
    loadFromStream<Func>(in, dep);
    return GOOD;
}

static const char* search(std::string* s,
                          const char* includeDir,
                          const char* file) {
    assert(!IsAbsolutePath(file));
    (*s = includeDir) += file;
    const char *dirFile = stripDotSlash((*s).c_str());
    return std::ifstream(dirFile) ? dirFile : 0;
}

static const char* search(std::string* s,
                          const idep::NameArray& a,
                          const char* file) {
    if (IsAbsolutePath(file)) {
        *s = file;
        const char *absFile = (*s).c_str();
        return std::ifstream(absFile) ? absFile : 0;
    }

    const char *dirFile = 0;
    for (int i = 0; i < a.length(); ++i) {
        dirFile = search(s, a[i], file);
        if (dirFile) {
            break;
        }
    }

    return dirFile;
}

                // -*-*-*- static recursive functions -*-*-*-

// The following temporary, file-scope pointer variables are used 
// only during the recursive calls of a single static function, getDep()
// in order to avoid the unnecessary time and space costs of passing 
// several invariant arguments on the program stack.

static idep_BinRel *s_dependencies_p;   // set just before first call to getDep
static idep_NameIndexMap *s_files_p;    // set just before first call to getDep
static idep::NameArray *s_includes_p;    // set just before first call to getDep
static int s_recurse;                   // set just before first call to getDep
static std::ostream *s_err_p;                // set just before first call to getDep

static int getDep (int index) 
{
    enum { BAD = -1, GOOD = 0 } status = GOOD;
     
    std::string buffer; // string buffer, do not use directly

    idep::FileDepIterator it((*s_files_p)[index]);
    for (it; it; ++it) {
        const char *dirFile = search(&buffer, *s_includes_p, it());
        if (!dirFile) {
            err(*s_err_p) << "include directory for file \""
                 << it() << "\" not specified." << std::endl; 
            status = BAD;
            continue;
        }

        int length = s_files_p->length();
        int otherIndex = s_files_p->entry(dirFile);

        if (s_files_p->length() > length) {
            // first time looking at this file
            s_dependencies_p->appendEntry();
                
            if (s_recurse && getDep(otherIndex)) {
                status = BAD;
            }
        }

        s_dependencies_p->set(index, otherIndex, 1);
    }

    if (!it.IsValidFile()) {
       err(*s_err_p) << "unable to open file \""
         << (*s_files_p)[index] << "\" for read access." << std::endl; 
        status = BAD;
    }

    return status;
}
                    
                // -*-*-*- idep_CompileDep_i -*-*-*-

struct idep_CompileDep_i {
    idep::NameArray d_includeDirectories;      // e.g., ".", "/usr/include"
    idep::NameArray d_rootFiles;               // files to be analyzed

    idep_NameIndexMap *d_fileNames_p;         // keys for relation
    idep_BinRel *d_dependencies_p;            // compile-time dependencies
    int d_numRootFiles;                       // number of roots in relation

    idep_CompileDep_i();
    ~idep_CompileDep_i();
};

idep_CompileDep_i::idep_CompileDep_i() 
: d_fileNames_p(0)
, d_dependencies_p(0)
, d_numRootFiles(-1)
{
}

idep_CompileDep_i::~idep_CompileDep_i() 
{
    delete d_fileNames_p;
    delete d_dependencies_p;
}

                // -*-*-*- idep_CompileDep -*-*-*-

idep_CompileDep::idep_CompileDep() 
: d_this(new idep_CompileDep_i)
{
}

idep_CompileDep::~idep_CompileDep()
{
    delete d_this;
}


  //  loadFromFile(file, this, &idep_CompileDep::addRootFile); 
      //      loadFromStream(cin, this, idep_CompileDep::addRootFile); 

struct addIncludeDirectoryFunctor
{
  static int call(idep_CompileDep * athis,const char *dirName)      
  {
    athis->addIncludeDirectory(dirName);
  }  
};

struct addRootFileFunctor
{
  static int call(idep_CompileDep * athis,const char *dirName)      
  {
    athis->addRootFile(dirName);
  }  
};



void idep_CompileDep::addIncludeDirectory(const char *dirName)
{
    if (*dirName) {
        int len = strlen(dirName);
        if ('/' == dirName[len-1]) {            // already ends in '/'
            d_this->d_includeDirectories.append(dirName);               
        }
        else {                                  // add trailing '/'
            char *buf = new char[len+2];
            memcpy(buf, dirName, len);
            buf[len] = '/';
            buf[len+1] = '\0';
            d_this->d_includeDirectories.append(buf);           
            delete [] buf;
        }
    }
}

int idep_CompileDep::readIncludeDirectories(const char *file)
{
  //TODO return 
  loadFromFile<addIncludeDirectoryFunctor>(file, this); 
}

void idep_CompileDep::addRootFile(const char *fileName)
{
    d_this->d_rootFiles.append(fileName);               
}

int idep_CompileDep::readRootFiles(const char *file)
{
  //TODO return 
  //  loadFromFile(file, this, &idep_CompileDep::addRootFile); 
  loadFromFile<addRootFileFunctor>(file, this); 
}

void idep_CompileDep::inputRootFiles()
{
    if (std::cin) {
      //todo 
      //      loadFromStream(cin, this, idep_CompileDep::addRootFile);
      loadFromStream<addRootFileFunctor>(std::cin, this); 

      std::cin.clear(std::_S_goodbit);             // reset eof for standard input
    }
}

int idep_CompileDep::calculate(std::ostream& orf, int recursionFlag)
{
    enum { BAD = -1, GOOD = 0 } status = GOOD;

    // clean up any previous calculation artifacts
    delete d_this->d_fileNames_p;       
    delete d_this->d_dependencies_p;

    // allocate new data structures for this calculation
    d_this->d_fileNames_p = new idep_NameIndexMap;
    d_this->d_dependencies_p = new idep_BinRel;     
    d_this->d_numRootFiles = 0;


    // place all root files at the start of the relation

    for (int i = 0; i < d_this->d_rootFiles.length(); ++i) {
        std::string s;
        const char *file = d_this->d_rootFiles[i];
        const char *dirFile = search(&s, d_this->d_includeDirectories, file);

        if (!dirFile) {
            err(orf) << "root file \"" << file 
                    << "\" not found." << std::endl;
            status = BAD;
        }
        else if (d_this->d_fileNames_p->add(dirFile) < 0) { 
            err(orf) << "root file \"" << file 
                    << "\" redundantly specified." << std::endl;
            status = BAD;
        }
        else {
            ++d_this->d_numRootFiles;
            d_this->d_dependencies_p->appendEntry();
        }
    }
        
    // We must now investigate the compile-time dependencies for each 
    // translation unit recursively.  First we will set up several
    // file-scope pointers to reduce recursive overhead.

    s_dependencies_p = d_this->d_dependencies_p;
    s_files_p = d_this->d_fileNames_p;
    s_includes_p = &d_this->d_includeDirectories;
    s_recurse = recursionFlag;
    s_err_p = &orf;

    // Each translation unit forms the root of a tree of dependencies.
    // We will visit each node only once, recording the results as we go.
    // Initially, only the translation units are present in the relation.

    for (int i = 0; i < d_this->d_numRootFiles; ++i) {
        const char *name = (*d_this->d_fileNames_p)[i];
        if (getDep(i)) {
            err(orf) << "could not determine all dependencies for \""
                    << name << "\"." << std::endl;
            status = BAD;
        }
    }

    if (recursionFlag) {
        d_this->d_dependencies_p->makeTransitive();
    }

    return status;
}

std::ostream& operator<<(std::ostream& o, const idep_CompileDep&(dep))
{
    const char *INDENT = "    ";
    for (idep_RootFileIter rit(dep); rit; ++rit) {
        idep::NameArray a;
        o << rit() << std::endl;
        for (idep_HeaderFileIter hit(rit); hit; ++hit) {
            if (IsAbsolutePath(hit())) {
                a.append(hit());
            } else {
                o << INDENT << hit() << std::endl;
            }
        }
        for (int i = 0; i < a.length(); ++i) {
           o << INDENT << a[i] << std::endl;
        }
        o << std::endl;
    }
    return o;
}

                // -*-*-*- idep_RootFileIter_i -*-*-*-

struct idep_RootFileIter_i {
    const idep_CompileDep_i& d_dep;
    int d_index;

    idep_RootFileIter_i(const idep_CompileDep_i& dep);
};

idep_RootFileIter_i::idep_RootFileIter_i(const idep_CompileDep_i& dep)
: d_dep(dep)
, d_index(0)
{
}

                // -*-*-*- idep_RootFileIter -*-*-*-

idep_RootFileIter::idep_RootFileIter(const idep_CompileDep& dep) 
: d_this(new idep_RootFileIter_i(*dep.d_this))
{
}

idep_RootFileIter::~idep_RootFileIter()
{
    delete d_this;
}

void idep_RootFileIter::operator++() 
{
    assert(*this);
    ++d_this->d_index;
}
 
idep_RootFileIter::operator const void *() const
{
    return d_this->d_index < d_this->d_dep.d_numRootFiles ? this : 0;
}
 
const char *idep_RootFileIter::operator()() const
{
    return (*d_this->d_dep.d_fileNames_p)[d_this->d_index];
}

                // -*-*-*- idep_HeaderFileIter_i -*-*-*-

struct idep_HeaderFileIter_i {
    const idep_RootFileIter_i& d_iter;
    int d_index;

    idep_HeaderFileIter_i(const idep_RootFileIter_i& iter);
};

idep_HeaderFileIter_i::idep_HeaderFileIter_i(const idep_RootFileIter_i& iter) 
: d_iter(iter)
, d_index(-1)
{
}

                // -*-*-*- idep_HeaderFileIter -*-*-*-

idep_HeaderFileIter::idep_HeaderFileIter(const idep_RootFileIter& iter) 
: d_this(new idep_HeaderFileIter_i(*iter.d_this))
{
    ++*this;
}

idep_HeaderFileIter::~idep_HeaderFileIter()
{
    delete d_this;
}


void idep_HeaderFileIter::operator++() 
{
    assert(*this);
    idep_BinRel *rel = d_this->d_iter.d_dep.d_dependencies_p;
    
    do {
        ++d_this->d_index;
    }
    while (   d_this->d_index < rel->length() 
           && !rel->get(d_this->d_iter.d_index, d_this->d_index)
    );
}
 
idep_HeaderFileIter::operator const void *() const
{
    idep_BinRel *rel = d_this->d_iter.d_dep.d_dependencies_p;
    return d_this->d_index < rel->length() ? this : 0;
}
 
const char *idep_HeaderFileIter::operator()() const
{
    return (*d_this->d_iter.d_dep.d_fileNames_p)[d_this->d_index];
}

