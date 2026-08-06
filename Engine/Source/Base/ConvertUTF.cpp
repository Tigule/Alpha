#include <Base/Base.h>

#include "ConvertUTF.h"

typedef DWORD UCS4;

static const DWORD offsetsFromUTF8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL};

static const BYTE bytesFromUTF8[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                        1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

static const DWORD firstByteMark[7] = {0x00UL, 0x00UL, 0xC0UL, 0xE0UL, 0xF0UL, 0xF8UL, 0xFCUL};

int ConvertUTF16toUTF8Length(const WORD *src, UINT srcMaxChars, UINT *srcChars) {
  const WORD *srcStart;
  const WORD *srcEnd;
  int         result;

  if (!srcMaxChars || !src) {
    if (srcChars) {
      *srcChars = 0;
    }

    return -1;
  }

  srcStart = src;
  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const WORD *>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  result = 0;

  while (src < srcEnd) {
    UCS4 ch = *src++;

    if (ch >= 0xD800 && ch <= 0xDBFF) {
      UCS4 ch2;

      if (src >= srcEnd) {
        goto sourceExhausted;
      }

      ch2 = *src;
      if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
        ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
        ++src;
      }
    }

    if (ch < 0x80) {
      ++result;
      if (!ch) {
        goto finished;
      }
    } else if (ch < 0x800) {
      result += 2;
    } else if (ch < 0x10000) {
      result += 3;
    } else if (ch < 0x200000) {
      result += 4;
    } else if (ch < 0x4000000) {
      result += 5;
    } else if (ch <= 0x7FFFFFFF) {
      result += 6;
    } else {
      result += 2;
    }
  }

sourceExhausted:
  result = -1;

finished:
  if (srcChars) {
    *srcChars = static_cast<UINT>(src - srcStart);
  }

  return result;
}

int ConvertUTF16toUTF8(char *dst, UINT dstMaxChars, const WORD *src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars) {
  const WORD *srcStart = src;
  const WORD *srcEnd;
  char       *dstStart = dst;
  char       *dstEnd = dst + dstMaxChars;
  int         result = 0;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const WORD *>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  while (src < srcEnd) {
    UCS4 ch = src[0];
    UINT srcIndex = 1;
    UINT bytesToWrite;

    if (ch >= 0xD800 && ch <= 0xDBFF) {
      UCS4 ch2;

      if (src + 1 >= srcEnd) {
        result = -1;
        goto finished;
      }

      ch2 = src[1];
      if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
        ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
        srcIndex = 2;
      }
    }

    if (ch < 0x80) {
      bytesToWrite = 1;
      if (!ch) {
        if (dst < dstEnd) {
          *dst++ = 0;
        } else {
          result = 1;
        }
        goto finished;
      }
    } else if (ch < 0x800) {
      bytesToWrite = 2;
    } else if (ch < 0x10000) {
      bytesToWrite = 3;
    } else if (ch < 0x200000) {
      bytesToWrite = 4;
    } else if (ch < 0x4000000) {
      bytesToWrite = 5;
    } else if (ch <= 0x7FFFFFFF) {
      bytesToWrite = 6;
    } else {
      ch = 0xFFFD;
      bytesToWrite = 2;
    }

    if (dst + bytesToWrite > dstEnd) {
      result = bytesToWrite;
      goto finished;
    }

    dst += bytesToWrite;
    switch (bytesToWrite) {
      case 6:
        *--dst = static_cast<char>((ch | 0x80) & 0xBF);
        ch >>= 6;
      case 5:
        *--dst = static_cast<char>((ch | 0x80) & 0xBF);
        ch >>= 6;
      case 4:
        *--dst = static_cast<char>((ch | 0x80) & 0xBF);
        ch >>= 6;
      case 3:
        *--dst = static_cast<char>((ch | 0x80) & 0xBF);
        ch >>= 6;
      case 2:
        *--dst = static_cast<char>((ch | 0x80) & 0xBF);
        ch >>= 6;
      case 1:
        *--dst = static_cast<char>(ch | firstByteMark[bytesToWrite]);
    }
    dst += bytesToWrite;
    src += srcIndex;
  }

  result = -1;

finished:
  if (srcChars) {
    *srcChars = static_cast<UINT>(src - srcStart);
  }
  if (dstChars) {
    *dstChars = static_cast<UINT>(dst - dstStart);
  }

  return result;
}

int ConvertUTF8toUTF16Length(LPCSTR src, UINT srcMaxChars, UINT *srcChars) {
  LPCSTR srcStart;
  LPCSTR srcEnd;
  int    result;

  if (!srcMaxChars || !src) {
    if (srcChars) {
      *srcChars = 0;
    }

    return -1;
  }

  srcStart = src;
  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<LPCSTR>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  result = 0;

  while (src < srcEnd) {
    UINT extraBytes = bytesFromUTF8[static_cast<BYTE>(*src)];
    UCS4 ch = 0;

    if (src + extraBytes >= srcEnd) {
      result = -1 - static_cast<int>(extraBytes);
      goto finished;
    }

    switch (extraBytes) {
      case 5:
        ch += static_cast<BYTE>(*src++);
        ch <<= 6;
      case 4:
        ch += static_cast<BYTE>(*src++);
        ch <<= 6;
      case 3:
        ch += static_cast<BYTE>(*src++);
        ch <<= 6;
      case 2:
        ch += static_cast<BYTE>(*src++);
        ch <<= 6;
      case 1:
        ch += static_cast<BYTE>(*src++);
        ch <<= 6;
      case 0:
        ch += static_cast<BYTE>(*src++);
    }
    ch -= offsetsFromUTF8[extraBytes];

    if (ch <= 0xFFFF) {
      ++result;
      if (!ch) {
        goto finished;
      }
    } else if (ch <= 0x10FFFF) {
      result += 2;
    } else {
      ++result;
    }
  }

  result = -1;

finished:
  if (srcChars) {
    *srcChars = static_cast<UINT>(src - srcStart);
  }

  return result;
}

int ConvertUTF8toUTF16(WORD *dst, UINT dstMaxChars, LPCSTR src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars) {
  LPCSTR srcStart = src;
  LPCSTR srcEnd;
  WORD  *dstStart = dst;
  WORD  *dstEnd = dst + dstMaxChars;
  int    result = 0;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<LPCSTR>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  while (src < srcEnd) {
    UINT extraBytes = bytesFromUTF8[static_cast<BYTE>(*src)];
    UINT srcIndex = 0;
    UCS4 ch = 0;

    if (src + extraBytes >= srcEnd) {
      result = -1 - static_cast<int>(extraBytes);
      goto finished;
    }

    switch (extraBytes) {
      case 5:
        ch += static_cast<BYTE>(src[srcIndex++]);
        ch <<= 6;
      case 4:
        ch += static_cast<BYTE>(src[srcIndex++]);
        ch <<= 6;
      case 3:
        ch += static_cast<BYTE>(src[srcIndex++]);
        ch <<= 6;
      case 2:
        ch += static_cast<BYTE>(src[srcIndex++]);
        ch <<= 6;
      case 1:
        ch += static_cast<BYTE>(src[srcIndex++]);
        ch <<= 6;
      case 0:
        ch += static_cast<BYTE>(src[srcIndex++]);
    }
    ch -= offsetsFromUTF8[extraBytes];

    if (dst >= dstEnd) {
      result = 1;
      goto finished;
    }

    if (ch <= 0xFFFF) {
      *dst++ = static_cast<WORD>(ch);
      if (!ch) {
        goto finished;
      }
    } else if (ch > 0x10FFFF) {
      *dst++ = 0xFFFD;
    } else {
      if (dst + 1 >= dstEnd) {
        result = 1;
        goto finished;
      }

      ch -= 0x10000;
      *dst++ = static_cast<WORD>((ch >> 10) + 0xD800);
      *dst++ = static_cast<WORD>((ch & 0x3FF) + 0xDC00);
    }

    src += srcIndex;
  }

  result = -1;

finished:
  if (srcChars) {
    *srcChars = static_cast<UINT>(src - srcStart);
  }
  if (dstChars) {
    *dstChars = static_cast<UINT>(dst - dstStart);
  }

  return result;
}

UINT sgetu8(const BYTE *strptr, int *chars) {
  UINT c;
  int  remaining;

  if (chars) {
    *chars = 0;
  }
  if (!strptr) {
    return static_cast<UINT>(-1);
  }

  c = *strptr++;
  if (!c) {
    return static_cast<UINT>(-1);
  }
  if (chars) {
    ++*chars;
  }

  if ((c & 0xFE) == 0xFC) {
    c &= 0x01;
    remaining = 5;
  } else if ((c & 0xFC) == 0xF8) {
    c &= 0x03;
    remaining = 4;
  } else if ((c & 0xF8) == 0xF0) {
    c &= 0x07;
    remaining = 3;
  } else if ((c & 0xF0) == 0xE0) {
    c &= 0x0F;
    remaining = 2;
  } else if ((c & 0xE0) == 0xC0) {
    c &= 0x1F;
    remaining = 1;
  } else {
    if ((c & 0x80) == 0x80) {
      return 0x80000000;
    }

    return c;
  }

  while (remaining-- > 0) {
    UINT next = *strptr++;

    if (!next) {
      return static_cast<UINT>(-1);
    }
    if (chars) {
      ++*chars;
    }
    if ((next & 0xC0) != 0x80) {
      return 0x80000000;
    }

    c = (c << 6) | (next & 0x3F);
  }

  return c;
}

char *sputu8(UINT c, char *strptr) {
  char *result = strptr;

  if (!strptr) {
    return result;
  }

  if (c < 0x80) {
    *strptr++ = static_cast<char>(c);
  } else if (c < 0x800) {
    *strptr++ = static_cast<char>((c >> 6) | 0xC0);
    *strptr++ = static_cast<char>((c & 0x3F) | 0x80);
  } else if (c < 0x10000) {
    *strptr++ = static_cast<char>((c >> 12) | 0xE0);
    *strptr++ = static_cast<char>(((c >> 6) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>((c & 0x3F) | 0x80);
  } else if (c < 0x200000) {
    *strptr++ = static_cast<char>((c >> 18) | 0xF0);
    *strptr++ = static_cast<char>(((c >> 12) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 6) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>((c & 0x3F) | 0x80);
  } else if (c < 0x400000) {
    *strptr++ = static_cast<char>((c >> 24) | 0xF8);
    *strptr++ = static_cast<char>(((c >> 18) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 12) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 6) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>((c & 0x3F) | 0x80);
  } else if (c < 0x80000000) {
    *strptr++ = static_cast<char>((c >> 30) | 0xFC);
    *strptr++ = static_cast<char>(((c >> 24) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 18) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 12) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>(((c >> 6) & 0x3F) | 0x80);
    *strptr++ = static_cast<char>((c & 0x3F) | 0x80);
  }

  *strptr = 0;
  return strptr;
}
