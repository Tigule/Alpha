#pragma once

BOOL  OsClipboardGetString(char *buf, UINT bufSize);
char *OsClipboardGetString();
void  OsClipboardFreeString(char *string);
BOOL  OsClipboardPutString(LPCSTR string);
