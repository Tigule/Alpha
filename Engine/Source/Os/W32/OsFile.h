#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOSFILE);

#define HOSFILE_INVALID ((HOSFILE) - 1)

struct OS_FILE_DATA {
  DWORD size;
  DWORD flags;
  char  fileName[260];
};

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType);
void      OsCloseFile(HOSFILE fileHandle);
int       OsDeleteFile(LPCSTR fileName);
int       OsDirectoryExists(LPCSTR dirName);
int       OsFileExists(LPCSTR path);
int       OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead);
int       OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten);
DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod);
DWORD     OsGetFileAttributes(LPCSTR fileName);
int       OsCreateDirectory(LPCSTR pathName, int recursive);
int       OsSetCurrentDirectory(LPCSTR pathName);
