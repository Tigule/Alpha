#include "Parser.h"
#include "MDLStatus.h"
#include "lex.h"

namespace MDL {
const char *TokenText(unsigned int token);
}

Parser::Parser(CMDLStatus *status, mdl_scan &scanner)
    : m_scanner(scanner), m_status(status), m_flags(0) {
}

int Parser::GetLineNumber() {
  return m_scanner.mdllineno;
}

unsigned int Parser::Token(const char **tokenText, UTokenData *data) {
  unsigned int token;
  if (m_flags & 2) {
    token = 0;
  } else {
    int scanToken = m_scanner.mdllex();
    token = scanToken;
    if (!scanToken) {
      FatalEOF();
      m_flags |= 3;
    } else if (scanToken == -2) {
      token = 0;
      m_flags |= 3;
    }
  }

  if (tokenText) {
    *tokenText = token == 0x1DF ? m_scanner.mdltext : MDL::TokenText(token);
  }
  if (data) {
    data->lVal = m_scanner.tokendata.lVal;
  }
  return token;
}

void Parser::FatalDuplicate(const char *found) {
  m_status->FatalDuplicate(found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalUnmatched(
    const char *item1,
    unsigned int count1,
    const char *item2,
    unsigned int count2
) {
  m_status->FatalUnmatched(item1, count1, item2, count2, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalNotFound(const char *expected) {
  m_status->FatalNotFound(expected, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalNotFound(unsigned int what) {
  FatalNotFound(MDL::TokenText(what));
}

void Parser::FatalUnexpected(const char *found) {
  m_status->FatalUnexpected(found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalExpected(const char *expected, const char *found) {
  m_status->FatalExpected(expected, found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalExpected(unsigned int what, const char *found) {
  FatalExpected(MDL::TokenText(what), found);
}

void Parser::FatalEOF() {
  m_status->FatalEOF(GetLineNumber());
  m_flags |= 1;
}

void Parser::WarningCount(const char *item, long expected, long actual) {
  m_status->WarningCount(item, expected, actual, GetLineNumber());
}

int Parser::FoundError() {
  return m_flags & 1;
}

void Parser::Expect(unsigned int what) {
  const char *tokentext;
  if (Token(&tokentext, 0) != what) {
    FatalExpected(what, tokentext);
  }
}

void Parser::Expect(unsigned int what, unsigned int cachedToken, const char *tokenText) {
  if (cachedToken != what) {
    FatalExpected(what, tokenText);
  }
}

long Parser::ExpectInt(unsigned int cachedToken, const char *tokenText, UTokenData *cachedValue) {
  if (cachedToken == 0x100) {
    return cachedValue->lVal;
  }
  FatalExpected(MDL::TokenText(0x100), tokenText);
  return 0;
}

long Parser::ExpectInt() {
  const char *tokentext;
  UTokenData value;
  unsigned int token = Token(&tokentext, &value);
  return ExpectInt(token, tokentext, &value);
}

float Parser::ExpectFloat() {
  const char *tokentext;
  UTokenData value;
  unsigned int token = Token(&tokentext, &value);
  if (token == 0x101) {
    return value.fVal;
  }
  if (token == 0x100) {
    return static_cast<float>(value.lVal);
  }
  FatalExpected(MDL::TokenText(0x101), tokentext);
  return 0.0f;
}

const char *Parser::ExpectString() {
  const char *tokentext;
  UTokenData value;
  if (Token(&tokentext, &value) == 0x102) {
    return value.sVal;
  }
  FatalExpected(MDL::TokenText(0x102), tokentext);
  return 0;
}

const char *Parser::ExpectString(
    unsigned int cachedToken,
    const char *tokenText,
    UTokenData *cachedValue
) {
  if (cachedToken == 0x102) {
    return cachedValue->sVal;
  }
  FatalExpected(MDL::TokenText(0x102), tokenText);
  return 0;
}

long Parser::GetOptionalInt(unsigned int *token, const char **tokenText, UTokenData *savedValue) {
  UTokenData value;
  *token = Token(tokenText, &value);
  if (*token != 0x100) {
    return -1;
  }
  long result = value.lVal;
  *token = Token(tokenText, savedValue);
  return result;
}

long Parser::GetOptionalInt(
    unsigned int cachedToken,
    UTokenData *cachedValue,
    unsigned int *token,
    const char **tokenText
) {
  if (cachedToken == 0x100) {
    *token = Token(tokenText, 0);
    return cachedValue->lVal;
  }
  *token = cachedToken;
  return -1;
}

int Parser::GetOptionalToken(unsigned int expected, unsigned int *token, const char **tokenText) {
  *token = Token(tokenText, 0);
  if (*token != expected) {
    return 0;
  }
  *token = Token(tokenText, 0);
  return 1;
}

int Parser::GetOptionalToken(
    unsigned int expected,
    unsigned int cachedToken,
    unsigned int *token,
    const char **tokenText
) {
  if (cachedToken == expected) {
    *token = Token(tokenText, 0);
    return 1;
  }
  *token = cachedToken;
  return 0;
}
