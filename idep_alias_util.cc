#include "idep_alias_util.h"

#include <assert.h>

#include <fstream>   // ifstream
#include <iostream>

#include "idep_alias_table.h"
#include "idep_token_iterator.h"

namespace idep {

const char kEmptyName[] = "";
const char kNullChar= *kEmptyName;
const char kCommentChar= '#';
const char kContinueChar= '\\';
const char kNewLineChar= '\n';

static std::ostream& warning(std::ostream& orf,
                             const char* file,
                             int line_number) {
  return orf << "Warning in " << file << '(' << line_number << "): ";
}

static std::ostream& err(std::ostream& orf, const char* file, int line_number) {
  return orf << "Error in " << file << '(' << line_number << "): ";
}

static int TryToAlias(AliasTable* table,
                      std::ostream& orf,
                      const char* input_name,
                      int line_number,
                      const char* component_name,
                      const char* alias) {
  if (table->Add(alias, component_name) < 0) {
    const char* previous_name = table->Lookup(alias);
    err(orf, input_name, line_number) << "two names for alias \""
        << alias << "\":" << std::endl << "    \"" << previous_name
        << "\" and \"" << component_name << "\"" << std::endl;
    return 1;
  }
  return 0;
}

int AliasUtil::ReadAliases(AliasTable* table,
                           std::ostream& orf,
                           std::istream& in,
                           const char* input_name) {
    // The following is a state-machine description of the alias language:

    enum State {
        START,          // starting a new alias
        CONTINUE_0,     // continue - no previous identifiers
        IDENT_0,        //    ident - no previous identifiers
        CONTINUE_1,     // continue - one previous identifier
        CONTINUE_NL,    // continue - newline terminated
        IDENT_NL,       //    ident - newline terminated
        NEWLINE_BL,     //  newline - blank line terminated
        CONTINUE_BL,    // continue - blank line terminated
        IDENT_BL,       //    ident - blank line terminated
        NUM_STATES      // must be last entry
    } state = START;

    enum Input {
        CONTINUE,       // continuation character by it self
        NEWLINE,        // end of line
        IDENT,          // indentifier (not the previous two)
        NUM_INPUTS      // must be last entry
    } input;

    enum Action {
        NOP,            // do nothing
        BEG_CUR,        // set component name to current token
        BEG_PRE,        // set component name to previous token
        BEG_PRE_CUR,    // set component name to previous and try current token
        TRY_CUR,        // try to alias current token to component name
        TRY_PRE,        // try to alias previous token to component name
        TRY_PRE_CUR,    // try to alias both tokens to component name
        END,            // unset component name
        NUM_ACTIONS     // must be last entry
    };

    static State nextStateTable[NUM_STATES][NUM_INPUTS] = {
        //CONTINUE      NEWLINE         IDENT
        { CONTINUE_0,   START,          IDENT_0         }, // START
        { CONTINUE_1,   START,          IDENT_NL        }, // CONTINUE_0
        { CONTINUE_1,   NEWLINE_BL,     IDENT_NL        }, // IDENT_0
        { CONTINUE_NL,  IDENT_0,        IDENT_NL        }, // CONTINUE_1
        { CONTINUE_NL,  IDENT_NL,       IDENT_NL        }, // CONTINUE_NL
        { CONTINUE_NL,  START,          IDENT_NL        }, // IDENT_NL
        { CONTINUE_BL,  START,          IDENT_BL        }, // NEWLINE_BL
        { CONTINUE_BL,  IDENT_BL,       IDENT_BL        }, // CONTINUE_BL
        { CONTINUE_BL,  NEWLINE_BL,     IDENT_BL        }  // IDENT_BL
    };

    static Action actionTable[NUM_STATES][NUM_INPUTS] = {
        //CONTINUE      NEWLINE         IDENT
        { NOP,          NOP,            BEG_CUR         }, // START
        { BEG_PRE,      NOP,            BEG_PRE_CUR     }, // CONTINUE_0
        { NOP,          NOP,            TRY_CUR         }, // IDENT_0
        { TRY_PRE,      NOP,            TRY_PRE_CUR     }, // CONTINUE_1
        { TRY_PRE,      NOP,            TRY_PRE_CUR     }, // CONTINUE_NL
        { NOP,          END,            TRY_CUR         }, // IDENT_NL
        { NOP,          END,            TRY_CUR         }, // NEWLINE_BL
        { TRY_PRE,      NOP,            TRY_PRE_CUR     }, // CONTINUE_BL
        { NOP,          NOP,            TRY_CUR         }  // IDENT_BL
    };

    int numBadAliases = 0;
    int lineno = 1;

    std::string componentName(kEmptyName);
    std::string lastToken(kEmptyName);
    Input lastInput = IDENT;

    for (TokenIterator it(in); it; ++it) {
        if (*it() == kCommentChar) {
            while (*it() != kNewLineChar) {
                ++it;                   // ignore all tokens until newline
            }
        }

        // Determine the input type:

        switch (*it()) {
          case kContinueChar: {
            input = (kNullChar == it()[1]) ? CONTINUE : IDENT;
          } break;
          case kNewLineChar: {
            input = NEWLINE;
          } break;
          default: {
            input = IDENT;
          } break;
        };

        // Perform the selected action:

        switch (actionTable[state][input]) {
          case NOP: {
          } break;
          case BEG_CUR: {
            componentName = it();
          } break;
          case BEG_PRE: {
            componentName = lastToken;
            warning(orf, input_name, lineno) << '"' << lastToken 
                << "\" << used as component name." << std::endl;
          } break;
          case BEG_PRE_CUR: {
            componentName = lastToken;
            numBadAliases += TryToAlias(table, orf, input_name, lineno,
                                                        componentName.c_str(), it());
            warning(orf, input_name, lineno) << '"' << lastToken 
                << "\" << used as component name." << std::endl;
          } break;
          case TRY_CUR: {
            numBadAliases += TryToAlias(table, orf, input_name, lineno,
                                                        componentName.c_str(), it());
          } break;
          case TRY_PRE: {
            numBadAliases += TryToAlias(table, orf, input_name, lineno,
                                                   componentName.c_str(), lastToken.c_str());
            warning(orf, input_name, lineno) << '"' << lastToken
                << "\" << used as alias name." << std::endl;
          } break;
          case TRY_PRE_CUR: {
            numBadAliases += TryToAlias(table, orf, input_name, lineno,
                                                   componentName.c_str(), lastToken.c_str());
            numBadAliases += TryToAlias(table, orf, input_name, lineno,
                                                        componentName.c_str(), it());
            warning(orf, input_name, lineno) << '"' << lastToken
                << "\" << used as alias name." << std::endl;
          } break;
          case END: {
            componentName = kEmptyName;
          } break;
          default: 
          case NUM_ACTIONS: {
            assert(0);
          }
        };

        // Advance to the next state:
        if (NEWLINE == input) {
            ++lineno;                           // end of line
        }
        lastToken = it();
        lastInput = input;

        state = nextStateTable[state][input];
    }

    return numBadAliases;                       // 0 on success
}


int AliasUtil::ReadAliases(AliasTable* table,
                           std::ostream& orf,
                           const char* file_name) {
  enum { IOERROR = -1 };
  std::ifstream in(file_name);
  if (!in)
    return IOERROR;

  return ReadAliases(table, orf, in, file_name);
}

}  // namespace idep
