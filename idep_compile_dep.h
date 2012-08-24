#ifndef IDEP_COMPILE_DEP_H_
#define IDEP_COMPILE_DEP_H_

// This wrapper component defines 3 fully insulated classes:
//       idep_CompileDep: environment for analyzing compile-time dependencies
//     idep_RootFileIter: iterate over the specified root file names
//   idep_HeaderFileIter: iterate over the dependencies of each root file

#include "basictypes.h"

#include <ostream>

class idep_RootFileIter;
class idep_HeaderFileIter;

class idep_CompileDep_i;
class idep_CompileDep {
  public:
    // CREATORS
    idep_CompileDep();
    ~idep_CompileDep();

    // MANIPULATORS
    void addIncludeDirectory(const char *dirName);
        // Add a directory to be searched for include files.
        // Errors will be detected during the calculation process.

    int readIncludeDirectories(const char *file);
        // Add a list of include directories read from a specified file.
        // This function assumes that each contiguous sequence of
        // non-whitespace characters represents directory to be added.
        // This function returns 0 unless the specified file is unreadable
        // or contains non-ascii characters.

    void addRootFile(const char *fileName);
        // Add the name of a file to be analyzed.  Errors in reading this file
        // will be detected only when the calculate() operation is invoked.

    int readRootFiles(const char *file);
        // Add a list of root file names read from a specified file.  This
        // function assumes that each contiguous sequence of non-whitespace
        // characters represents a file name to be added.  The effect is the
        // same as if each root file name had been added individually.  This
        // function returns 0 unless the specified file is unreadable or
        // contains non-ascii characters.  Errors in reading individual root
        // files named there in will be detected only when a processing
        // operation is invoked.

    void inputRootFiles();
        // Similar to readRootFiles except that input is presumed to come
        // from <stdin>, which is reset on eof.  No check is done for
        // non-ascii characters.

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
    friend class idep_RootFileIter;
    friend class idep_HeaderFileIter;

    idep_CompileDep_i *d_this;

    DISALLOW_COPY_AND_ASSIGN(idep_CompileDep);
};

std::ostream& operator<<(std::ostream& o, const idep_CompileDep&);
    // output dependencies in standard format:
    //    A series of files is emitted one per line, with a blank line
    //    denoting the end of each series.  The first file in each series is
    //    is the root file.  Each subsequent file in the series represents a
    //    header file upon which the root file depends at compile time.

class idep_RootFileIter_i;
class idep_RootFileIter {
    idep_RootFileIter_i *d_this;

    friend class idep_HeaderFileIter;

  private:
    idep_RootFileIter(const idep_RootFileIter&);            // not implemented
    idep_RootFileIter& operator=(const idep_RootFileIter&); // not implemented

  public:
    // CREATORS
    idep_RootFileIter(const idep_CompileDep& compileDep);
    ~idep_RootFileIter();

    // MANIPULATORS
    void operator++();

    // ACCESSORS
    operator const void *() const;
    const char *operator()() const;
        // Returns the name of the current root file.
};

class idep_HeaderFileIter_i;
class idep_HeaderFileIter {
 public:
  idep_HeaderFileIter(const idep_RootFileIter& rootFileIter);
  ~idep_HeaderFileIter();

  void operator++();

  operator const void *() const;

  // Returns the name of the current file on which the current root
  // file depends (either directly or indirectly) at compile time.
  const char* operator()() const;

 private:
  idep_HeaderFileIter_i* impl_;

  DISALLOW_COPY_AND_ASSIGN(idep_HeaderFileIter);
};

#endif  // IDEP_COMPILE_DEP_H_
