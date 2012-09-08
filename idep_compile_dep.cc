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

namespace idep {

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

//typedef void (CompileDep::*Func)(const char *); // TODO 
template <typename Func> static void loadFromStream(std::istream& in, CompileDep *dep) 
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

template <typename Func> static bool loadFromFile(const char* file,
                                                  CompileDep* dep)
{
    if (!IsAsciiFile(file))
        return false;

    std::ifstream in(file);
    assert(in);
    loadFromStream<Func>(in, dep);
    return true;
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

    const char* dir_file = 0;
    for (int i = 0; i < a.Length(); ++i) {
        dir_file = search(s, a[i], file);
        if (dir_file)
            break;
    }

    return dir_file;
}

                // -*-*-*- static recursive functions -*-*-*-

// The following temporary, file-scope pointer variables are used 
// only during the recursive calls of a single static function, getDep()
// in order to avoid the unnecessary time and space costs of passing 
// several invariant arguments on the program stack.

static idep::BinaryRelation *s_dependencies_p;   // set just before first call to getDep
static idep::NameIndexMap *s_files_p;    // set just before first call to getDep
static idep::NameArray *s_includes_p;    // set just before first call to getDep
static bool s_recurse;                   // set just before first call to getDep
static std::ostream *s_err_p;                // set just before first call to getDep

static int getDep(int index) {
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

        int length = s_files_p->Length();
        int otherIndex = s_files_p->Entry(dirFile);

        if (s_files_p->Length() > length) {
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

                // -*-*-*- CompileDepImpl -*-*-*-

struct CompileDepImpl {
    idep::NameArray d_includeDirectories;      // e.g., ".", "/usr/include"
    idep::NameArray d_rootFiles;               // files to be analyzed

    idep::NameIndexMap *d_fileNames_p;         // keys for relation
    idep::BinaryRelation *d_dependencies_p;            // compile-time dependencies
    int d_numRootFiles;                       // number of roots in relation

    CompileDepImpl();
    ~CompileDepImpl();
};

CompileDepImpl::CompileDepImpl()
    : d_fileNames_p(0),
      d_dependencies_p(0),
      d_numRootFiles(-1) {
}

CompileDepImpl::~CompileDepImpl()
{
    delete d_fileNames_p;
    delete d_dependencies_p;
}

                // -*-*-*- CompileDep -*-*-*-

CompileDep::CompileDep() 
: d_this(new CompileDepImpl)
{
}

CompileDep::~CompileDep()
{
    delete d_this;
}


  //  loadFromFile(file, this, &CompileDep::addRootFile); 
      //      loadFromStream(cin, this, CompileDep::addRootFile); 

struct addIncludeDirectoryFunctor
{
  static int call(CompileDep* athis, const char* dir_name)
  {
    athis->AddIncludeDirectory(dir_name);
  }
};

struct addRootFileFunctor {
  static int call(CompileDep* compile_dep,const char* dir_name) {
    compile_dep->AddRootFile(dir_name);
  }
};

void CompileDep::AddIncludeDirectory(const char* dir_name) {
    if (*dir_name) {
        int len = strlen(dir_name);
        if ('/' == dir_name[len - 1]) {            // already ends in '/'
            d_this->d_includeDirectories.Append(dir_name);
        } else {                                  // add trailing '/'
          char* buf = new char[len + 2];
          memcpy(buf, dir_name, len);
          buf[len] = '/';
          buf[len + 1] = '\0';
          d_this->d_includeDirectories.Append(buf);
          delete[] buf;
       }
    }
}

bool CompileDep::ReadIncludeDirectories(const char* file) {
  return loadFromFile<addIncludeDirectoryFunctor>(file, this);
}

void CompileDep::AddRootFile(const char* file_name) {
    d_this->d_rootFiles.Append(file_name);
}

bool CompileDep::ReadRootFiles(const char* file) {
  //  loadFromFile(file, this, &CompileDep::addRootFile);
  return loadFromFile<addRootFileFunctor>(file, this);
}

void CompileDep::InputRootFiles() {
    if (std::cin) {
      //todo
      //      loadFromStream(cin, this, CompileDep::addRootFile);
      loadFromStream<addRootFileFunctor>(std::cin, this);

      // Reset eof for standard input.
      std::cin.clear(std::_S_goodbit);
    }
}

bool CompileDep::calculate(std::ostream& orf, bool recursionFlag) {
    bool success = true;

    // clean up any previous calculation artifacts
    delete d_this->d_fileNames_p;
    delete d_this->d_dependencies_p;

    // allocate new data structures for this calculation
    d_this->d_fileNames_p = new idep::NameIndexMap;
    d_this->d_dependencies_p = new idep::BinaryRelation;
    d_this->d_numRootFiles = 0;


    // place all root files at the start of the relation

    for (int i = 0; i < d_this->d_rootFiles.Length(); ++i) {
        std::string s;
        const char *file = d_this->d_rootFiles[i];
        const char *dirFile = search(&s, d_this->d_includeDirectories, file);

        if (!dirFile) {
            err(orf) << "root file \"" << file
                    << "\" not found." << std::endl;
            success = false;
        }
        else if (d_this->d_fileNames_p->Add(dirFile) < 0) {
            err(orf) << "root file \"" << file
                    << "\" redundantly specified." << std::endl;
            success = false;
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
            success = false;
        }
    }

    if (recursionFlag)
        d_this->d_dependencies_p->makeTransitive();

    return success;
}

std::ostream& operator<<(std::ostream& o, const CompileDep& dep)
{
    const char *INDENT = "    ";
    for (RootFileIterator rit(dep); rit; ++rit) {
        idep::NameArray a;
        o << rit() << std::endl;
        for (HeaderFileIterator hit(rit); hit; ++hit) {
            if (IsAbsolutePath(hit())) {
                a.Append(hit());
            } else {
                o << INDENT << hit() << std::endl;
            }
        }
        for (int i = 0; i < a.Length(); ++i) {
           o << INDENT << a[i] << std::endl;
        }
        o << std::endl;
    }
    return o;
}

                // -*-*-*- RootFileIteratorImpl -*-*-*-

struct RootFileIteratorImpl {
    const CompileDepImpl& d_dep;
    int d_index;

    RootFileIteratorImpl(const CompileDepImpl& dep);
};

RootFileIteratorImpl::RootFileIteratorImpl(const CompileDepImpl& dep)
    : d_dep(dep),
      d_index(0) {
}

                // -*-*-*- RootFileIterator -*-*-*-

RootFileIterator::RootFileIterator(const CompileDep& compile_dep)
    : d_this(new RootFileIteratorImpl(*compile_dep.d_this)) {
}

RootFileIterator::~RootFileIterator()
{
    delete d_this;
}

void RootFileIterator::operator++()
{
    assert(*this);
    ++d_this->d_index;
}

RootFileIterator::operator const void *() const
{
    return d_this->d_index < d_this->d_dep.d_numRootFiles ? this : 0;
}

const char *RootFileIterator::operator()() const
{
    return (*d_this->d_dep.d_fileNames_p)[d_this->d_index];
}

                // -*-*-*- HeaderFileIteratorImpl -*-*-*-

struct HeaderFileIteratorImpl {
  const RootFileIteratorImpl& d_iter;
  int d_index;

  HeaderFileIteratorImpl(const RootFileIteratorImpl& iter);
};

HeaderFileIteratorImpl::HeaderFileIteratorImpl(const RootFileIteratorImpl& iter)
    : d_iter(iter),
      d_index(-1) {
}

                // -*-*-*- HeaderFileIterator -*-*-*-

HeaderFileIterator::HeaderFileIterator(const RootFileIterator& iter)
    : impl_(new HeaderFileIteratorImpl(*iter.d_this)) {
  ++*this;
}

HeaderFileIterator::~HeaderFileIterator() {
  delete impl_;
}

void HeaderFileIterator::operator++() {
  assert(*this);
  idep::BinaryRelation* rel = impl_->d_iter.d_dep.d_dependencies_p;

  do {
    ++impl_->d_index;
  } while (impl_->d_index < rel->Length() &&
           !rel->get(impl_->d_iter.d_index, impl_->d_index)
  );
}

HeaderFileIterator::operator const void *() const {
  idep::BinaryRelation* rel = impl_->d_iter.d_dep.d_dependencies_p;
  return impl_->d_index < rel->Length() ? this : 0;
}

const char* HeaderFileIterator::operator()() const {
  return (*impl_->d_iter.d_dep.d_fileNames_p)[impl_->d_index];
}

}  // namespace idep
