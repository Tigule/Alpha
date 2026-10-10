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
    srcEnd = (const WORD *)-1;
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
    *srcChars = src - srcStart;
  }

  return result;
}

int ConvertUTF16toUTF8(char *dst, UINT dstMaxChars, const WORD *src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars) {
  const WORD *srcStart = src;
  const WORD *srcEnd;
  char       *dstStart = dst;
  char       *dstEnd;
  int         result;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = (const WORD *)-1;
  } else {
    srcEnd = src + srcMaxChars;
  }

  result = 0;
  dstEnd = dst + dstMaxChars;

  for (;;) {
    if (src >= srcEnd) {
      result = -1;
      break;
    }

    UCS4 ch = src[0];
    UINT srcIndex = 1;
    UINT bytesToWrite;

    if (ch >= 0xD800 && ch <= 0xDBFF) {
      UCS4 ch2;

      if (src + 1 >= srcEnd) {
        result = -1;
        break;
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
          break;
        }

        result = 1;
        break;
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
      bytesToWrite = 2;
      ch = 0xFFFD;
    }

    if (dst + bytesToWrite > dstEnd) {
      result = bytesToWrite;
      break;
    }

    dst += bytesToWrite;
    switch (bytesToWrite) {
      case 6:
        *--dst = (ch | 0x80) & 0xBF;
        ch >>= 6;
      case 5:
        *--dst = (ch | 0x80) & 0xBF;
        ch >>= 6;
      case 4:
        *--dst = (ch | 0x80) & 0xBF;
        ch >>= 6;
      case 3:
        *--dst = (ch | 0x80) & 0xBF;
        ch >>= 6;
      case 2:
        *--dst = (ch | 0x80) & 0xBF;
        ch >>= 6;
      case 1:
        *--dst = ch | firstByteMark[bytesToWrite];
    }
    dst += bytesToWrite;
    src += srcIndex;
  }

  if (srcChars) {
    *srcChars = src - srcStart;
  }
  if (dstChars) {
    *dstChars = dst - dstStart;
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
    srcEnd = (LPCSTR)-1;
  } else {
    srcEnd = src + srcMaxChars;
  }

  result = 0;

  for (;;) {
    if (src >= srcEnd) {
      result = -1;
      break;
    }

    UINT extraBytes = bytesFromUTF8[(BYTE)*src];

    if (src + extraBytes >= srcEnd) {
      result = -1 - (int)extraBytes;
      break;
    }

    UCS4 ch = 0;

    switch (extraBytes) {
      case 5:
        ch += (BYTE)(*src++);
        ch <<= 6;
      case 4:
        ch += (BYTE)(*src++);
        ch <<= 6;
      case 3:
        ch += (BYTE)(*src++);
        ch <<= 6;
      case 2:
        ch += (BYTE)(*src++);
        ch <<= 6;
      case 1:
        ch += (BYTE)(*src++);
        ch <<= 6;
      case 0:
        ch += (BYTE)(*src++);
    }
    ch -= offsetsFromUTF8[extraBytes];

    if (ch <= 0xFFFF) {
      ++result;
      if (!ch) {
        break;
      }
    } else if (ch > 0x10FFFF) {
      ++result;
    } else {
      result += 2;
    }
  }

  if (srcChars) {
    *srcChars = src - srcStart;
  }

  return result;
}

int ConvertUTF8toUTF16(WORD *dst, UINT dstMaxChars, LPCSTR src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars) {
  LPCSTR srcStart = src;
  LPCSTR srcEnd;
  WORD  *dstStart = dst;
  WORD  *dstEnd;
  int    result;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = (LPCSTR)-1;
  } else {
    srcEnd = src + srcMaxChars;
  }

  dstEnd = dst + dstMaxChars;
  result = 0;

  for (;;) {
    if (src >= srcEnd) {
      result = -1;
      break;
    }

    UINT extraBytes = bytesFromUTF8[(BYTE)*src];

    if (src + extraBytes >= srcEnd) {
      result = -1 - (int)extraBytes;
      break;
    }

    UINT srcIndex = 0;
    UCS4 ch = 0;

    switch (extraBytes) {
      case 5:
        ch += (BYTE)src[srcIndex++];
        ch <<= 6;
      case 4:
        ch += (BYTE)src[srcIndex++];
        ch <<= 6;
      case 3:
        ch += (BYTE)src[srcIndex++];
        ch <<= 6;
      case 2:
        ch += (BYTE)src[srcIndex++];
        ch <<= 6;
      case 1:
        ch += (BYTE)src[srcIndex++];
        ch <<= 6;
      case 0:
        ch += (BYTE)src[srcIndex++];
    }
    ch -= offsetsFromUTF8[extraBytes];

    if (dst >= dstEnd) {
      result = 1;
      break;
    }

    if (ch <= 0xFFFF) {
      *dst++ = ch;
      if (!ch) {
        break;
      }
    } else if (ch > 0x10FFFF) {
      *dst++ = 0xFFFD;
    } else {
      if (dst + 1 >= dstEnd) {
        result = 1;
        break;
      }

      ch -= 0x10000;
      *dst++ = (ch >> 10) + 0xD800;
      *dst++ = (ch & 0x3FF) + 0xDC00;
    }

    src += srcIndex;
  }

  if (srcChars) {
    *srcChars = src - srcStart;
  }
  if (dstChars) {
    *dstChars = dst - dstStart;
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
    return -1;
  }

  c = *strptr++;
  if (!c) {
    return -1;
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

  for (int i = 0; i < remaining; ++i) {
    BYTE next = *strptr++;

    if (!next) {
      return -1;
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
    *strptr++ = c;
  } else if (c < 0x800) {
    *strptr++ = (c >> 6) | 0xC0;
    *strptr++ = (c & 0x3F) | 0x80;
  } else if (c < 0x10000) {
    *strptr++ = (c >> 12) | 0xE0;
    *strptr++ = ((c >> 6) & 0x3F) | 0x80;
    *strptr++ = (c & 0x3F) | 0x80;
  } else if (c < 0x200000) {
    *strptr++ = (c >> 18) | 0xF0;
    *strptr++ = ((c >> 12) & 0x3F) | 0x80;
    *strptr++ = ((c >> 6) & 0x3F) | 0x80;
    *strptr++ = (c & 0x3F) | 0x80;
  } else if (c < 0x400000) {
    *strptr++ = (c >> 24) | 0xF8;
    *strptr++ = ((c >> 18) & 0x3F) | 0x80;
    *strptr++ = ((c >> 12) & 0x3F) | 0x80;
    *strptr++ = ((c >> 6) & 0x3F) | 0x80;
    *strptr++ = (c & 0x3F) | 0x80;
  } else if (c < 0x80000000) {
    *strptr++ = (c >> 30) | 0xFC;
    *strptr++ = ((c >> 24) & 0x3F) | 0x80;
    *strptr++ = ((c >> 18) & 0x3F) | 0x80;
    *strptr++ = ((c >> 12) & 0x3F) | 0x80;
    *strptr++ = ((c >> 6) & 0x3F) | 0x80;
    *strptr++ = (c & 0x3F) | 0x80;
  }

  *strptr = 0;
  return strptr;
}
