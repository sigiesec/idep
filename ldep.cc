#include "idep_link_dep.h"

#include <iostream>

// This file contains a main program to exercise the idep_link_dep component.

static const char* help() {
return
"\nldep: Analyze the link-time dependencies among a collection of components.\n"
"\n"
"  The following command line interface is supported:\n"
"\n"
"    ldep [-U<dir>] [-u<un>] [-a<aliases>] [-d<deps>] [-l|-L] [-x|-X] [-s]\n"
"\n"
"      -U<dir>     Specify directory not to group as a package.\n"
"      -u<un>      Specify file containing directories not to group.\n"
"      -a<aliases> Specify file containg list of component name aliases.\n"
"      -d<deps>    Specify file containg list of compile-time dependencies.\n"
"      -l          Long listing: provide non-redundant list of dependencies.\n"
"      -L          Long listing: provide complete list of dependencies.\n"
"      -x          Suppress printing any alias/unalias information.\n"
"      -X          Suppress printing all but the levelized component names.\n"
"      -s          Do _not_ remove suffixes; consider each file separately.\n"
"\n"
"    This command takes no arguments.  The dependencies themselves will\n"
"    come from standard input unless the -d option has been invoked.\n"
"\n"
"  TYPICAL USAGE:\n"
"\n"
"    ldep -aaliases -ddependencies\n\n";
}

static enum { IOERROR = -1, SUCCESS = 0, DESIGN_ERROR = 1 } s_status = SUCCESS;

static std::ostream& PrintError() {
  s_status = IOERROR;
  return std::cerr << "error: ";
}

static int missing(const char* arg_name, char option) {
    PrintError() << "missing `" << arg_name << "' argument for -"
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
    int fileFlag = 0;        // -d<file> sets this to 1
    int longListingFlag = 0; // both -l and -L set this to 1
    int canonicalFlag = 1;   // -L sets this to 0 and -l sets it back to 1
    int suffixFlag = 1;      // -s sets this to 1
    int suppression = 0;     // -x sets this to 1; -X sets it to 2.
    idep_LinkDep environment;
    for (int i = 1; i < argc; ++i) {
        const char *word = argv[i];
        if  ('-' == word[0]) {
            char option = word[1];
            switch(option) {
              case 'U': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("dir", option);
                }
                environment.addUnaliasDirectory(arg);
              } break;
              case 'u': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("file", option);
                }
                if (0 != environment.readUnaliasDirectories(arg)) {
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
              case 'd': {
                const char *arg = getArg(&i, argc, (const char **)argv);
                if (!*arg) {
                    return missing("file", option);
                }
                environment.addDependencyFile(arg);
                fileFlag = 1;
              } break;
              case 'l': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                canonicalFlag = 1;
                longListingFlag = 1;
              } break;
              case 'L': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                longListingFlag = 1;
                canonicalFlag = 0;
              } break;
              case 's': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                suffixFlag = 0;
              } break;
              case 'x': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                suppression = 1;
              } break;
              case 'X': {
                const char *arg = word + 2;
                if (*arg) {
                    return extra(arg, option);
                }
                suppression = 2;
              } break;
              default: {
                 PrintError() << "unknown option \"" << word << "\"." << std::endl
                       << help();
                 return s_status;
              } break;
            }
        }
        else {
             PrintError() << "illegal argument \"" << word << "\"." << std::endl
                   << help();
             return s_status;
        }
    }

    if (!fileFlag) {
        environment.addDependencyFile(""); // "" is synonym for standard input
    }

    int result = environment.calculate(std::cerr, canonicalFlag, suffixFlag);

    s_status = result < 0 ? IOERROR : result > 0 ? DESIGN_ERROR : SUCCESS; 

    if (s_status >= 0) {
        if (0 == suppression) {
            environment.printAliases(std::cout);
            environment.printUnaliases(std::cout);
        }
        environment.printCycles(std::cerr);
        environment.printLevels(std::cout, longListingFlag, suppression >= 2);
        if (suppression <= 1) {
            environment.printSummary(std::cout);
        }
    }

    return s_status;
}
