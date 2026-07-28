#include <storm.h>

#include <ctype.h>
#include <stdarg.h>

typedef union _SSTR_PACKED {
  DWORD dword;
  BYTE  byte[4];
} SSTR_PACKED, *SSTR_PACKEDPTR;

static const DWORD s_hashtable[16] = {
    0x486E26EE, 0xDCAA16B3, 0xE1918EEF, 0x202DAFDB, 0x341C7DC7, 0x1C365303, 0x40EF2D37, 0x65FD5E49,
    0xD6057177, 0x904ECE93, 0x1C38024F, 0x98FD323B, 0xE3061AE7, 0xA39B0FA1, 0x9797F25F, 0xE4444563,
};

static const __int64 s_hashtable64[16] = {
    0x486E26EEDCAA16B3, 0xE1918EEF202DAFDB, 0x341C7DC71C365303, 0x40EF2D3765FD5E49, 0xD6057177904ECE93, 0x1C38024F98FD323B,
    0xE3061AE7A39B0FA1, 0x9797F25FE4444563, 0xCD2EC20C8DC1B898, 0x31759633799A306D, 0x8C2063852E6E9627, 0x79237D9973922C66,
    0x8728628D28628824, 0x8F1F7E9625887795, 0x296E3281389C0D60, 0x6F4893CA61636542,
};

/*
 * These are the original Storm dword string primitives.  The current client
 * retains this implementation (including the deliberately-used result
 * scratch) after its argument validation prologues.
 */
static DWORD  s_check_markresultused;
static double s_realDigit[20][10];
static int    s_initialized;

#define SSTR_INIT_DWORD_OPERATIONS \
  DWORD sstrCheckModNum;           \
  DWORD sstrCheckNotNum = 0xFFFFFFFF

#define SSTR_CHECK_FOR_NULL_BYTES(num)                                                                                           \
  (sstrCheckModNum = (num) + 0x7EFEFEFF, sstrCheckNotNum -= (num), sstrCheckModNum ^= sstrCheckNotNum, sstrCheckNotNum |= (num), \
   sstrCheckModNum &= 0x81010101)

#define SSTR_CHECK_END_LOOP s_check_markresultused = sstrCheckModNum

#define SSTR_CHECK_NULL_BYTE0(packed) (!(packed).byte[0])
#define SSTR_CHECK_NULL_BYTE1(packed) (!(packed).byte[1])
#define SSTR_CHECK_NULL_BYTE2(packed) (!((packed).dword & 0x00FF0000))
#define SSTR_CHECK_NULL_BYTE3(packed) (!((packed).dword & 0xFF000000))

#define SSTR_SKIP_LEADING_BYTES           \
  for (; (DWORD)currdest & 3; ++currdest) \
    if (!*currdest)                       \
  goto sstrEndSkip

#define SSTR_SKIP_ALIGNED_DWORDS                    \
  do {                                              \
    SSTR_PACKED packed = *(SSTR_PACKEDPTR)currdest; \
    currdest += sizeof(SSTR_PACKED);                \
    if (!SSTR_CHECK_FOR_NULL_BYTES(packed.dword))   \
      continue;                                     \
    if (SSTR_CHECK_NULL_BYTE0(packed)) {            \
      currdest -= 4;                                \
      goto sstrEndSkip;                             \
    }                                               \
    if (SSTR_CHECK_NULL_BYTE1(packed)) {            \
      currdest -= 3;                                \
      goto sstrEndSkip;                             \
    }                                               \
    if (SSTR_CHECK_NULL_BYTE2(packed)) {            \
      currdest -= 2;                                \
      goto sstrEndSkip;                             \
    }                                               \
    if (SSTR_CHECK_NULL_BYTE3(packed)) {            \
      currdest -= 1;                                \
      goto sstrEndSkip;                             \
    }                                               \
    SSTR_CHECK_END_LOOP;                            \
  } while (1)

#define SSTR_BEGIN_COPY DWORD sstrNegOffset = (DWORD) - (int)(destsize - (currdest - dest))

#define SSTR_COPY_LEADING_BYTES                      \
  while (((DWORD)source & 3) && sstrNegOffset)       \
    if (!(*(enddest + sstrNegOffset++) = *source++)) \
  goto sstrEndCopy

#define SSTR_COPY_ALIGNED_DWORDS                                 \
  if ((int)(sstrNegOffset += 3) < 0) {                           \
    enddest -= 3;                                                \
    SSTR_PACKED packed = *(SSTR_PACKEDPTR)source;                \
    source += sizeof(SSTR_PACKED);                               \
    while (!SSTR_CHECK_FOR_NULL_BYTES(packed.dword)) {           \
      *(SSTR_PACKEDPTR)(enddest + sstrNegOffset) = packed;       \
      if ((int)(sstrNegOffset += 4) >= 0)                        \
        goto sstrDoneAligned;                                    \
      packed = *(SSTR_PACKEDPTR)source;                          \
      source += sizeof(SSTR_PACKED);                             \
    }                                                            \
    for (;;) {                                                   \
      if (SSTR_CHECK_NULL_BYTE0(packed)) {                       \
        *(enddest + sstrNegOffset) = packed.byte[0];             \
        sstrNegOffset += 1;                                      \
        goto sstrEndCopy;                                        \
      }                                                          \
      if (SSTR_CHECK_NULL_BYTE1(packed)) {                       \
        *(WORD *)(enddest + sstrNegOffset) = (WORD)packed.dword; \
        sstrNegOffset += 2;                                      \
        goto sstrEndCopy;                                        \
      }                                                          \
      if (SSTR_CHECK_NULL_BYTE2(packed)) {                       \
        *(WORD *)(enddest + sstrNegOffset) = (WORD)packed.dword; \
        *(enddest + sstrNegOffset + 2) = 0;                      \
        sstrNegOffset += 3;                                      \
        goto sstrEndCopy;                                        \
      }                                                          \
      *(SSTR_PACKEDPTR)(enddest + sstrNegOffset) = packed;       \
      sstrNegOffset += sizeof(SSTR_PACKED);                      \
      if (SSTR_CHECK_NULL_BYTE3(packed))                         \
        goto sstrEndCopy;                                        \
      SSTR_CHECK_END_LOOP;                                       \
      if ((int)sstrNegOffset >= 0)                               \
        goto sstrDoneAligned;                                    \
      packed = *(SSTR_PACKEDPTR)source;                          \
      source += sizeof(SSTR_PACKED);                             \
    }                                                            \
  sstrDoneAligned:                                               \
    enddest += 3;                                                \
  }                                                              \
  sstrNegOffset -= 3

#define SSTR_COPY_TRAILING_BYTES                     \
  while (sstrNegOffset)                              \
    if (!(*(enddest + sstrNegOffset++) = *source++)) \
      goto sstrEndCopy;                              \
  *enddest = 0

static void CheckInitialized() {
  if (!s_initialized) {
    SStrInitialize();
  }
}

static void InitializeFloatDigits() {
  double *out;
  double  scale;
  int     exponent;
  int     digit;

  out = &s_realDigit[0][0];
  exponent = -1;
  do {
    digit = 0;
    scale = pow(10.0, exponent);
    do {
      out[digit] = digit * scale;
      digit++;
    } while (digit < 10);
    out += 10;
    exponent--;
  } while (exponent > -21);
}

static inline double SStrParseIntegerDouble(LPCSTR *string) {
  LPCSTR source;
  LPCSTR chunkStart;
  double result;
  DWORD  chunk;
  DWORD  digit;

  source = *string;
  result = 0.0;
  chunk = *source - '0';
  chunkStart = source;
  if (chunk < 10) {
    digit = source[1] - '0';
    source++;
    while (digit < 10) {
      chunk = chunk * 10 + digit;
      source++;
      if (chunk >= 0x19999999) {
        result = result * pow(10.0, source - chunkStart) + chunk;
        chunk = 0;
        chunkStart = source;
      }
      digit = *source - '0';
    }
  } else {
    chunk = 0;
  }

  if (result != 0.0) {
    result = result * pow(10.0, source - chunkStart) + chunk;
  } else {
    result = chunk;
  }

  *string = source;
  return result;
}

static inline double SStrParseDecimalDouble(LPCSTR string) {
  const char *scan;
  double      result;
  DWORD       digit;
  int         exponent;
  int         tableOffset;
  int         negative;

  CheckInitialized();
  scan = string;
  negative = FALSE;

  if (*scan == '-') {
    negative = TRUE;
    scan++;
  }

  result = SStrParseIntegerDouble(&scan);

  if (*scan == '.' && (DWORD)(scan[1] - '0') < 10) {
    scan++;
    tableOffset = 0;
    exponent = -1;
    digit = *scan - '0';
    do {
      scan++;
      if (exponent > -21) {
        result += (&s_realDigit[0][0])[tableOffset + digit];
      } else {
        result += digit * pow(10.0, exponent);
      }
      exponent--;
      tableOffset += 10;
      digit = *scan - '0';
    } while (digit < 10);
  }

  if (*scan == 'e' || *scan == 'E') {
    scan++;
    if (*scan == '+') {
      scan++;
    }
    result *= pow(10.0, SStrToInt(scan));
  }

  return negative ? -result : result;
}

const char *SStrChr(const char *string, char ch) {
  char current;

  FATALASSERT(string);

  current = *string;
  while (current) {
    if (current == ch) {
      return string;
    }
    current = string[1];
    string++;
  }

  return NULL;
}

char *SStrChr(char *string, char ch) {
  char current;

  FATALASSERT(string);

  current = *string;
  while (current) {
    if (current == ch) {
      return string;
    }
    current = string[1];
    string++;
  }

  return NULL;
}

const char *SStrChrR(const char *string, char ch) {
  const char *result;

  FATALASSERT(string);

  result = NULL;
  while (*string) {
    if (*string == ch) {
      result = string;
    }
    string++;
  }

  return result;
}

char *SStrChrR(char *string, char ch) {
  char *result;

  FATALASSERT(string);

  result = NULL;
  while (*string) {
    if (*string == ch) {
      result = string;
    }
    string++;
  }

  return result;
}

int APIENTRY SStrCmp(LPCSTR string1, LPCSTR string2, DWORD maxchars) {
  FATALASSERT(string1);
  FATALASSERT(string2);

  return strncmp(string1, string2, maxchars);
}

int APIENTRY SStrCmpI(LPCSTR string1, LPCSTR string2, DWORD maxchars) {
  FATALASSERT(string1);
  FATALASSERT(string2);

  return _strnicmp(string1, string2, maxchars);
}

DWORD APIENTRY SStrCopy(char *dest, LPCSTR source, DWORD destsize) {
  FATALASSERT(dest);
  FATALASSERT(source);

  if (!destsize) {
    return 0;
  }

  --destsize;
  {
    SSTR_INIT_DWORD_OPERATIONS;
    char *currdest = dest;
    char *enddest = dest + destsize;

    SSTR_BEGIN_COPY;
    SSTR_COPY_LEADING_BYTES;
    SSTR_COPY_ALIGNED_DWORDS;
    SSTR_COPY_TRAILING_BYTES;
  sstrEndCopy:
    currdest = enddest + (sstrNegOffset - 1);
    return (DWORD)(currdest - dest);
  }
}

char *APIENTRY SStrDupA(LPCSTR string, LPCSTR fileName, unsigned int lineNumber) {
  DWORD bytes;
  char *result;

  FATALASSERT(string);

  bytes = SStrLen(string) + 1;
  result = (char *)SMemAlloc(bytes, fileName, lineNumber, 0);
  memcpy(result, string, bytes);

  return result;
}

extern "C" void APIENTRY SStrInitialize() {
  InitializeFloatDigits();
  s_initialized = TRUE;
}

extern "C" void APIENTRY SStrDestroy() {
}

DWORD APIENTRY SStrLen(LPCSTR string) {
  FATALASSERT(string);

  {
    SSTR_INIT_DWORD_OPERATIONS;
    LPCSTR currdest = string;

    SSTR_SKIP_LEADING_BYTES;
    SSTR_SKIP_ALIGNED_DWORDS;
  sstrEndSkip:
    return (DWORD)(currdest - string);
  }
}

DWORD APIENTRY SStrLen(const unsigned short *string) {
  const unsigned short *scan;

  FATALASSERT(string);

  scan = string;
  if (*scan) {
    do {
      scan++;
    } while (*scan);
  }

  return (DWORD)(scan - string);
}

DWORD APIENTRY SStrPack(char *dest, LPCSTR source, DWORD destsize) {
  FATALASSERT(dest);
  FATALASSERT(source);

  if (!destsize) {
    return 0;
  }

  --destsize;
  {
    SSTR_INIT_DWORD_OPERATIONS;
    char *currdest = dest;
    char *enddest = dest + destsize;

    if (destsize != 0x7FFFFFFE) {
      *enddest = 0;
    }

    SSTR_SKIP_LEADING_BYTES;
    SSTR_SKIP_ALIGNED_DWORDS;
  sstrEndSkip:
    SSTR_BEGIN_COPY;
    SSTR_COPY_LEADING_BYTES;
    SSTR_COPY_ALIGNED_DWORDS;
    SSTR_COPY_TRAILING_BYTES;
  sstrEndCopy:
    currdest = enddest + (sstrNegOffset - 1);
    return (DWORD)(currdest - dest);
  }
}

static int ISStrVPrintf(char *dest, unsigned int maxchars, LPCSTR format, char *arglist) {
  int written;

  if (!maxchars) {
    return 0;
  }

  if (maxchars == 0x7FFFFFFF) {
    if (g_opt.orderedprintfenabled) {
      return vsoprintf(dest, format, arglist);
    }
    return vsprintf(dest, format, (va_list)arglist);
  }

  if (g_opt.orderedprintfenabled) {
    written = vsnoprintf(dest, (int)maxchars, format, arglist);
  } else {
    written = _vsnprintf(dest, maxchars, format, (va_list)arglist);
  }
  if ((unsigned int)written >= maxchars) {
    dest[maxchars - 1] = 0;
    return maxchars - 1;
  }

  return written;
}

DWORD __cdecl SStrPrintf(char *dest, DWORD maxchars, LPCSTR format, ...) {
  va_list args;

  va_start(args, format);
  FATALASSERT(dest);
  FATALASSERT(format);

  return ISStrVPrintf(dest, maxchars, format, (char *)args);
}

DWORD __cdecl SStrVPrintf(char *dest, DWORD maxchars, LPCSTR format, char *arglist) {
  FATALASSERT(dest);
  FATALASSERT(format);

  return (DWORD)ISStrVPrintf(dest, maxchars, format, arglist);
}

double APIENTRY SStrToDouble(LPCSTR string) {
  FATALASSERT(string);

  return SStrParseDecimalDouble(string);
}

float APIENTRY SStrToFloat(LPCSTR string) {
  FATALASSERT(string);

  return (float)SStrParseDecimalDouble(string);
}

static inline int SStrParseInt(LPCSTR string) {
  unsigned int result;
  unsigned int digit;
  int          negative;

  negative = *string == '-';
  if (negative) {
    string++;
  }

  result = *string - '0';
  if (result < 10) {
    digit = string[1] - '0';
    string++;
    while (digit < 10) {
      string++;
      result = digit + 10 * result;
      digit = *string - '0';
    }
  } else {
    result = 0;
  }

  return negative ? -(int)result : (int)result;
}

int APIENTRY SStrToInt(LPCSTR string) {
  FATALASSERT(string);

  return SStrParseInt(string);
}

static inline unsigned __int64 SStrParseUnsigned64(LPCSTR *string) {
  LPCSTR           source;
  LPCSTR           chunkStart;
  unsigned __int64 result;
  DWORD            chunk;
  DWORD            digit;
  DWORD            multiplier;

  source = *string;
  result = 0;
  chunk = *source - '0';
  chunkStart = source;
  if (chunk < 10) {
    digit = source[1] - '0';
    source++;
    while (digit < 10) {
      chunk = chunk * 10 + digit;
      source++;
      if (chunk >= 0x19999999) {
        multiplier = (DWORD)(pow(10.0, source - chunkStart) + 0.5);
        result = result * multiplier + chunk;
        chunk = 0;
        chunkStart = source;
      }
      digit = *source - '0';
    }
  } else {
    chunk = 0;
  }

  if (result != 0) {
    multiplier = (DWORD)(pow(10.0, source - chunkStart) + 0.5);
    result = result * multiplier + chunk;
  } else {
    result = chunk;
  }

  *string = source;
  return result;
}

__int64 APIENTRY SStrToInt64(LPCSTR string) {
  unsigned __int64 result;
  int              negative;

  FATALASSERT(string);

  result = 0;
  negative = FALSE;

  if (*string == '-') {
    negative = TRUE;
    string++;
  }

  result = SStrParseUnsigned64(&string);

  return (__int64)(negative ? (0 - result) : result);
}

unsigned int APIENTRY SStrToUnsigned(LPCSTR string) {
  unsigned int result;
  unsigned int digit;

  FATALASSERT(string);

  result = *string - '0';
  if (result >= 10) {
    return 0;
  }

  digit = string[1] - '0';
  string++;
  while (digit < 10) {
    string++;
    result = digit + 10 * result;
    digit = *string - '0';
  }

  return result;
}

void APIENTRY SStrTokenize(LPCSTR *string, char *buffer, DWORD bufferchars, LPCSTR whitespace, int *quoted) {
  LPCSTR source;
  int    usedquotes;
  DWORD  destchars;
  int    inquotes;
  int    quoteEnabled;

  FATALASSERT(string);
  if (!string) {
    return;
  }
  FATALASSERT(*string);
  if (!*string) {
    return;
  }
  FATALASSERT(buffer || !bufferchars);
  FATALASSERT(whitespace);

  quoteEnabled = SStrChr(whitespace, '"') != NULL;
  source = *string;
  inquotes = FALSE;
  usedquotes = FALSE;

  while (*source && SStrChr(whitespace, *source)) {
    if (quoteEnabled && *source == '"') {
      inquotes = TRUE;
      usedquotes = TRUE;
      source++;
      break;
    }
    source++;
  }

  destchars = 0;
  while (*source) {
    char ch = *source;

    if (quoteEnabled && ch == '"') {
      if (destchars && !inquotes) {
        break;
      }
      inquotes = !inquotes;
      usedquotes = TRUE;
      source++;
      if (!inquotes) {
        break;
      }
      continue;
    }

    if (!inquotes && SStrChr(whitespace, ch)) {
      source++;
      break;
    }

    if (buffer && destchars + 1 < bufferchars) {
      buffer[destchars++] = ch;
    }
    source++;
  }

  if (buffer && destchars < bufferchars) {
    buffer[destchars] = 0;
  }
  *string = source;
  if (quoted) {
    *quoted = usedquotes;
  }
}

DWORD APIENTRY SStrHash(LPCSTR string, DWORD flags, DWORD seed) {
  DWORD hash;
  DWORD offset;
  DWORD ch;

  FATALASSERT(string);

  hash = seed;
  if (!hash) {
    hash = 0x7FED7FED;
  }

  offset = 0xEEEEEEEE;
  ch = (BYTE)*string;
  if (flags & 0x1) {
    while (ch) {
      hash += offset;
      offset *= 0x21;
      hash ^= s_hashtable[ch >> 4] - s_hashtable[ch & 0xF];
      offset += ch;
      offset += hash + 3;
      ch = (BYTE) * ++string;
    }
  } else {
    while (ch) {
      ++string;
      if (ch >= 'a' && ch <= 'z') {
        ch -= 'a' - 'A';
      }

      if (ch == '/') {
        ch = '\\';
      }
      hash += offset;
      offset *= 0x21;
      hash ^= s_hashtable[ch >> 4] - s_hashtable[ch & 0xF];
      offset += ch;
      offset += hash + 3;
      ch = (BYTE)*string;
    }
  }

  if (!hash) {
    hash = 1;
  }

  return hash;
}

__int64 APIENTRY SStrHash64(LPCSTR string, DWORD flags, __int64 seed) {
  __int64 result;
  DWORD   ch;
  __int64 adjust;

  FATALASSERT(string);

  result = seed;
  if (!result) {
    result = 0x7FED7FED7FED7FED;
  }
  adjust = 0xEEEEEEEEEEEEEEEE;
  ch = (BYTE)*string;
  if (flags & 0x1) {
    while (ch) {
      result += adjust;
      adjust *= 0x21;
      result ^= s_hashtable64[ch >> 4] + s_hashtable64[ch & 0xF];
      adjust += ch;
      adjust += result + 3;
      ch = (BYTE) * ++string;
    }
  } else {
    while (ch) {
      ++string;
      if (ch >= 'a' && ch <= 'z') {
        ch -= 'a' - 'A';
      }

      if (ch == '/') {
        ch = '\\';
      }
      result += adjust;
      adjust *= 0x21;
      result ^= s_hashtable64[ch >> 4] + s_hashtable64[ch & 0xF];
      adjust += ch;
      adjust += result + 3;
      ch = (BYTE)*string;
    }
  }

  if (!result) {
    result = 1;
  }

  return result;
}

static DWORD bjhash(unsigned char *k, DWORD length, DWORD initval) {
  DWORD a = 0x9E3779B9;
  DWORD b = 0x9E3779B9;
  DWORD c = initval;
  DWORD len = length;

#define BJ_MIX(a_, b_, c_) \
  do {                     \
    a_ -= b_;              \
    a_ -= c_;              \
    a_ ^= (c_ >> 13);      \
    b_ -= c_;              \
    b_ -= a_;              \
    b_ ^= (a_ << 8);       \
    c_ -= a_;              \
    c_ -= b_;              \
    c_ ^= (b_ >> 13);      \
    a_ -= b_;              \
    a_ -= c_;              \
    a_ ^= (c_ >> 12);      \
    b_ -= c_;              \
    b_ -= a_;              \
    b_ ^= (a_ << 16);      \
    c_ -= a_;              \
    c_ -= b_;              \
    c_ ^= (b_ >> 5);       \
    a_ -= b_;              \
    a_ -= c_;              \
    a_ ^= (c_ >> 3);       \
    b_ -= c_;              \
    b_ -= a_;              \
    b_ ^= (a_ << 10);      \
    c_ -= a_;              \
    c_ -= b_;              \
    c_ ^= (b_ >> 15);      \
  } while (0)

  while (len >= 12) {
    a += k[0] + ((DWORD)k[1] << 8) + ((DWORD)k[2] << 16) + ((DWORD)k[3] << 24);
    b += k[4] + ((DWORD)k[5] << 8) + ((DWORD)k[6] << 16) + ((DWORD)k[7] << 24);
    c += k[8] + ((DWORD)k[9] << 8) + ((DWORD)k[10] << 16) + ((DWORD)k[11] << 24);
    BJ_MIX(a, b, c);
    k += 12;
    len -= 12;
  }

  c += length;
  switch (len) {
    case 11:
      c += (DWORD)k[10] << 24;
    case 10:
      c += (DWORD)k[9] << 16;
    case 9:
      c += (DWORD)k[8] << 8;
    case 8:
      b += (DWORD)k[7] << 24;
    case 7:
      b += (DWORD)k[6] << 16;
    case 6:
      b += (DWORD)k[5] << 8;
    case 5:
      b += k[4];
    case 4:
      a += (DWORD)k[3] << 24;
    case 3:
      a += (DWORD)k[2] << 16;
    case 2:
      a += (DWORD)k[1] << 8;
    case 1:
      a += k[0];
  }

  BJ_MIX(a, b, c);
#undef BJ_MIX
  return c;
}

DWORD APIENTRY SStrHashHT(LPCSTR string) {
  char  buf[0x400];
  char *out = buf;
  DWORD ch = (BYTE)*string;
  DWORD used = 0;
  while (ch) {
    string++;
    if (used >= sizeof(buf) - 1) {
      break;
    }
    if (ch >= 'a' && ch <= 'z') {
      ch -= 'a' - 'A';
    } else if (ch == '/') {
      ch = '\\';
    }
    *out = (char)ch;
    ch = (BYTE)*string;
    used++;
    out++;
  }
  *out = 0;

  return bjhash((unsigned char *)buf, used, 0);
}

void APIENTRY SStrUpper(char *string) {
  _strupr(string);
}

void APIENTRY SStrLower(char *string) {
  _strlwr(string);
}

const char *SStrStr(const char *string, const char *search) {
  DWORD searchLen;

  FATALASSERT(string);
  FATALASSERT(search);

  searchLen = SStrLen(search);
  while (*string) {
    if (!SStrCmp(string, search, searchLen)) {
      return string;
    }
    string++;
  }

  return NULL;
}

char *SStrStr(char *string, const char *search) {
  DWORD searchLen;

  FATALASSERT(string);
  FATALASSERT(search);

  searchLen = SStrLen(search);
  while (*string) {
    if (!SStrCmp(string, search, searchLen)) {
      return string;
    }
    string++;
  }

  return NULL;
}

const char *SStrStrI(const char *string, const char *search) {
  DWORD searchLen;

  FATALASSERT(string);
  FATALASSERT(search);

  searchLen = SStrLen(search);
  while (*string) {
    if (!SStrCmpI(string, search, searchLen)) {
      return string;
    }
    string++;
  }

  return NULL;
}

char *SStrStrI(char *string, const char *search) {
  DWORD searchLen;

  FATALASSERT(string);
  FATALASSERT(search);

  searchLen = SStrLen(search);
  while (*string) {
    if (!SStrCmpI(string, search, searchLen)) {
      return string;
    }
    string++;
  }

  return NULL;
}

char *Int64ToString(__int64 num, char *buf, DWORD destsize) {
  char  nbuf[32];
  char *out;
  char *scan;
  int   thou;

  if (destsize > sizeof(nbuf)) {
    destsize = sizeof(nbuf);
  }

  scan = nbuf;
  thou = 0;

  if (num == 0) {
    *scan++ = '0';
  }

  while (num && scan < nbuf + destsize - 1) {
    *scan++ = (char)('0' + (int)(num % 10));
    num /= 10;
    if (++thou == 3) {
      if (num) {
        *scan++ = ',';
        thou = 0;
      }
    }
  }

  if (num) {
    memset(buf, '*', destsize - 1);
    buf[destsize - 1] = 0;
    return buf;
  }

  out = buf;
  while (scan >= nbuf) {
    *out++ = *--scan;
  }
  *out = 0;
  return buf;
}

namespace STypeCache {
  char **s_table;
  int    s_tableSize;
  int    s_tableSizeBits;
  char  *s_namesBase;
  char  *s_names;
  int    s_namesFree;
  int    s_numEntries;
  int    s_reprobeCount;
  char  *s_lastSearchValue;
  long   s_interlock;
  int    s_probe3Count;
  int    s_probe2Count;
  char  *s_lastSearchKey;
  int    s_probe1Count;
  int    s_stringBytes;
}  // namespace STypeCache

void STypeCache::Shutdown() {
  char  *block;
  char  *next;
  char **table;

  while (SInterlockedIncrement(&s_interlock) != 1) {
    SInterlockedDecrement(&s_interlock);
  }

  block = s_namesBase;
  while (block) {
    next = *(char **)block;
    SMemFree(block, __FILE__, __LINE__, 0);
    block = next;
  }

  table = s_table;
  s_namesBase = NULL;
  if (table) {
    SMemFree(table, __FILE__, __LINE__, 0);
  }

  s_table = NULL;
  SInterlockedDecrement(&s_interlock);
}

void STypeCache::Grow() {
  int   bits;
  int   i;
  char *block;
  char *key;
  char *oldKey;
  char *value;

  bits = s_tableSizeBits ? s_tableSizeBits : 9;
  bits++;
  s_tableSizeBits = bits;
  s_tableSize = 1 << bits;

  s_table = (char **)SMemReAlloc(s_table, (DWORD)(s_tableSize * sizeof(char *)), __FILE__, __LINE__, 0);
  memset(s_table, 0, (size_t)(s_tableSize * sizeof(char *)));

  block = s_namesBase;
  while (block) {
    key = block + sizeof(char *);
    if (*key) {
      do {
        oldKey = key;
        value = key + SStrLen(key) + 1;
        key = value + SStrLen(value) + 1;
        i = GetProbe(oldKey);
        s_table[i] = oldKey;
      } while (*key);
    }
    block = *(char **)block;
  }
}

int STypeCache::GetProbe(const char *rawname) {
  DWORD hash;
  int   probe;
  int   reprobe;

  hash = SStrHashHT(rawname);
  if (s_numEntries >= s_tableSize) {
    Grow();
  }

  s_probe1Count++;
  probe = (int)(hash & (DWORD)(s_tableSize - 1));
  if (!s_table[probe] || SStrCmp(rawname, s_table[probe], 0x7FFFFFFF) == 0) {
    return probe;
  }

  s_probe2Count++;
  probe = (int)((hash >> s_tableSizeBits) & (DWORD)(s_tableSize - 1));
  if (s_table[probe] && SStrCmp(rawname, s_table[probe], 0x7FFFFFFF) != 0) {
    s_probe3Count++;
    for (reprobe = 0; reprobe < 8; reprobe++) {
      s_reprobeCount++;
      probe = (probe + 1) & (s_tableSize - 1);
      if (!s_table[probe] || SStrCmp(rawname, s_table[probe], 0x7FFFFFFF) == 0) {
        return probe;
      }
    }

    return -1;
  }

  return probe;
}

const char *STypeCache::Get(const char *rawname) {
  int probe;

  if (!rawname) {
    SInterlockedDecrement(&s_interlock);
    return s_lastSearchValue;
  }

  while (SInterlockedIncrement(&s_interlock) != 1) {
    SInterlockedDecrement(&s_interlock);
  }
  probe = GetProbe(rawname);
  if (probe >= 0 && s_table && s_table[probe]) {
    s_lastSearchKey = s_table[probe];
    s_lastSearchValue = s_lastSearchKey + SStrLen(s_lastSearchKey) + 1;
    return s_lastSearchValue;
  }

  return NULL;
}

const char *STypeCache::Set(const char *rawname, const char *decname) {
  int   probe;
  int   keyBytes;
  int   valueBytes;
  char *slot;
  char *newBlock;

  probe = GetProbe(rawname);
  if (probe < 0) {
    SInterlockedDecrement(&s_interlock);
    return decname;
  }

  keyBytes = (int)SStrLen(rawname) + 1;
  valueBytes = (int)SStrLen(decname) + 1;
  s_stringBytes += keyBytes + valueBytes;

  if (!s_names || s_namesFree + keyBytes + valueBytes >= 0x2000) {
    newBlock = (char *)SMemAlloc(0x2000, __FILE__, __LINE__, 8);
    if (!s_names) {
      s_namesBase = newBlock;
    } else {
      *(char **)s_names = newBlock;
    }
    s_names = newBlock;
    *(char **)s_names = NULL;
    s_namesFree = sizeof(char *);
  }

  slot = s_names + s_namesFree;
  if (s_table) {
    s_table[probe] = slot;
  }

  SStrCopy(slot, rawname, 0xFF);
  slot += SStrLen(slot) + 1;
  SStrCopy(slot, decname, 0xFF);
  s_namesFree = (int)((slot + SStrLen(slot) + 1) - s_names);
  s_numEntries += 2;

  SInterlockedDecrement(&s_interlock);
  return decname;
}
