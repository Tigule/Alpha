#pragma once

int   ConvertUTF16toUTF8Length(const WORD *src, UINT srcMaxChars, UINT *srcChars);
int   ConvertUTF16toUTF8(char *dst, UINT dstMaxChars, const WORD *src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars);
int   ConvertUTF8toUTF16Length(LPCSTR src, UINT srcMaxChars, UINT *srcChars);
int   ConvertUTF8toUTF16(WORD *dst, UINT dstMaxChars, LPCSTR src, UINT srcMaxChars, UINT *dstChars, UINT *srcChars);
UINT  sgetu8(const BYTE *strptr, int *chars);
char *sputu8(UINT c, char *strptr);
