#include "OsFile.h"

#include <storm.h>
#include <windows.h>

DWORD __fastcall OsPathGetRootChars(const char *path);

HOSFILE __fastcall OsCreateFile(
    const char   *fileName,
    unsigned long desiredAccess,
    unsigned long shareMode,
    unsigned long createDisposition,
    unsigned long flagsAndAttributes,
    unsigned long extendedFileType
) {
  unsigned short fileName16[MAX_PATH];

  ASSERT(fileName);
  if (!fileName) {
    return reinterpret_cast<HOSFILE>(INVALID_HANDLE_VALUE);
  }

  ASSERT(desiredAccess);
  if (!desiredAccess) {
    return reinterpret_cast<HOSFILE>(INVALID_HANDLE_VALUE);
  }

  ASSERT(createDisposition >= CREATE_NEW);
  ASSERT(createDisposition <= TRUNCATE_EXISTING);

  if (createDisposition < CREATE_NEW || createDisposition > TRUNCATE_EXISTING) {
    return reinterpret_cast<HOSFILE>(INVALID_HANDLE_VALUE);
  }

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);

  (void)extendedFileType;
  return reinterpret_cast<HOSFILE>(
      CreateFileW(reinterpret_cast<LPCWSTR>(fileName16), desiredAccess, shareMode, 0, createDisposition, flagsAndAttributes, 0)
  );
}

void __fastcall OsCloseFile(HOSFILE fileHandle) {
  CloseHandle(reinterpret_cast<HANDLE>(fileHandle));
}

int __fastcall OsFileExists(const char *path) {
  unsigned long attributes;

  if (!path || !path[0]) {
    return 0;
  }

  attributes = OsGetFileAttributes(path);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int __fastcall OsDirectoryExists(const char *dirName) {
  if (!dirName || !dirName[0]) {
    return 0;
  }

  unsigned long attributes = OsGetFileAttributes(dirName);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int __fastcall OsReadFile(HOSFILE fileHandle, void *buffer, unsigned long bytesToRead, unsigned long *bytesRead) {
  FATALASSERT(buffer);

  FATALASSERT(bytesRead);

  return ReadFile(reinterpret_cast<HANDLE>(fileHandle), buffer, bytesToRead, bytesRead, 0);
}

int __fastcall OsWriteFile(HOSFILE fileHandle, const void *buffer, unsigned long bytesToWrite, unsigned long *bytesWritten) {
  FATALASSERT(buffer);

  FATALASSERT(bytesWritten);

  return WriteFile(reinterpret_cast<HANDLE>(fileHandle), buffer, bytesToWrite, bytesWritten, 0);
}

unsigned __int64 __fastcall OsSetFilePointer(HOSFILE fileHandle, __int64 distanceToMove, unsigned long moveMethod) {
  LARGE_INTEGER distance;

  distance.QuadPart = distanceToMove;
  distance.LowPart = SetFilePointer(reinterpret_cast<HANDLE>(fileHandle), distance.LowPart, &distance.HighPart, moveMethod);

  if (distance.LowPart == 0xFFFFFFFF && GetLastError()) {
    return static_cast<unsigned __int64>(-1);
  }

  return static_cast<unsigned __int64>(distance.QuadPart);
}

unsigned long __fastcall OsGetFileAttributes(const char *fileName) {
  unsigned short fileName16[MAX_PATH];

  FATALASSERT(fileName);

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return GetFileAttributesW(reinterpret_cast<LPCWSTR>(fileName16));
}

int __fastcall OsDeleteFile(const char *fileName) {
  unsigned short fileName16[MAX_PATH];

  FATALASSERT(fileName);

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return DeleteFileW(reinterpret_cast<LPCWSTR>(fileName16));
}

int __fastcall OsCreateDirectory(const char *pathName, int recursive) {
  char           tempName[MAX_PATH];
  unsigned short pathName16[MAX_PATH];

  FATALASSERT(pathName);

  if (recursive) {
    SStrCopy(tempName, pathName, MAX_PATH);

    char *slash = SStrChr(tempName + OsPathGetRootChars(tempName), '\\');
    while (slash) {
      *slash = 0;
      SUniConvertUTF8to16(pathName16, MAX_PATH, tempName, 0x7FFFFFFF, 0, 0);
      CreateDirectoryW(reinterpret_cast<LPCWSTR>(pathName16), 0);
      *slash = '\\';
      slash = SStrChr(slash + 1, '\\');
    }
  }

  SUniConvertUTF8to16(pathName16, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return CreateDirectoryW(reinterpret_cast<LPCWSTR>(pathName16), 0);
}

int __fastcall OsSetCurrentDirectory(const char *pathName) {
  unsigned short dst[MAX_PATH];

  FATALASSERT(pathName);

  SUniConvertUTF8to16(dst, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return SetCurrentDirectoryW(reinterpret_cast<LPCWSTR>(dst));
}
