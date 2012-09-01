#ifndef IDEP_COMPILE_DEP_H_
#define IDEP_COMPILE_DEP_H_

// This wrapper component defines 3 fully insulated classes:
//       CompileDep: environment for analyzing compile-time dependencies
//     RootFileIterator: iterate over the specified root file names
//   HeaderFileIterator: iterate over the dependencies of each root file

#include "basictypes.h"

#include <ostream>

namespace idep {

class RootFileIter;
class HeaderFileIterator;

class CompileDepImpl;
class CompileDep {
 public:
  CompileDep();
  ~CompileDep();

  // Add a directory to be searched for include files.
  // Errors will be detected during the calculation process.
  void AddIncludeDirectory(const char* dir_name);

  // Add a list of include directories read from a specified file.
  // This function assumes that each contiguous sequence of
  // non-whitespace characters represents directory to be added.
  // This function returns 0 unless the specified file is unreadable
  // or contains non-ascii characters.
  int ReadIncludeDirectories(const char* file);

  // Add the name of a file to be analyzed.  Errors in reading this file
  // will be detected only when the calculate() operation is invoked.
  void AddRootFile(const char* file_name);

  // Add a list of root file names read from a specified file.  This
  // function assumes that each contiguous sequence of non-whitespace
  // characters represents a file name to be added.  The effect is the
  // same as if each root file name had been added individually.  This
  // function returns 0 unless the specified file is unreadable or
  // contains non-ascii characters.  Errors in reading individual root
  // files named there in will be detected only when a processing
  // operation is invoked.
  int readRootFiles(const char *file);

  // Similar to readRootFiles except that input is presumed to come
  // from <stdin>, which is reset on eof.  No check is done for
  // non-ascii characters.
  void inputRootFiles();

  // Calculate compile-time dependencies among the specified set of
  // rootfiles. Return true on success, false on error.  Errors will
  // be printed to the indicated output stream (err).  By default,
  // calculation of dependencies will recurse even for files (such as
  // (compiler supplied headers) defined outside the current directory.
  // Specifying 0 as the value of the optional second argument suppresses
  // recursive investigation of dependencies within external files.
  // Note that turning off recursion is potentially much faster, but
  // provides an incomplete list of compile-time dependencies.
  bool calculate(std::ostream& err, bool recursion_flag);

 private:
  friend class RootFileIterator;
  friend class HeaderFileIterator;

  CompileDepImpl *d_this;

  DISALLOW_COPY_AND_ASSIGN(CompileDep);
};

// output dependencies in standard format:
//    A series of files is emitted one per line, with a blank line
//    denoting the end of each series.  The first file in each series is
//    is the root file.  Each subsequent file in the series represents a
//    header file upon which the root file depends at compile time.
std::ostream& operator<<(std::ostream& o, const CompileDep&);

class RootFileIteratorImpl;
class RootFileIterator {
 public:
  RootFileIterator(const CompileDep& compile_dep);
  ~RootFileIterator();

  void operator++();

  operator const void *() const;

  // Returns the name of the current root file.
  const char* operator()() const;

 private:
  friend class HeaderFileIterator;

  RootFileIteratorImpl *d_this;

  DISALLOW_COPY_AND_ASSIGN(RootFileIterator);
};

class HeaderFileIteratorImpl;
class HeaderFileIterator {
 public:
  HeaderFileIterator(const RootFileIterator& iterator);
  ~HeaderFileIterator();

  void operator++();

  operator const void *() const;

  // Returns the name of the current file on which the current root
  // file depends (either directly or indirectly) at compile time.
  const char* operator()() const;

 private:
  HeaderFileIteratorImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(HeaderFileIterator);
};

}  // namespace idep

#endif  // IDEP_COMPILE_DEP_H_
