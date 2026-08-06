#include "Parser.h"
#include "MDLStatus.h"
#include "lex.h"

namespace MDL {
  LPCSTR TokenText(UINT token);
}

Parser::Parser(CMDLStatus *status, mdl_scan &scanner) : m_scanner(scanner), m_status(status), m_flags(0) {
}

int Parser::GetLineNumber() {
  return m_scanner.mdllineno;
}

UINT Parser::Token(LPCSTR *tokenText, UTokenData *data) {
  UINT token;
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

void Parser::FatalDuplicate(LPCSTR found) {
  m_status->FatalDuplicate(found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalUnmatched(LPCSTR item1, UINT count1, LPCSTR item2, UINT count2) {
  m_status->FatalUnmatched(item1, count1, item2, count2, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalNotFound(LPCSTR expected) {
  m_status->FatalNotFound(expected, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalNotFound(UINT what) {
  FatalNotFound(MDL::TokenText(what));
}

void Parser::FatalUnexpected(LPCSTR found) {
  m_status->FatalUnexpected(found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalExpected(LPCSTR expected, LPCSTR found) {
  m_status->FatalExpected(expected, found, GetLineNumber());
  m_flags |= 1;
}

void Parser::FatalExpected(UINT what, LPCSTR found) {
  FatalExpected(MDL::TokenText(what), found);
}

void Parser::FatalEOF() {
  m_status->FatalEOF(GetLineNumber());
  m_flags |= 1;
}

void Parser::WarningCount(LPCSTR item, long expected, long actual) {
  m_status->WarningCount(item, expected, actual, GetLineNumber());
}

int Parser::FoundError() {
  return m_flags & 1;
}

void Parser::Expect(UINT what) {
  LPCSTR tokentext;
  if (Token(&tokentext, 0) != what) {
    FatalExpected(what, tokentext);
  }
}

void Parser::Expect(UINT what, UINT cachedToken, LPCSTR tokenText) {
  if (cachedToken != what) {
    FatalExpected(what, tokenText);
  }
}

long Parser::ExpectInt(UINT cachedToken, LPCSTR tokenText, UTokenData *cachedValue) {
  if (cachedToken == 0x100) {
    return cachedValue->lVal;
  }
  FatalExpected(MDL::TokenText(0x100), tokenText);
  return 0;
}

long Parser::ExpectInt() {
  LPCSTR     tokentext;
  UTokenData value;
  UINT       token = Token(&tokentext, &value);
  return ExpectInt(token, tokentext, &value);
}

float Parser::ExpectFloat() {
  LPCSTR     tokentext;
  UTokenData value;
  UINT       token = Token(&tokentext, &value);
  if (token == 0x101) {
    return value.fVal;
  }
  if (token == 0x100) {
    return static_cast<float>(value.lVal);
  }
  FatalExpected(MDL::TokenText(0x101), tokentext);
  return 0.0f;
}

LPCSTR Parser::ExpectString() {
  LPCSTR     tokentext;
  UTokenData value;
  if (Token(&tokentext, &value) == 0x102) {
    return value.sVal;
  }
  FatalExpected(MDL::TokenText(0x102), tokentext);
  return 0;
}

LPCSTR Parser::ExpectString(UINT cachedToken, LPCSTR tokenText, UTokenData *cachedValue) {
  if (cachedToken == 0x102) {
    return cachedValue->sVal;
  }
  FatalExpected(MDL::TokenText(0x102), tokenText);
  return 0;
}

long Parser::GetOptionalInt(UINT *token, LPCSTR *tokenText, UTokenData *savedValue) {
  UTokenData value;
  *token = Token(tokenText, &value);
  if (*token != 0x100) {
    return -1;
  }
  long result = value.lVal;
  *token = Token(tokenText, savedValue);
  return result;
}

long Parser::GetOptionalInt(UINT cachedToken, UTokenData *cachedValue, UINT *token, LPCSTR *tokenText) {
  if (cachedToken == 0x100) {
    *token = Token(tokenText, 0);
    return cachedValue->lVal;
  }
  *token = cachedToken;
  return -1;
}

int Parser::GetOptionalToken(UINT expected, UINT *token, LPCSTR *tokenText) {
  *token = Token(tokenText, 0);
  if (*token != expected) {
    return 0;
  }
  *token = Token(tokenText, 0);
  return 1;
}

int Parser::GetOptionalToken(UINT expected, UINT cachedToken, UINT *token, LPCSTR *tokenText) {
  if (cachedToken == expected) {
    *token = Token(tokenText, 0);
    return 1;
  }
  *token = cachedToken;
  return 0;
}
