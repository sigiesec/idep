#include "idep_alias_dep.h"

#include <iostream>

// This file contains a main program to exercise the idep_aliasdep component.

#define NL "\n"
static const char* help() {
return NL
"adep - create aliases to group files into cohesive components."             NL
""                                                                           NL
"  The following 3 command line interface modes are supported:"              NL
""                                                                           NL
"    adep [-s] [-a<alias>] [-f<filelist> ] [-X<fn>] [-x<xFile>] <filename>*" NL
"    adep -v [-a<alias>] [-f<filelist>] [-X<fn>] [-x<xFile>] <cfilename>*"   NL
"    adep -e [-a<alias>] [-f<filelist>] [-X<fn>] [-x<xFile>] <cfilename>*"   NL
""                                                                           NL
"      -s           Suppress the printing of suffixes for unpaired names."   NL
"      -v           Verify file contains component name as 1st dependency."  NL
"      -e           Extract aliases using name of first dependency."         NL
"      -a<alias>    Specify file containing component name aliases."         NL
"      -f<filelist> Specify file containing a list of files to consider."    NL
"      -X<fn>       Specify name of file to ignore during processing."       NL
"      -x<xFile>    Specify file containing a list of filenames to ignore."  NL
""                                                                           NL
"    Each filename on the command line specifies a file to be considered for"NL
"    processing.  Specifying no arguments indicates that the list of files"  NL
"    is to come from standard input unless the -f option has been invoked."  NL
""                                                                           NL
"  TYPICAL USAGE:"                                                           NL
""                                                                           NL
"    adep -s *.[ch]                     // print unpaired files to stdout"   NL
""                                                                           NL
"    adep -v -aaliases *.c              // print #include errors to stderr"  NL
""                                                                           NL
"    adep -e *.c                        // print extracted aliases to stderr"NL
NL;
}

static enum { IOERROR = -1, GOOD = 0, BAD = 1 } s_status = GOOD;

static std::ostream& PrintError() {
  s_status = IOERROR;
  return std::cerr << "error: ";
}

static int missing(const char *argName, char option) {
  PrintError() << "missing `" << argName << "' argument for -"
      << option << " option." << std::endl;
  return s_status;
}

static int extra(const char *text, char option) {
  PrintError() << "extra text \"" << text << "\" encountered after -"
      << option << " option." << std::endl;
  return s_status;
}

static int unreadable(const char *dirFile, char option) {
  PrintError() << "unable to read \"" << dirFile << "\" for -"
      << option << " option." << std::endl;
  return s_status;
}

static int incorrect(const char *file, char option) {
  PrintError() << "file \"" << file << "\" contained invalid contents for -"
      << option << " option." << std::endl;
    return s_status;
}

static const char *getArg(int *i, int argc, const char *argv[]) {
  return 0 != argv[*i][2] ? argv[*i] + 2 :
      ++*i >= argc || '-' == argv[*i][0] ? "" : argv[*i];
}

int main(int argc, char* argv[]) {
    int argCount = 0;        // record the number of files on the command line
    int fileFlag = 0;        // -f<file> sets this to 1
    int suffixFlag = 1;      // -s sets this to 0
    int verifyFlag = 0;      // -v sets this to 1
    int extractFlag = 0;     // -e sets this to 1

    idep::AliasDep environment;
    for (int i = 1; i < argc; ++i) {
        const char *word = argv[i];
        if  ('-' == word[0]) {
            char option = word[1];
            switch(option) {
              case 'X': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("dir", option);
                }
                environment.addIgnoreName(arg);
              } break;
              case 'x': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("file", option);
                }
                if (0 != environment.readIgnoreNames(arg)) {
                    return unreadable(arg, option);
                }
              } break;
              case 'a': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("file", option);
                }
                int s = environment.readAliases(std::cerr, arg);
                if (s < 0) {
                    return unreadable(arg, option);
                }
                if (s > 0) {
                    return incorrect(arg, option);
                }
              } break;
              case 'f': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("file", option);
                }
                if (0 != environment.readFileNames(arg)) {
                    return unreadable(arg, option);
                }
                fileFlag = 1;
              } break;
              case 's': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                suffixFlag = 0;
              } break;
              case 'v': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                verifyFlag = 1;
              } break;
              case 'e': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                extractFlag = 1;
              } break;
              default: {
                 PrintError() << "unknown option \"" << word << "\"." << std::endl
                              << help();
                 return s_status;
              } break;
            }
        }
        else {
            ++argCount;
            environment.addFileName(argv[i]);
        }
    }

    if (!fileFlag && !argCount) {
        environment.inputFileNames();
    }

    int result = extractFlag ? environment.extract(std::cout, std::cerr)
                             : verifyFlag
                             ? environment.verify(std::cerr)
                             : environment.unpaired(std::cout, std::cerr, suffixFlag);

    s_status = result < 0 ? IOERROR : result > 0 ? BAD : GOOD;

    return s_status;
}
