#pragma once

int __fastcall   OsClipboardGetString(char *buf, unsigned int bufSize);
char *__fastcall OsClipboardGetString();
void __fastcall  OsClipboardFreeString(char *string);
int __fastcall   OsClipboardPutString(const char *string);
