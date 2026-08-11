#ifndef ENGINE_SOURCE_MDLFILE_LEX_H
#define ENGINE_SOURCE_MDLFILE_LEX_H

#include <stdio.h>

union mdl_data {
  char  cVal;
  long  lVal;
  float fVal;
  char *sVal;
};

class mdl_scan {
 public:
  mdl_scan(LPCSTR input, int inputSize);
  ~mdl_scan();

  virtual int mdlwrap() {
    return 1;
  }
  virtual void __cdecl mdlerror(char *format, ...);
  virtual void         output(int character) {
    putc(character, mdlout);
  }
  virtual void YY_FATAL(char *message) {
    mdlerror(message);
    mdlLexFatal = 1;
  }
  virtual void ECHO() {
    fputs(mdltext, mdlout);
  }

  int  mdllex();
  int  mdlgetc() {
    return input();
  }
  int  input();
  int  unput(int character);
  void mdl_reset();
  void setinput(LPCSTR inputBuffer) {
    mdlin = inputBuffer;
  }
  void setoutput(FILE *outputFile) {
    mdlout = outputFile;
  }
  void NLSTATE() {
  }
  void YY_INIT() {
  }
  void YY_USER() {
  }
  void YY_SCANNER() {
  }
  void mdlless(int count) {
    while (mdlleng > count) {
      unput(static_cast<BYTE>(mdltext[--mdlleng]));
    }
    mdltext[mdlleng] = 0;
  }
  void mdlcomment(char *material);
  int  mdlmapch(int character, int) {
    return character;
  }

 protected:
  UINT *state;
  int   size;
  BOOL  mustfree;
  int   mdl_end;
  int   mdl_start;
  int   mdl_lastc;
  int   mdlLexFatal;
  char  save;

 public:
  mdl_data tokendata;
  char    *mdltext;
  LPCSTR   mdlin;
  FILE    *mdlout;
  int      mdllineno;
  int      mdlleng;
};

#endif
