#pragma once

void BaseFileInitialize();
void BaseFileDestroy();
int  BaseFilePrefetch(const char *fileName);
int  BaseFileIsFetched(const char *fileName);
int  BaseFileLoad(const char *fileName, void **fileBuffer, unsigned long *fileSize);
void BaseFileFlush();
void BaseFileRegisterUncachable(const char *fileName);
void BaseFileUnregisterUncachable(const char *fileName);
void BaseFileDumpStats();
