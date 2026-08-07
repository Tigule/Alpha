#pragma once

void BaseFileInitialize();
void BaseFileDestroy();
BOOL BaseFilePrefetch(LPCSTR fileName);
int  BaseFileIsFetched(LPCSTR fileName);
int  BaseFileLoad(LPCSTR fileName, LPVOID *fileBuffer, DWORD *fileSize);
void BaseFileFlush();
void BaseFileRegisterUncachable(LPCSTR fileName);
void BaseFileUnregisterUncachable(LPCSTR fileName);
void BaseFileDumpStats();
