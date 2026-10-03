#include <storm.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PRINTF_DEFAULT_LIMIT 0x100000

enum ArgumentSize {
  e_intSized = 0,
  e_pointerSized = 1,
  e_longLongSized = 2,
  e_doubleSized = 3,
  e_takesNoSpace = 4
};

struct SpecifierRange {
  LPCSTR begin;
  int    length;
  int    argument;
};

union ArgumentType {
  int      asInt;
  LPVOID   asPointer;
  LONGLONG asLongLong;
  double   asDouble;
};

static int ParseFormatSpecifier(LPCSTR *specifierPtr, ArgumentSize *size, int *orderingPtr) {
  LPCSTR specifier = *specifierPtr;
  int    currentNumber = 0;
  *size = e_intSized;

  for (;;) {
    char ch = *specifier++;
    if (isdigit(ch)) {
      currentNumber = currentNumber * 10 + ch - '0';
    } else if (ch == '$') {
      *orderingPtr = currentNumber ? currentNumber - 1 : 0;
    } else {
      currentNumber = 0;
      switch (ch) {
        case 'l':
          if (*specifier == 'l') {
            specifier++;
            *size = e_longLongSized;
          }
          break;
        case 'I':
          if (specifier[0] == '6' && specifier[1] == '4') {
            specifier += 2;
            *size = e_longLongSized;
          }
          break;
        case 'C':
        case 'D':
        case 'U':
        case 'X':
        case 'c':
        case 'd':
        case 'i':
        case 'u':
        case 'x':
          *specifierPtr = specifier;
          return TRUE;
        case 'P':
        case 'S':
        case 'p':
        case 's':
          *size = e_pointerSized;
          *specifierPtr = specifier;
          return TRUE;
        case 'E':
        case 'F':
        case 'G':
        case 'e':
        case 'f':
        case 'g':
          *size = e_doubleSized;
          *specifierPtr = specifier;
          return TRUE;
        case '%':
        case '\0':
          *size = e_takesNoSpace;
          *specifierPtr = specifier;
          return FALSE;
      }
    }
  }
}

static void RemoveOrderingFromFormatSpecifier(char *specifier) {
  char *dollarSign;
  char *firstDigit;

  dollarSign = strchr(specifier, '$');
  if (!dollarSign) {
    return;
  }

  firstDigit = dollarSign;
  while (isdigit(firstDigit[-1])) {
    firstDigit--;
  }

  memmove(firstDigit, dollarSign + 1, strlen(dollarSign));
}

static void FixUpLongLongFormatSpecifier(char *specifier) {
  specifier = strstr(specifier, "ll");
  if (specifier) {
    memmove(specifier + 3, specifier + 2, strlen(specifier) - 1);
    memcpy(specifier, "I64", 3);
  }
}

int __cdecl vsnoprintf(char *out, int outSize, LPCSTR format, char *argumentList) {
  LPCSTR         end = out + outSize - 1;
  ArgumentSize   argumentSizeList[256] = {e_intSized};
  ArgumentType   orderedArgumentList[256] = {0};
  SpecifierRange specifierRange[256] = {0};
  LPCSTR const   start = out;
  int            argumentIndex = 0;
  int            argumentCount = 0;
  LPCSTR         formatAt = format;
  int            specifierCount = 0;

  for (;;) {
    char ch = *formatAt++;
    if (ch == '%') {
      ArgumentSize argumentSize;
      LPCSTR       begin = formatAt - 1;
      int          hasArgument = ParseFormatSpecifier(&formatAt, &argumentSize, &argumentIndex);
      specifierRange[specifierCount].length = formatAt - begin;
      specifierRange[specifierCount].begin = begin;
      specifierRange[specifierCount].argument = argumentIndex;
      specifierCount++;
      if (hasArgument > 0) {
        argumentSizeList[argumentIndex] = argumentSize;
        ++argumentIndex;
        if (argumentCount < argumentIndex) {
          argumentCount = argumentIndex;
        }
      }
    } else if (!ch) {
      break;
    }
  }

  for (argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
    switch (argumentSizeList[argumentIndex]) {
      case e_intSized:
      case e_pointerSized:
        orderedArgumentList[argumentIndex].asInt = va_arg(argumentList, int);
        break;
      case e_longLongSized:
        orderedArgumentList[argumentIndex].asLongLong = va_arg(argumentList, LONGLONG);
        break;
      case e_doubleSized:
        orderedArgumentList[argumentIndex].asDouble = va_arg(argumentList, double);
        break;
      default:
        orderedArgumentList[argumentIndex].asLongLong = 0;
        break;
    }
  }

  specifierCount = 0;
  while (out < end) {
    char ch = *format++;
    if (!ch) {
      break;
    }
    if (ch != '%') {
      *out++ = ch;
    } else {
      SpecifierRange &range = specifierRange[specifierCount++];
      char            individualFormatSpecifier[256];
      memcpy(individualFormatSpecifier, range.begin, range.length);
      individualFormatSpecifier[range.length] = 0;
      RemoveOrderingFromFormatSpecifier(individualFormatSpecifier);
      argumentIndex = range.argument;
      if (argumentSizeList[argumentIndex] == e_longLongSized) {
        FixUpLongLongFormatSpecifier(individualFormatSpecifier);
      }

      int written = 0;
      switch (argumentSizeList[argumentIndex]) {
        case e_intSized:
        case e_pointerSized:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier, orderedArgumentList[argumentIndex].asInt);
          break;
        case e_longLongSized:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier, orderedArgumentList[argumentIndex].asLongLong);
          break;
        case e_doubleSized:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier, orderedArgumentList[argumentIndex].asDouble);
          break;
        case e_takesNoSpace:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier);
          break;
      }

      out += written;
      format += range.length - 1;
    }
  }

  *out = 0;
  return out - start;
}

int __cdecl vsoprintf(char *out, LPCSTR format, char *argumentList) {
  return vsnoprintf(out, PRINTF_DEFAULT_LIMIT, format, argumentList);
}

int __cdecl snoprintf(char *out, int outSize, LPCSTR format, ...) {
  int     result;
  va_list arglist;

  va_start(arglist, format);
  result = vsnoprintf(out, outSize, format, (char *)arglist);
  va_end(arglist);
  return result;
}

int __cdecl soprintf(char *out, LPCSTR format, ...) {
  int     result;
  va_list arglist;

  va_start(arglist, format);
  result = vsnoprintf(out, PRINTF_DEFAULT_LIMIT, format, (char *)arglist);
  va_end(arglist);
  return result;
}
