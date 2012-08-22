#include "idep_compile_dep.h"

#include <iostream>

// This file contains a main program to exercise the idep_compile_dep component.

const char cdep_usage[] =
"\ncdep: Extract compile-time dependencies from a collection of files.\n"
"\n"
"  The following command line interface is supported:\n"
"\n"
"    cdep [-I<dir>] [-i<dirlist>] [-f<filelist>] [-x] <filename>*\n"
"\n"
"      -I<dir>      Specify include directory to search.\n"
"      -i<dirlist>  Specify file containing a list of directories to search.\n"
"      -f<filelist> Specify file containing a list of files to process.\n"
"      -x           Do _not_ check recursively for nested includes.\n"
"\n"
"    Each filename on the command line specifies a file to be considered for\n"
"    processing.  Specifying no arguments indicates that the list of files\n"
"    is to come from standard input unless the -f option has been invoked.\n"
"\n"
"  TYPICAL USAGE:\n"
"\n"
"    cdep -iincludes *.[ch]\n\n";

static enum { IOERROR = -1, GOOD = 0 } s_status = GOOD;

static std::ostream& err() {
  s_status = IOERROR;
  return std::cerr << "error: ";
}

static int missing(const char* argName, char option)  {
  err() << "missing `" << argName << "' argument for -"
        << option << " option." << std::endl;
  return s_status;
}

static int extra(const char* text, char option) {
  err() << "extra text \"" << text << "\" encountered after -"
        << option << " option." << std::endl;
  return s_status;
}

static int unreadable(const char* dirFile, char option) {
  err() << "unable to read \"" << dirFile << "\" for -"
        << option << " option." << std::endl;
  return s_status;
}

static const char* getArg(int* i, int argc, const char* argv[]) {
  return 0 != argv[*i][2] ? argv[*i] + 2 :
         ++*i >= argc || '-' == argv[*i][0] ? "" : argv[*i];
}

int main(int argc, char* argv[]) {
  int file_count = 0;          // Record the number of files on the command line.
  bool file_flag = false;      // -f<file> sets this to true.
  bool recursion_flag = true;  // -x sets this to false.
  idep_CompileDep environment;
  for (int i = 1; i < argc; ++i) {
    const char* word = argv[i];
    if  ('-' == word[0]) {
      char option = word[1];
      switch(option) {
        case 'I': {
          const char** p = (const char **)argv;
          const char* arg = getArg(&i, argc, p);
          if (!*arg)
            return missing("dir", option);

          environment.addIncludeDirectory(arg);
        }
        break;
        case 'i': {
          const char** p = (const char **)argv;
          const char* arg = getArg(&i, argc, p);
          if (!*arg)
            return missing("file", option);

          if (0 != environment.readIncludeDirectories(arg))
            return unreadable(arg, option);
        }
        break;
        case 'f': {
          const char ** p = (const char **)argv;
          const char *arg = getArg(&i, argc, p);
          if (!*arg)
            return missing("file", option);

          if (0 != environment.readRootFiles(arg))
            return unreadable(arg, option);

          file_flag = true;
        }
        break;
        case 'x': {
          const char ** p = (const char **)argv;
          const char *arg = getArg(&i, argc, p);
          if (*arg)
            return extra(arg, option);

          recursion_flag = false;
        }
        break;
        default: {
          err() << "unknown option \"" << word << "\"." << std::endl
                << cdep_usage;
          return s_status;
        }
        break;
      }
    } else {
      ++file_count;
      environment.addRootFile(argv[i]);
    }
  }

  if (!file_flag && !file_count)
    environment.inputRootFiles();

  if (environment.calculate(std::cerr, recursion_flag))
    s_status = IOERROR;

  std::cout << environment;

  return s_status;
}
