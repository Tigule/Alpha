#include "Parser.h"
#include "MDLStatus.h"
#include "lex.h"

namespace MDL {
  LPCSTR TokenText(UINT token);
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
    if (scanToken) {
      if (scanToken == -2) {
        token = 0;
        m_flags |= 3;
      }
    } else {
      FatalEOF();
      m_flags |= 3;
    }
  }

  if (tokenText) {
    *tokenText = token == MDLTOK_UNKNOWN ? m_scanner.mdltext : MDL::TokenText(token);
  }
  if (data) {
    data->lVal = m_scanner.tokendata.lVal;
  }
  return token;
}

void Parser::FatalDuplicate(LPCSTR found) {
  m_status->Add(STATUS_FATAL, "Error (line %d): Found duplicate \"%s\"\n", GetLineNumber(), found);
  m_flags |= 1;
}

void Parser::FatalUnmatched(LPCSTR item1, UINT count1, LPCSTR item2, UINT count2) {
  m_status->Add(
      STATUS_FATAL, "Error (line %d): found %d \"%s\", but %d \"%s\", counts must match\n", GetLineNumber(), count1, item1, count2, item2
  );
  m_flags |= 1;
}

void Parser::FatalNotFound(LPCSTR expected) {
  m_status->Add(STATUS_FATAL, "Error (line %d): Expected \"%s\"\n", GetLineNumber(), expected);
  m_flags |= 1;
}

void Parser::FatalNotFound(UINT what) {
  FatalNotFound(MDL::TokenText(what));
}

void Parser::FatalUnexpected(LPCSTR found) {
  m_status->Add(STATUS_FATAL, "Error (line %d): Unexpected token \"%s\"\n", GetLineNumber(), found);
  m_flags |= 1;
}

void Parser::FatalExpected(LPCSTR expected, LPCSTR found) {
  m_status->Add(STATUS_FATAL, "Error (line %d): Expected \"%s\", but found \"%s\"\n", GetLineNumber(), expected, found);
  m_flags |= 1;
}

void Parser::FatalExpected(UINT what, LPCSTR found) {
  FatalExpected(MDL::TokenText(what), found);
}

void Parser::FatalEOF() {
  m_status->Add(STATUS_FATAL, "Error (line %d): Unexpected end of file\n", GetLineNumber());
  m_flags |= 1;
}

void Parser::WarningCount(LPCSTR item, long expected, long actual) {
  m_status->Add(STATUS_WARNING, "Warning (line %d): Expected %d \"%s\", but found %d\n", GetLineNumber(), expected, item, actual);
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
  if (cachedToken == MDLTOK_LONG) {
    return cachedValue->lVal;
  }
  FatalExpected(MDL::TokenText(MDLTOK_LONG), tokenText);
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
  if (token == MDLTOK_FLOAT) {
    return value.fVal;
  }
  if (token == MDLTOK_LONG) {
    return value.lVal;
  }
  FatalExpected(MDL::TokenText(MDLTOK_FLOAT), tokentext);
  return 0.0f;
}

LPCSTR Parser::ExpectString() {
  LPCSTR     tokentext;
  UTokenData value;
  if (Token(&tokentext, &value) == MDLTOK_STRING) {
    return value.sVal;
  }
  FatalExpected(MDL::TokenText(MDLTOK_STRING), tokentext);
  return 0;
}

LPCSTR Parser::ExpectString(UINT cachedToken, LPCSTR tokenText, UTokenData *cachedValue) {
  if (cachedToken == MDLTOK_STRING) {
    return cachedValue->sVal;
  }
  FatalExpected(MDL::TokenText(MDLTOK_STRING), tokenText);
  return 0;
}

long Parser::GetOptionalInt(UINT *token, LPCSTR *tokenText, UTokenData *savedValue) {
  UTokenData value;
  *token = Token(tokenText, &value);
  if (*token != MDLTOK_LONG) {
    return -1;
  }
  long result = value.lVal;
  *token = Token(tokenText, savedValue);
  return result;
}

long Parser::GetOptionalInt(UINT cachedToken, UTokenData *cachedValue, UINT *token, LPCSTR *tokenText) {
  if (cachedToken != MDLTOK_LONG) {
    *token = cachedToken;
    return -1;
  }
  *token = Token(tokenText, 0);
  return cachedValue->lVal;
}

BOOL Parser::GetOptionalToken(UINT expected, UINT *token, LPCSTR *tokenText) {
  *token = Token(tokenText, 0);
  if (*token != expected) {
    return 0;
  }
  *token = Token(tokenText, 0);
  return 1;
}

BOOL Parser::GetOptionalToken(UINT expected, UINT cachedToken, UINT *token, LPCSTR *tokenText) {
  if (cachedToken != expected) {
    *token = cachedToken;
    return 0;
  }
  *token = Token(tokenText, 0);
  return 1;
}
