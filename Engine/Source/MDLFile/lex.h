#ifndef ENGINE_SOURCE_MDLFILE_LEX_H
#define ENGINE_SOURCE_MDLFILE_LEX_H

#include <stdio.h>

union mdl_data {
  char cVal;
  long lVal;
  float fVal;
  char *sVal;
};

class mdl_scan {
 public:
  mdl_scan(const char *input, int inputSize);
  ~mdl_scan();

  virtual int mdlwrap() {
    return 1;
  }
  virtual void __cdecl mdlerror(char *format, ...);
  virtual void output(int character) {
    putc(character, mdlout);
  }
  virtual void YY_FATAL(char *message) {
    mdlerror(message);
    mdlLexFatal = 1;
  }
  virtual void ECHO() {
    fputs(mdltext, mdlout);
  }

  int mdllex();
  int mdlgetc();
  int input();
  int unput(int character);
  void mdl_reset();
  void setinput(const char *);
  void setoutput(FILE *);
  void NLSTATE();
  void YY_INIT();
  void YY_USER();
  void YY_SCANNER();
  void mdlless(int);
  void mdlcomment(char *material);
  int mdlmapch(int character, int count);

 protected:
  unsigned int *state;
  int size;
  int mustfree;
  int mdl_end;
  int mdl_start;
  int mdl_lastc;
  int mdlLexFatal;
  char save;

 public:
  mdl_data tokendata;
  char *mdltext;
  const char *mdlin;
  FILE *mdlout;
  int mdllineno;
  int mdlleng;
};

#endif
