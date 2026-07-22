#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOSFILE);

#define HOSFILE_INVALID ((HOSFILE) - 1)

HOSFILE __fastcall OsCreateFile(
    const char   *fileName,
    unsigned long desiredAccess,
    unsigned long shareMode,
    unsigned long createDisposition,
    unsigned long flagsAndAttributes,
    unsigned long extendedFileType
);
void __fastcall             OsCloseFile(HOSFILE fileHandle);
int __fastcall              OsDeleteFile(const char *fileName);
int __fastcall              OsDirectoryExists(const char *dirName);
int __fastcall              OsFileExists(const char *path);
int __fastcall              OsReadFile(HOSFILE fileHandle, void *buffer, unsigned long bytesToRead, unsigned long *bytesRead);
int __fastcall              OsWriteFile(HOSFILE fileHandle, const void *buffer, unsigned long bytesToWrite, unsigned long *bytesWritten);
unsigned __int64 __fastcall OsSetFilePointer(HOSFILE fileHandle, __int64 distanceToMove, unsigned long moveMethod);
unsigned long __fastcall    OsGetFileAttributes(const char *fileName);
int __fastcall              OsCreateDirectory(const char *pathName, int recursive);
int __fastcall              OsSetCurrentDirectory(const char *pathName);
