#ifndef IDEP_FILE_DEP_ITERATOR_H_
#define IDEP_FILE_DEP_ITERATOR_H_

namespace idep {

class FileDepIteratorImpl;

// This component defines 1 fully insulated iterator class:
// Iterate over the header files included by a file.
class FileDepIterator {
 public:
  // Create a compile-time dependency iterator for the specified file.
  // The filenames in preprocessor include directives will be presented
  // in the order in which they appear in the file.  Dependencies that
  // are conditionally compiled or commented out with multi-line
  // /* ... */ comments will none-the-less be returned by this iterator.
  FileDepIterator(const char* fileName);
  ~FileDepIterator();

  // Return to the first dependency in the file (if one exists).
  void reset();

  // Return non-zero if the specified file is valid/readable; else 0.
  bool isValidFile() const;

  // Advance to next dependency in the file.  The behavior of this
  // function is undefined if the iteration state is invalid.
  void operator++();

  // Return non-zero if the current iteration state (i.e., dependency)
  // is valid; else 0.  Note that this function will correctly return 0
  // if the file itself is not valid.
  operator const void *() const;

  // Return the name of current file on which this file depends.
  // If the iteration state is not valid, 0 is returned.
  const char* operator()() const;

 private:
  FileDepIteratorImpl* impl_;

  // Disallow copy and assign.
  FileDepIterator(const FileDepIterator&);
  FileDepIterator& operator=(const FileDepIterator&);
};

}  // namespace idep

#endif  // IDEP_FILE_DEP_ITERATOR_H_
