#ifndef ENGINE_SOURCE_MDLFILE_PARSER_H
#define ENGINE_SOURCE_MDLFILE_PARSER_H

#include <Base/Base.h>

class CMDLStatus;
class mdl_scan;
union UTokenData {
  char  cVal;
  long  lVal;
  float fVal;
  char *sVal;
};

class Parser {
 public:
  Parser(CMDLStatus *status, mdl_scan &scanner);

  void   FatalDuplicate(LPCSTR found);
  void   FatalUnmatched(LPCSTR item1, UINT count1, LPCSTR item2, UINT count2);
  void   FatalNotFound(UINT what);
  void   FatalNotFound(LPCSTR expected);
  void   FatalUnexpected(LPCSTR found);
  void   FatalExpected(UINT what, LPCSTR found);
  void   FatalExpected(LPCSTR expected, LPCSTR found);
  void   FatalEOF();
  void   WarningCount(LPCSTR item, long expected, long actual);
  BOOL   FoundError();
  void   Expect(UINT what, UINT cachedToken, LPCSTR tokenText);
  void   Expect(UINT what);
  long   ExpectInt(UINT cachedToken, LPCSTR tokenText, UTokenData *cachedValue);
  long   ExpectInt();
  float  ExpectFloat();
  LPCSTR ExpectString(UINT cachedToken, LPCSTR tokenText, UTokenData *cachedValue);
  LPCSTR ExpectString();
  long   GetOptionalInt(UINT cachedToken, UTokenData *cachedValue, UINT *token, LPCSTR *tokenText);
  long   GetOptionalInt(UINT *token, LPCSTR *tokenText, UTokenData *savedValue);
  BOOL   GetOptionalToken(UINT expected, UINT cachedToken, UINT *token, LPCSTR *tokenText);
  BOOL   GetOptionalToken(UINT expected, UINT *token, LPCSTR *tokenText);
  UINT   Token(LPCSTR *tokenText, UTokenData *data);
  int    GetLineNumber();

 private:
  Parser &operator=(const Parser &);

  mdl_scan   &m_scanner;
  CMDLStatus *m_status;
  UINT        m_flags;
};

#endif
