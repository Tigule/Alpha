#ifndef ENGINE_SOURCE_MDLFILE_PARSER_H
#define ENGINE_SOURCE_MDLFILE_PARSER_H

class CMDLStatus;
class mdl_scan;
union UTokenData {
  char cVal;
  long lVal;
  float fVal;
  char *sVal;
};

class Parser {
 public:
  Parser(CMDLStatus *status, mdl_scan &scanner);

  void FatalDuplicate(const char *found);
  void FatalUnmatched(const char *item1, unsigned int count1, const char *item2, unsigned int count2);
  void FatalNotFound(unsigned int what);
  void FatalNotFound(const char *expected);
  void FatalUnexpected(const char *found);
  void FatalExpected(unsigned int what, const char *found);
  void FatalExpected(const char *expected, const char *found);
  void FatalEOF();
  void WarningCount(const char *item, long expected, long actual);
  int FoundError();
  void Expect(unsigned int what, unsigned int cachedToken, const char *tokenText);
  void Expect(unsigned int what);
  long ExpectInt(unsigned int cachedToken, const char *tokenText, UTokenData *cachedValue);
  long ExpectInt();
  float ExpectFloat();
  const char *ExpectString(unsigned int cachedToken, const char *tokenText, UTokenData *cachedValue);
  const char *ExpectString();
  long GetOptionalInt(unsigned int cachedToken, UTokenData *cachedValue, unsigned int *token, const char **tokenText);
  long GetOptionalInt(unsigned int *token, const char **tokenText, UTokenData *savedValue);
  int GetOptionalToken(unsigned int expected, unsigned int cachedToken, unsigned int *token, const char **tokenText);
  int GetOptionalToken(unsigned int expected, unsigned int *token, const char **tokenText);
  unsigned int Token(const char **tokenText, UTokenData *data);
  int GetLineNumber();

 private:
  Parser &operator=(const Parser &);

  mdl_scan &m_scanner;
  CMDLStatus *m_status;
  unsigned int m_flags;
};

#endif
