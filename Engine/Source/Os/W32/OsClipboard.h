#pragma once

int   OsClipboardGetString(char *buf, UINT bufSize);
char *OsClipboardGetString();
void  OsClipboardFreeString(char *string);
int   OsClipboardPutString(LPCSTR string);
