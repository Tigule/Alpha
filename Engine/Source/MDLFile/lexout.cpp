#include <Base/Base.h>

#include "lex.h"

#include <storm.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

namespace MDL {
  LPCSTR TokenText(UINT token);
}

mdl_scan::mdl_scan(LPCSTR in, int inputSize) {
  size = inputSize;
  mdltext = static_cast<char *>(SMemAlloc(size + 1, "new", -1, 0));
  state = static_cast<UINT *>(SMemAlloc(sizeof(UINT) * (size + 1), "new", -1, 0));
  if (!mdltext || !state) {
    mdlerror("out of dynamic memory in mdl_scan");
    exit(1);
  }
  mustfree = 1;
  mdl_end = 0;
  mdl_start = 0;
  mdl_lastc = '\n';
  mdlLexFatal = 0;
  save = 0;
  tokendata.lVal = 0;
  mdlin = in;
  mdllineno = 1;
  mdlout = stdout;
  mdlleng = 0;
}

mdl_scan::~mdl_scan() {
  if (mustfree) {
    mustfree = 0;
    if (mdltext) {
      SMemFree(mdltext, "delete", -1, 0);
    }
    if (state) {
      SMemFree(state, "delete", -1, 0);
    }
  }
}

void __cdecl mdl_scan::mdlerror(char *format, ...) {
  va_list args;
  va_start(args, format);
  if (mdllineno) {
    fprintf(stderr, "%d: ", mdllineno);
  }
  vfprintf(stderr, format, args);
  fputc('\n', stderr);
  va_end(args);
}

int mdl_scan::mdlgetc() {
  return input();
}

int mdl_scan::input() {
  int result = static_cast<BYTE>(*mdlin);
  if (result) {
    ++mdlin;
  }
  mdl_lastc = result;
  if (result == '\n') {
    ++mdllineno;
  }
  return result;
}

int mdl_scan::unput(int character) {
  if (mdlin && mdl_lastc) {
    --mdlin;
  }
  mdl_lastc = character;
  if (character == '\n') {
    --mdllineno;
  }
  return character;
}

void mdl_scan::mdl_reset() {
  mdl_start = 0;
  mdl_end = 0;
  mdlleng = 0;
  mdl_lastc = '\n';
  mdllineno = 1;
}

void mdl_scan::setinput(LPCSTR inputBuffer) {
  mdlin = inputBuffer;
}

void mdl_scan::setoutput(FILE *outputFile) {
  mdlout = outputFile;
}

void mdl_scan::NLSTATE() {
}

void mdl_scan::YY_INIT() {
}

void mdl_scan::YY_USER() {
}

void mdl_scan::YY_SCANNER() {
}

void mdl_scan::mdlless(int count) {
  while (mdlleng > count) {
    unput(static_cast<BYTE>(mdltext[--mdlleng]));
  }
  mdltext[mdlleng] = 0;
}

void mdl_scan::mdlcomment(char *material) {
  char *scan = material;
  while (*scan) {
    int character = input();
    if (!character) {
      mdlerror("end of file in comment");
      return;
    }
    if (character == *scan) {
      ++scan;
    } else {
      scan = material;
      if (character == *material) {
        ++scan;
      }
    }
  }
}

int mdl_scan::mdlmapch(int character, int) {
  return character;
}

int mdl_scan::mdllex() {
  for (;;) {
    int character = input();
    if (!character) {
      mdlleng = 0;
      mdltext[0] = 0;
      return 0;
    }
    if (isspace(character)) {
      continue;
    }

    mdlleng = 0;
    mdltext[mdlleng++] = static_cast<char>(character);

    if (character == '/') {
      int next = input();
      if (next == '/') {
        while ((character = input()) != 0 && character != '\n') {
        }
        continue;
      }
      if (next == '*') {
        mdlcomment("*/");
        continue;
      }
      unput(next);
    }

    if (character == '"') {
      mdlleng = 0;
      while ((character = input()) != 0 && character != '"') {
        if (character == '\\') {
          int escaped = input();
          if (!escaped) {
            break;
          }
          character = escaped;
        }
        if (mdlleng < size) {
          mdltext[mdlleng++] = static_cast<char>(character);
        }
      }
      mdltext[mdlleng] = 0;
      tokendata.sVal = mdltext;
      return 0x102;
    }

    if (isdigit(character) || character == '-' || character == '+' || (character == '.' && isdigit(static_cast<BYTE>(*mdlin)))) {
      BOOL isFloat = character == '.';
      while (*mdlin) {
        int next = static_cast<BYTE>(*mdlin);
        if (!isdigit(next) && next != '.' && next != 'e' && next != 'E' && next != '-' && next != '+') {
          break;
        }
        if (next == '.' || next == 'e' || next == 'E') {
          isFloat = 1;
        }
        if (mdlleng < size) {
          mdltext[mdlleng++] = *mdlin;
        }
        ++mdlin;
      }
      mdltext[mdlleng] = 0;
      if (isFloat) {
        tokendata.fVal = static_cast<float>(strtod(mdltext, 0));
        return 0x101;
      }
      tokendata.lVal = strtol(mdltext, 0, 10);
      return 0x100;
    }

    if (isalpha(character) || character == '_') {
      while (isalnum(static_cast<BYTE>(*mdlin)) || *mdlin == '_') {
        if (mdlleng < size) {
          mdltext[mdlleng++] = *mdlin;
        }
        ++mdlin;
      }
      mdltext[mdlleng] = 0;
      for (UINT token = 0x103; token < 0x1DF; ++token) {
        if (!strcmp(mdltext, MDL::TokenText(token))) {
          return token;
        }
      }
      return 0x1DF;
    }

    mdltext[mdlleng] = 0;
    return character;
  }
}
