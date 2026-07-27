#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOSFILE);

#define HOSFILE_INVALID ((HOSFILE) - 1)

struct OS_FILE_DATA {
  unsigned long size;
  unsigned long flags;
  char          fileName[260];
};

HOSFILE OsCreateFile(
    const char   *fileName,
    unsigned long desiredAccess,
    unsigned long shareMode,
    unsigned long createDisposition,
    unsigned long flagsAndAttributes,
    unsigned long extendedFileType
);
void OsCloseFile(HOSFILE fileHandle);
int OsDeleteFile(const char *fileName);
int OsDirectoryExists(const char *dirName);
int OsFileExists(const char *path);
int OsReadFile(HOSFILE fileHandle, void *buffer, unsigned long bytesToRead, unsigned long *bytesRead);
int OsWriteFile(HOSFILE fileHandle, const void *buffer, unsigned long bytesToWrite, unsigned long *bytesWritten);
unsigned __int64 OsSetFilePointer(HOSFILE fileHandle, __int64 distanceToMove, unsigned long moveMethod);
unsigned long OsGetFileAttributes(const char *fileName);
int OsCreateDirectory(const char *pathName, int recursive);
int OsSetCurrentDirectory(const char *pathName);
