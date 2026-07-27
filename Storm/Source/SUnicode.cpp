#include <storm.h>

typedef unsigned long UCS4;

static const unsigned long offsetsFromUTF8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL};

static const unsigned char bytesFromUTF8[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

static const unsigned long firstByteMark[7] = {0x00UL, 0x00UL, 0xC0UL, 0xE0UL, 0xF0UL, 0xF8UL, 0xFCUL};

#define ASCII_CODE_PAGE                                                                                                                           \
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, \
      0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022,     \
      0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033,     \
      0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044,     \
      0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055,     \
      0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066,     \
      0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     \
      0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F

static const unsigned short CP1252[256] = {
    ASCII_CODE_PAGE, 0x20AC, 0xFFFF, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFF, 0x017D, 0xFFFF,
    0xFFFF,          0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFF, 0x017E, 0x0178, 0x00A0,
    0x00A1,          0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF, 0x00B0, 0x00B1,
    0x00B2,          0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF, 0x00C0, 0x00C1, 0x00C2,
    0x00C3,          0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3,
    0x00D4,          0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4,
    0x00E5,          0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5,
    0x00F6,          0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

static const unsigned short CP10000[256] = {
    ASCII_CODE_PAGE, 0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1, 0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
    0x00EA,          0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3, 0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC, 0x2020,
    0x00B0,          0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF, 0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8, 0x221E, 0x00B1,
    0x2264,          0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211, 0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x2126, 0x00E6, 0x00F8, 0x00BF, 0x00A1, 0x00AC,
    0x221A,          0x0192, 0x2248, 0x2206, 0x00AB, 0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153, 0x2013, 0x2014, 0x201C, 0x201D,
    0x2018,          0x2019, 0x00F7, 0x25CA, 0x00FF, 0x0178, 0x2044, 0x00A4, 0x2039, 0x203A, 0xFB01, 0xFB02, 0x2021, 0x00B7, 0x201A, 0x201E, 0x2030,
    0x00C2,          0x00CA, 0x00C1, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4, 0xFFFF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131,
    0x02C6,          0x02DC, 0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7,
};

static const unsigned short CP437[256] = {
    ASCII_CODE_PAGE, 0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9,          0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 0x00E1,
    0x00ED,          0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 0x2591, 0x2592,
    0x2593,          0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 0x2514, 0x2534, 0x252C,
    0x251C,          0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559,
    0x2558,          0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3,
    0x03C3,          0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, 0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321,
    0x00F7,          0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
};

extern "C" int APIENTRY SUniConvertUTF16to8Len(const unsigned short *src, unsigned long srcMaxChars, unsigned long *srcChars) {
  const unsigned short *srcStart;
  const unsigned short *srcEnd;
  int                   result;

  if (!srcMaxChars || !src) {
    if (srcChars) {
      *srcChars = 0;
    }

    return -1;
  }

  srcStart = src;
  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const unsigned short *>(-1);
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
    *srcChars = static_cast<unsigned int>(src - srcStart);
  }

  return result;
}

extern "C" int APIENTRY SUniConvertUTF16to8(
    char                 *dst,
    unsigned long         dstMaxChars,
    const unsigned short *src,
    unsigned long         srcMaxChars,
    unsigned long        *dstChars,
    unsigned long        *srcChars
) {
  char                 *dstStart;
  const unsigned short *srcStart;
  int                   result;
  const unsigned short *srcEnd;
  unsigned int          srcIndex;
  char                 *dstEnd;

  dstStart = dst;
  srcStart = src;
  result = 0;
  dstEnd = dst + dstMaxChars;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const unsigned short *>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  while (src < srcEnd) {
    UCS4         ch = src[0];
    unsigned int bytesToWrite;

    srcIndex = 1;

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
    *srcChars = static_cast<unsigned int>(src - srcStart);
  }
  if (dstChars) {
    *dstChars = static_cast<unsigned int>(dst - dstStart);
  }

  return result;
}

extern "C" int APIENTRY SUniConvertUTF8to16Len(const char *src, unsigned long srcMaxChars, unsigned long *srcChars) {
  const char *srcStart;
  const char *srcEnd;
  int         result;

  if (!srcMaxChars || !src) {
    if (srcChars) {
      *srcChars = 0;
    }

    return -1;
  }

  srcStart = src;
  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const char *>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  result = 0;

  while (src < srcEnd) {
    unsigned int extraBytes = bytesFromUTF8[*src];
    UCS4         ch = 0;

    if (src + extraBytes >= srcEnd) {
      result = -1 - static_cast<int>(extraBytes);
      goto finished;
    }

    switch (extraBytes) {
      case 5:
        ch += *src++;
        ch <<= 6;
      case 4:
        ch += *src++;
        ch <<= 6;
      case 3:
        ch += *src++;
        ch <<= 6;
      case 2:
        ch += *src++;
        ch <<= 6;
      case 1:
        ch += *src++;
        ch <<= 6;
      case 0:
        ch += *src++;
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
    *srcChars = static_cast<unsigned int>(src - srcStart);
  }

  return result;
}

extern "C" int APIENTRY SUniConvertUTF8to16(
    unsigned short *dst,
    unsigned long   dstMaxChars,
    const char     *src,
    unsigned long   srcMaxChars,
    unsigned long  *dstChars,
    unsigned long  *srcChars
) {
  unsigned short *dstStart;
  const char     *srcStart;
  unsigned short *dstEnd;
  int             result;
  const char     *srcEnd;

  dstStart = dst;
  srcStart = src;

  if (srcMaxChars == 0x7FFFFFFF) {
    srcEnd = reinterpret_cast<const char *>(-1);
  } else {
    srcEnd = src + srcMaxChars;
  }

  dstEnd = dst + dstMaxChars;
  result = 0;

  while (src < srcEnd) {
    unsigned int extraBytes;
    unsigned int srcIndex;
    UCS4         ch;

    extraBytes = bytesFromUTF8[static_cast<unsigned char>(*src)];

    if (src + extraBytes >= srcEnd) {
      result = -1 - static_cast<int>(extraBytes);
      goto finished;
    }

    srcIndex = 0;
    ch = 0;

    switch (extraBytes) {
      case 5:
        ch += static_cast<unsigned char>(src[srcIndex++]);
        ch <<= 6;
      case 4:
        ch += static_cast<unsigned char>(src[srcIndex++]);
        ch <<= 6;
      case 3:
        ch += static_cast<unsigned char>(src[srcIndex++]);
        ch <<= 6;
      case 2:
        ch += static_cast<unsigned char>(src[srcIndex++]);
        ch <<= 6;
      case 1:
        ch += static_cast<unsigned char>(src[srcIndex++]);
        ch <<= 6;
      case 0:
        ch += static_cast<unsigned char>(src[srcIndex++]);
    }
    ch -= offsetsFromUTF8[extraBytes];

    if (dst >= dstEnd) {
      result = 1;
      goto finished;
    }

    if (ch <= 0xFFFF) {
      *dst++ = static_cast<unsigned short>(ch);
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
      *dst++ = static_cast<unsigned short>((ch >> 10) + 0xD800);
      *dst++ = static_cast<unsigned short>((ch & 0x3FF) + 0xDC00);
    }

    src += srcIndex;
  }

  result = -1;

finished:
  if (srcChars) {
    *srcChars = static_cast<unsigned int>(src - srcStart);
  }
  if (dstChars) {
    *dstChars = static_cast<unsigned int>(dst - dstStart);
  }

  return result;
}

extern "C" unsigned int APIENTRY SUniSGetUTF8(const unsigned char *strptr, int *chars) {
  unsigned int c;
  int          remaining;

  if (chars) {
    *chars = 0;
  }
  if (!strptr) {
    return static_cast<unsigned int>(-1);
  }

  c = *strptr++;
  if (!c) {
    return static_cast<unsigned int>(-1);
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
    unsigned int next = *strptr++;

    if (!next) {
      return static_cast<unsigned int>(-1);
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

extern "C" char *APIENTRY SUniSPutUTF8(unsigned long c, char *strptr) {
  if (!strptr) {
    return strptr;
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

static int FindUTF8Character(const char *utf8String, int index, int direction) {
  while (index > 0 && utf8String[index] && (((unsigned char)utf8String[index] & 0xC0) == 0x80)) {
    index += direction;
  }
  return index;
}

extern "C" int APIENTRY SUniFindUTF8ChrStart(const char *utf8String, int index) {
  return FindUTF8Character(utf8String, index, -1);
}

extern "C" int APIENTRY SUniFindAfterUTF8Chr(const char *utf8String, int index) {
  return FindUTF8Character(utf8String, index + 1, 1);
}

static DWORD SUniConvertUTF16ToCP(unsigned short *codepage, char *dest, const unsigned short *source, DWORD destsize) {
  char *start;

  if (!destsize) {
    return 0;
  }

  start = dest;
  while (*source && destsize) {
    unsigned short ch;
    unsigned int   cp;

    ch = *source;
    if (ch < 0x100 && codepage[ch] == ch) {
      cp = (unsigned char)*source;
    } else {
      cp = 0xFF;
      while (cp > 0 && codepage[cp] != ch) {
        --cp;
      }
      if (!cp) {
        cp = '?';
      }
    }

    *dest++ = (char)cp;
    ++source;
    --destsize;
  }

  if (destsize) {
    *dest++ = 0;
  }

  return (DWORD)(dest - start);
}

static DWORD SUniConvertCPToUTF16(unsigned short *codepage, unsigned short *dest, const char *source, DWORD destsize) {
  unsigned short *start;

  if (!destsize) {
    return 0;
  }

  start = dest;
  while (*source && destsize) {
    *dest++ = codepage[(unsigned char)*source];
    ++source;
    --destsize;
  }

  if (destsize) {
    *dest++ = 0;
  }

  return (DWORD)(dest - start);
}

extern "C" DWORD APIENTRY SUniConvertUTF16ToWin(char *dest, const unsigned short *source, DWORD destsize) {
  return SUniConvertUTF16ToCP(const_cast<unsigned short *>(CP1252), dest, source, destsize);
}

extern "C" DWORD APIENTRY SUniConvertUTF16ToMac(char *dest, const unsigned short *source, DWORD destsize) {
  return SUniConvertUTF16ToCP(const_cast<unsigned short *>(CP10000), dest, source, destsize);
}

extern "C" DWORD APIENTRY SUniConvertUTF16ToDos(char *dest, const unsigned short *source, DWORD destsize) {
  return SUniConvertUTF16ToCP(const_cast<unsigned short *>(CP437), dest, source, destsize);
}

extern "C" DWORD APIENTRY SUniConvertWinToUTF16(unsigned short *dest, const char *source, DWORD destsize) {
  return SUniConvertCPToUTF16(const_cast<unsigned short *>(CP1252), dest, source, destsize);
}

extern "C" DWORD APIENTRY SUniConvertMacToUTF16(unsigned short *dest, const char *source, DWORD destsize) {
  return SUniConvertCPToUTF16(const_cast<unsigned short *>(CP10000), dest, source, destsize);
}

extern "C" DWORD APIENTRY SUniConvertDosToUTF16(unsigned short *dest, const char *source, DWORD destsize) {
  return SUniConvertCPToUTF16(const_cast<unsigned short *>(CP437), dest, source, destsize);
}
