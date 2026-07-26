#pragma once

void __fastcall BaseFileInitialize();
void __fastcall BaseFileDestroy();
int  __fastcall BaseFilePrefetch(const char *fileName);
int  __fastcall BaseFileIsFetched(const char *fileName);
int  __fastcall BaseFileLoad(const char *fileName, void **fileBuffer, unsigned long *fileSize);
void __fastcall BaseFileFlush();
void __fastcall BaseFileRegisterUncachable(const char *fileName);
void __fastcall BaseFileUnregisterUncachable(const char *fileName);
void __fastcall BaseFileDumpStats();
