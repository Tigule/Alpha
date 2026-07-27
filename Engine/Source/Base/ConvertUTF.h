#pragma once

int ConvertUTF16toUTF8Length(const unsigned short *src, unsigned int srcMaxChars, unsigned int *srcChars);
int ConvertUTF16toUTF8(
    char                 *dst,
    unsigned int          dstMaxChars,
    const unsigned short *src,
    unsigned int          srcMaxChars,
    unsigned int         *dstChars,
    unsigned int         *srcChars
);
int ConvertUTF8toUTF16Length(const char *src, unsigned int srcMaxChars, unsigned int *srcChars);
int ConvertUTF8toUTF16(
    unsigned short *dst,
    unsigned int    dstMaxChars,
    const char     *src,
    unsigned int    srcMaxChars,
    unsigned int   *dstChars,
    unsigned int   *srcChars
);
unsigned int sgetu8(const unsigned char *strptr, int *chars);
char *sputu8(unsigned int c, char *strptr);
