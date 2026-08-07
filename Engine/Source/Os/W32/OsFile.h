#pragma once

#include <storm.h>

DECLARE_STRICT_HANDLE(HOSFILE);

#define HOSFILE_INVALID ((HOSFILE) - 1)

struct OS_FILE_DATA {
  DWORD size;
  DWORD flags;
  char  fileName[MAX_PATH];
};

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType);
void      OsCloseFile(HOSFILE fileHandle);
BOOL      OsDeleteFile(LPCSTR fileName);
BOOL      OsDirectoryExists(LPCSTR dirName);
BOOL      OsFileExists(LPCSTR path);
BOOL      OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead);
BOOL      OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten);
DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod);
DWORD     OsGetFileAttributes(LPCSTR fileName);
BOOL      OsCreateDirectory(LPCSTR pathName, int recursive);
BOOL      OsSetCurrentDirectory(LPCSTR pathName);
