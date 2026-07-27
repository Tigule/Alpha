#pragma once

int OsClipboardGetString(char *buf, unsigned int bufSize);
char *OsClipboardGetString();
void OsClipboardFreeString(char *string);
int OsClipboardPutString(const char *string);
