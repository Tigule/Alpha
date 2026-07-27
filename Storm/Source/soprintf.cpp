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
  const char *start;
  int         length;
  int         ordering;
};

union ArgumentType {
  unsigned __int64 integer;
  double           real;
};

static int ParseFormatSpecifier(const char **specifierPtr, ArgumentSize *size, int *orderingPtr) {
  const char *specifier;
  int         currentNumber;

  specifier = *specifierPtr;
  currentNumber = 0;
  *size = e_intSized;

  for (;;) {
    int ch;

    ch = *specifier++;
    if (isdigit(ch)) {
      currentNumber = currentNumber * 10 + ch - '0';
      continue;
    }

    if (ch == '$') {
      *orderingPtr = currentNumber ? currentNumber - 1 : 0;
      continue;
    }

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
      case 'E':
      case 'F':
      case 'G':
      case 'e':
      case 'f':
      case 'g':
        *size = e_doubleSized;
        *specifierPtr = specifier;
        return TRUE;
      case 'P':
      case 'S':
      case 'p':
      case 's':
        *size = e_pointerSized;
        *specifierPtr = specifier;
        return TRUE;
      case '%':
      case '\0':
        *size = e_takesNoSpace;
        *specifierPtr = specifier;
        return FALSE;
      default:
        break;
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
  if (!specifier) {
    return;
  }

  memmove(specifier + 3, specifier + 2, strlen(specifier) - 1);
  specifier[0] = 'I';
  specifier[1] = '6';
  specifier[2] = '4';
}

int __cdecl vsnoprintf(char *out, int outSize, const char *format, char *argumentList) {
  SpecifierRange    specifierRange[256];
  ArgumentSize      argumentSizeList[256];
  ArgumentType      orderedArgumentList[256];
  char              individualFormatSpecifier[256];
  ArgumentSize      argumentSize;
  const char *const start = out;
  int               argumentIndex;
  const char       *formatAt;
  const char       *end;
  int               argumentCount;
  SpecifierRange   *range;
  int               ordering;
  int               specifierCount;
  int               hasArgument;
  int               written;
  char              ch;

  memset(argumentSizeList, 0, sizeof(argumentSizeList));
  memset(orderedArgumentList, 0, sizeof(orderedArgumentList));
  memset(specifierRange, 0, sizeof(specifierRange));

  ordering = 0;
  argumentCount = 0;
  specifierCount = 0;
  formatAt = format;
  do {
    ch = *formatAt++;
    if (ch == '%') {
      range = &specifierRange[specifierCount++];
      range->start = formatAt - 1;
      hasArgument = ParseFormatSpecifier(&formatAt, &argumentSize, &ordering);
      range->length = formatAt - range->start;
      range->ordering = ordering;
      if (hasArgument > 0) {
        argumentSizeList[ordering] = argumentSize;
        ++ordering;
        if (argumentCount < ordering) {
          argumentCount = ordering;
        }
      }
    }
  } while (ch);

  for (argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
    switch (argumentSizeList[argumentIndex]) {
      case e_intSized:
      case e_pointerSized:
        orderedArgumentList[argumentIndex].integer = *(DWORD *)argumentList;
        argumentList += sizeof(DWORD);
        break;
      case e_longLongSized:
        orderedArgumentList[argumentIndex].integer = *(unsigned __int64 *)argumentList;
        argumentList += sizeof(unsigned __int64);
        break;
      case e_doubleSized:
        orderedArgumentList[argumentIndex].real = *(double *)argumentList;
        argumentList += sizeof(double);
        break;
      default:
        orderedArgumentList[argumentIndex].integer = 0;
        break;
    }
  }

  end = out + outSize - 1;
  range = specifierRange;
  while (out < end && (ch = *format++) != 0) {
    if (ch == '%') {
      memcpy(individualFormatSpecifier, range->start, range->length);
      individualFormatSpecifier[range->length] = 0;
      RemoveOrderingFromFormatSpecifier(individualFormatSpecifier);
      ordering = range->ordering;
      if (argumentSizeList[ordering] == e_longLongSized) {
        FixUpLongLongFormatSpecifier(individualFormatSpecifier);
      }

      written = 0;
      switch (argumentSizeList[ordering]) {
        case e_intSized:
        case e_pointerSized:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier, (DWORD)orderedArgumentList[ordering].integer);
          break;
        case e_longLongSized:
        case e_doubleSized:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier, orderedArgumentList[ordering].integer);
          break;
        case e_takesNoSpace:
          written = _snprintf(out, end - out + 1, individualFormatSpecifier);
          break;
      }

      out += written;
      format += range->length - 1;
      ++range;
    } else {
      *out++ = ch;
    }
  }

  *out = 0;
  return out - start;
}

int __cdecl vsoprintf(char *out, const char *format, char *argumentList) {
  return vsnoprintf(out, PRINTF_DEFAULT_LIMIT, format, argumentList);
}

int __cdecl snoprintf(char *out, int outSize, const char *format, ...) {
  int     result;
  va_list arglist;

  va_start(arglist, format);
  result = vsnoprintf(out, outSize, format, (char *)arglist);
  va_end(arglist);
  return result;
}

int __cdecl soprintf(char *out, const char *format, ...) {
  int     result;
  va_list arglist;

  va_start(arglist, format);
  result = vsnoprintf(out, PRINTF_DEFAULT_LIMIT, format, (char *)arglist);
  va_end(arglist);
  return result;
}
