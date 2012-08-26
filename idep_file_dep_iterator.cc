#include "idep_file_dep_iterator.h"

#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>

#include <fstream>

// Arbitrary maximum length for line containing an include directive. Note that
// other lines may be longer.
enum { MAX_LINE_LENGTH  = 2048 };

static int loadBuf(std::istream& in, char* buf, int bufSize) {
    enum { END_OF_INPUT = -1, SUCCESS = 0, OVERFLOW = 1 };

    // The getline (istream) method returns the line as a string but extracts
    // the trailing '\n'  only if there is space to spare in the buffer.  We
    // need to test for this condition by placing a non-null character in the 
    // last position of the array before loading it.  If the last character is 
    // null after the read then the newline was NOT extracted.

    int nearEnd = bufSize - 1;

    buf[nearEnd] = '\n'; // anything but '\0'

    if (in.getline(buf, bufSize).fail()) {
        return END_OF_INPUT;    // nothing left
    }

    if ('\0' == buf[nearEnd]) { // line was too long
        char c;
        while (in && !in.get(c).eof() && c != '\n') { 
            // skip to next line
        }
        return OVERFLOW;        // buffer contains null-terminated string
    }

    return SUCCESS;
}

const char *extractDependency(char *buffer) {
    // We assume we have a null terminated string that possibly contains a 
    // valid include directive.  We will assume that such a directive has 
    // the following syntax:
    // 
    // ^#[ \t]*"include"[ \t ]*[<"][ \t]*{filename}[>" \t\n]
    //                                   ~~~~~~~~~~
    // i.e.,                             ^want this
    //  1. The first character on the line MUST be a '#'
    //  2. This character may be followed by any number of spaces or tabs.
    //  3. The next non-whitespace char must be an 'i' followed by "nclude".
    //  4. This string may be followed by any number of spaces or tabs.
    //  5. The next character must be either a '<' or a '"'. 
    //  6. This character may be followed by any number of spaces or tabs.
    //  7. The {filename} follows and is terminated by whitespace, '>', or '"'.
    //
    // If an include directive is found, the buffer is modified and a pointer 
    // to the included filename is returned; otherwise 0 is returned. 

    if ('#' != buffer[0]) {                             // 1.
        return 0;
    }

    char *p = buffer;           
    while (isspace(*++p)) {                             // 2.
    }                           

    if ('i' != *p) {                                    // 3a.
        return 0;
    }

    static char KEY[] = "include";
    if (0 != strncmp(p, "include", sizeof KEY - 1)) {   // 3b.
        return 0;
    }
    p += sizeof KEY - 1;        // advance over KEY

    while (isspace(*p)) {                               // 4.
        ++p;
    }                           

    if ('<' != *p && '"' != *p) {                       // 5.
        return 0;
    }
    ++p;                        

    while (isspace(*p)) {                               // 6.
        ++p;
    }                           

    // At this point, p points to the start of the file name
    // all we need to do is detect the end of the string. 

    int length = strcspn(p, " \t\">"); 
    p[length] = 0;                                      // 7.

    return p;
}

namespace idep {

struct FileDepIteratorImpl {
  std::ifstream d_file;
  char d_buf[MAX_LINE_LENGTH];
  const char *d_header_p;
  bool is_valid_file;

  FileDepIteratorImpl(const char* file_name);
};

FileDepIteratorImpl::FileDepIteratorImpl(const char* file_name)
    : d_file(file_name),
      d_header_p(d_buf)  /* Buffer is not yet initialized. */,
      is_valid_file(d_file != NULL) /* Depends on result of initialization. */ {
}

FileDepIterator::FileDepIterator(const char *fileName)
    : impl_(new FileDepIteratorImpl(fileName)) {
  if (!IsValidFile())
    impl_->d_header_p = 0; // Iteration state is invalid.

  ++*this; // load first occurrence
}

FileDepIterator::~FileDepIterator() {
    delete impl_;
}

void FileDepIterator::Reset() {
  if (IsValidFile()) {
    impl_->d_file.seekg(std::ios::beg); // rewind to beginning of file
    impl_->d_file.clear(impl_->d_file.rdstate() & std::ios::badbit);
    impl_->d_header_p = impl_->d_buf;
  }
  ++*this; // load first occurrence
}

bool FileDepIterator::IsValidFile() const {
  return impl_->is_valid_file;
}

void FileDepIterator::operator++() {
  impl_->d_header_p = 0;
  while (loadBuf(impl_->d_file, impl_->d_buf, sizeof impl_->d_buf) >= 0) {
    if (impl_->d_header_p = extractDependency(impl_->d_buf)) { // `=' ok
      break;
    }
  }
}

FileDepIterator::operator const void *() const {
  return impl_->d_header_p ? this : 0;
}

const char* FileDepIterator::operator()() const {
  return impl_->d_header_p;
}

}  // namespace idep
