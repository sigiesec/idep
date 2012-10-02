#include "idep_compile_dep.h"

#include <stdarg.h>
#include <stdio.h>

#include <iostream>

namespace {

const char cdep_usage[] =
"cdep: Extract compile-time dependencies from a collection of files.\n"
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

const size_t kBufferSize = 2048;

void Printf(const char* msg, ...) {
  char buffer[kBufferSize + 1];
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer, kBufferSize, msg, args);
  va_end(args);
  fprintf(stderr, "%s", buffer);
}

int Missing(const char* arg_name, char option)  {
  Printf("error: missing '%s' argument for option -%c.\n", arg_name, option);
  return -1;
}

int Extra(const char* text, char option) {
  Printf("error: extra text \"%s\" encountered after -%c option.\n", text, option);
  return -1;
}

int Unreadable(const char* dir_file, char option) {
  Printf("error: unable to read \"%s\" for -%c option.\n", dir_file, option);
  return -1;
}

const char* GetArg(int* i, int argc, const char* argv[]) {
  return 0 != argv[*i][2] ? argv[*i] + 2 :
         ++*i >= argc || '-' == argv[*i][0] ? "" : argv[*i];
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    Printf(cdep_usage);
    return 0;
  }

  int file_count = 0;          // Record the number of files on the command line.
  bool read_from_file = false;      // -f<file> sets this to true.
  bool check_recursive = true;  // -x sets this to false.
  idep::CompileDep compile_dep;
  for (int i = 1; i < argc; ++i) {
    const char* word = argv[i];
    if  ('-' == word[0]) {
      char option = word[1];
      switch (option) {
        case 'I': {
          const char** p = (const char **)argv;
          const char* dir_name = GetArg(&i, argc, p);
          if (!*dir_name)
            return Missing("dir", option);

          compile_dep.AddIncludeDirectory(dir_name);
        }
        break;
        case 'i': {
          const char** p = (const char **)argv;
          const char* arg = GetArg(&i, argc, p);
          if (!*arg)
            return Missing("file", option);

          if (!compile_dep.ReadIncludeDirectories(arg))
            return Unreadable(arg, option);
        }
        break;
        case 'f': {
          const char** p = (const char **)argv;
          const char* arg = GetArg(&i, argc, p);
          if (!*arg)
            return Missing("file", option);

          if (!compile_dep.ReadRootFiles(arg))
            return Unreadable(arg, option);

          read_from_file = true;
        }
        break;
        case 'x': {
          const char** p = (const char **)argv;
          const char* arg = GetArg(&i, argc, p);
          if (*arg)
            return Extra(arg, option);

          check_recursive = false;
        }
        break;
        default: {
          Printf("error: unknown option \"%s\".\n", word);
          Printf(cdep_usage);
          return -1;
        }
        break;
      }
    } else {
      ++file_count;
      compile_dep.AddRootFile(argv[i]);
    }
  }

  if (!read_from_file && !file_count)
    compile_dep.InputRootFiles();

  int status = 0;
  if (!compile_dep.Calculate(std::cerr, check_recursive))
    status = -1;

  std::cout << compile_dep;

  return status;
}
