#ifndef ENGINE_SOURCE_MDLFILE_LEX_H
#define ENGINE_SOURCE_MDLFILE_LEX_H

#include <stdio.h>

#define YYNEWLINE 10

union mdl_data {
  char  cVal;
  long  lVal;
  float fVal;
  char *sVal;
};

class mdl_scan {
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

  mdl_scan(LPCSTR in, int sz);
  ~mdl_scan();

  int  mdllex();
  int  mdlgetc() {
    return *mdlin++;
  }

  virtual int mdlwrap() {
    return 1;
  }
  virtual void __cdecl mdlerror(char *fmt, ...);
  virtual void         output(int c) {
    putc(c, mdlout);
  }
  virtual void YY_FATAL(char *msg) {
    mdlerror(msg);
    mdlLexFatal = 1;
  }
  virtual void ECHO() {
    fputs(mdltext, mdlout);
  }

  int  input();
  int  unput(int c);
  void mdl_reset();
  void setinput(LPCSTR in) {
    mdlin = in;
  }
  void setoutput(FILE *out) {
    mdlout = out;
  }
  void NLSTATE() {
    mdl_lastc = YYNEWLINE;
  }
  void YY_INIT() {
    mdl_start = 0;
    mdlleng = mdl_end = 0;
    mdl_lastc = YYNEWLINE;
  }
  void YY_USER() {
    save = mdltext[mdlleng];
    mdltext[mdlleng] = 0;
  }
  void YY_SCANNER() {
    mdltext[mdlleng] = save;
  }
  void mdlless(int n) {
    if (n >= 0 && n <= mdl_end) {
      YY_SCANNER();
      mdlleng = n;
      YY_USER();
    }
  }
  void mdlcomment(char *const mat);
  int  mdlmapch(int delim, int escape);
};

#endif
