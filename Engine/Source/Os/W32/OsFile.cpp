#include <Base/Base.h>

#include "OsFile.h"

#include <malloc.h>
#include <storm.h>
#include <windows.h>

DWORD OsPathGetRootChars(LPCSTR path);
void  OsPathStripFilename(char *buffer);

static void UTF16ToUTF8(const WORD *src, char *dest, DWORD destLength) {
  DWORD destChars;
  SUniConvertUTF16to8(dest, destLength, src, 0x7FFFFFFF, &destChars, 0);
  if (destLength - 1 < destChars) {
    dest[destLength - 1] = 0;
    return;
  }
  dest[destChars] = 0;
}

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType) {
  WORD fileName16[MAX_PATH];

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

void OsCloseFile(HOSFILE fileHandle) {
  CloseHandle(reinterpret_cast<HANDLE>(fileHandle));
}

int OsFileExists(LPCSTR path) {
  DWORD attributes;

  if (!path || !path[0]) {
    return 0;
  }

  attributes = OsGetFileAttributes(path);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int OsDirectoryExists(LPCSTR dirName) {
  if (!dirName || !dirName[0]) {
    return 0;
  }

  DWORD attributes = OsGetFileAttributes(dirName);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead) {
  FATALASSERT(buffer);

  FATALASSERT(bytesRead);

  return ReadFile(reinterpret_cast<HANDLE>(fileHandle), buffer, bytesToRead, bytesRead, 0);
}

int OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten) {
  FATALASSERT(buffer);

  FATALASSERT(bytesWritten);

  return WriteFile(reinterpret_cast<HANDLE>(fileHandle), buffer, bytesToWrite, bytesWritten, 0);
}

int OsFlushFile(HOSFILE__ *fileHandle) {
  return FlushFileBuffers(reinterpret_cast<HANDLE>(fileHandle));
}

DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod) {
  LARGE_INTEGER distance;

  distance.QuadPart = distanceToMove;
  distance.LowPart = SetFilePointer(reinterpret_cast<HANDLE>(fileHandle), distance.LowPart, &distance.HighPart, moveMethod);

  if (distance.LowPart == 0xFFFFFFFF && GetLastError()) {
    return static_cast<DWORDLONG>(-1);
  }

  return static_cast<DWORDLONG>(distance.QuadPart);
}

DWORDLONG OsGetFileSize(HOSFILE__ *fileHandle) {
  LARGE_INTEGER size;
  size.LowPart = GetFileSize(reinterpret_cast<HANDLE>(fileHandle), reinterpret_cast<DWORD *>(&size.HighPart));
  return size.QuadPart;
}

int OsGetFileTime(HOSFILE__ *fileHandle, OSFILETIME *createFileTime, OSFILETIME *accessFileTime, OSFILETIME *writeFileTime) {
  return GetFileTime(
      reinterpret_cast<HANDLE>(fileHandle), reinterpret_cast<FILETIME *>(createFileTime), reinterpret_cast<FILETIME *>(accessFileTime),
      reinterpret_cast<FILETIME *>(writeFileTime)
  );
}

int OsSetFileTime(HOSFILE__ *fileHandle, const OSFILETIME *createFileTime, const OSFILETIME *accessFileTime, const OSFILETIME *writeFileTime) {
  return SetFileTime(
      reinterpret_cast<HANDLE>(fileHandle), reinterpret_cast<const FILETIME *>(createFileTime), reinterpret_cast<const FILETIME *>(accessFileTime),
      reinterpret_cast<const FILETIME *>(writeFileTime)
  );
}

int OsGetFileTime(LPCSTR fileName, OSFILETIME *createFileTime, OSFILETIME *accessFileTime, OSFILETIME *writeFileTime) {
  if (createFileTime) {
    createFileTime->m_value = 0;
  }
  if (accessFileTime) {
    accessFileTime->m_value = 0;
  }
  if (writeFileTime) {
    writeFileTime->m_value = 0;
  }

  HOSFILE file = OsCreateFile(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0x3F3F3F3F);
  if (file == HOSFILE_INVALID) {
    return 0;
  }

  int result = OsGetFileTime(file, createFileTime, accessFileTime, writeFileTime);
  OsCloseFile(file);
  return result;
}

int OsSetEndOfFile(HOSFILE__ *fileHandle) {
  return SetEndOfFile(reinterpret_cast<HANDLE>(fileHandle));
}

DWORD OsGetFileAttributes(LPCSTR fileName) {
  FATALASSERT(fileName);

  WORD *fileName16 = static_cast<WORD *>(_alloca(MAX_PATH * sizeof(WORD)));
  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return GetFileAttributesW(reinterpret_cast<LPCWSTR>(fileName16));
}

int OsSetFileAttributes(LPCSTR fileName, DWORD attributes) {
  FATALASSERT(fileName);
  WORD *fileName16 = static_cast<WORD *>(_alloca(MAX_PATH * sizeof(WORD)));
  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return SetFileAttributesW(reinterpret_cast<LPCWSTR>(fileName16), attributes);
}

int OsMoveFile(LPCSTR existingFileName, LPCSTR newFileName) {
  WORD existingFileName16[MAX_PATH];
  WORD newFileName16[MAX_PATH];
  FATALASSERT(existingFileName);
  FATALASSERT(newFileName);
  SUniConvertUTF8to16(existingFileName16, MAX_PATH, existingFileName, 0x7FFFFFFF, 0, 0);
  SUniConvertUTF8to16(newFileName16, MAX_PATH, newFileName, 0x7FFFFFFF, 0, 0);
  return MoveFileW(reinterpret_cast<LPCWSTR>(existingFileName16), reinterpret_cast<LPCWSTR>(newFileName16));
}

int OsCopyFile(LPCSTR existingFileName, LPCSTR newFileName, int failIfExists) {
  WORD existingFileName16[MAX_PATH];
  WORD newFileName16[MAX_PATH];
  SUniConvertUTF8to16(newFileName16, MAX_PATH, newFileName, 0x7FFFFFFF, 0, 0);
  SUniConvertUTF8to16(existingFileName16, MAX_PATH, existingFileName, 0x7FFFFFFF, 0, 0);
  return CopyFileW(reinterpret_cast<LPCWSTR>(existingFileName16), reinterpret_cast<LPCWSTR>(newFileName16), failIfExists);
}

int OsDeleteFile(LPCSTR fileName) {
  FATALASSERT(fileName);

  WORD *fileName16 = static_cast<WORD *>(_alloca(MAX_PATH * sizeof(WORD)));
  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return DeleteFileW(reinterpret_cast<LPCWSTR>(fileName16));
}

int OsCreateDirectory(LPCSTR pathName, int recursive) {
  char tempName[MAX_PATH];
  WORD pathName16[MAX_PATH];

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

int OsRemoveDirectory(LPCSTR pathName) {
  FATALASSERT(pathName);
  WORD *pathName16 = static_cast<WORD *>(_alloca(MAX_PATH * sizeof(WORD)));
  SUniConvertUTF8to16(pathName16, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return RemoveDirectoryW(reinterpret_cast<LPCWSTR>(pathName16));
}

struct RemoveDirectoryRecurseData {
  LPCSTR path;
  DWORD  flags;
};

int OsFileList(LPCSTR inDir, LPCSTR inPattern, int (*inCallback)(OS_FILE_DATA &, LPVOID), LPVOID inCBParam, int returnHidden) {
  char             findPath[MAX_PATH];
  WORD             findPath16[MAX_PATH];
  WIN32_FIND_DATAW findData;
  OS_FILE_DATA     osfData;

  SStrCopy(findPath, inDir, 0x7FFFFFFF);
  OsPathStripFilename(findPath);
  SStrPack(findPath, inPattern, 0x7FFFFFFF);
  SUniConvertUTF8to16(findPath16, MAX_PATH, findPath, 0x7FFFFFFF, 0, 0);

  HANDLE findHandle = FindFirstFileW(reinterpret_cast<LPCWSTR>(findPath16), &findData);
  int    result = 0;
  if (findHandle != INVALID_HANDLE_VALUE) {
    for (;;) {
      UTF16ToUTF8(findData.cFileName, osfData.fileName, sizeof(osfData.fileName));
      osfData.size = findData.nFileSizeLow;
      osfData.flags = 0;
      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        osfData.flags = FILE_ATTRIBUTE_DIRECTORY;
      }
      if (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        osfData.flags |= FILE_ATTRIBUTE_READONLY;
      }
      if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
        osfData.flags |= FILE_ATTRIBUTE_HIDDEN;
      }
      if ((returnHidden || !(findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) && inCallback(osfData, inCBParam)) {
        result = 1;
        break;
      }
      if (!FindNextFileW(findHandle, &findData)) {
        break;
      }
    }
    FindClose(findHandle);
  }
  return result;
}

static int EnumRemoveDirectoryRecurse(OS_FILE_DATA &file, LPVOID param) {
  RemoveDirectoryRecurseData *data = static_cast<RemoveDirectoryRecurseData *>(param);
  LPCSTR                      pathSlash;
  char                        relPath[MAX_PATH];

  pathSlash = data->path;

  if (file.flags & FILE_ATTRIBUTE_DIRECTORY) {
    if (SStrCmp(file.fileName, "..", 0x7FFFFFFF) && SStrCmp(file.fileName, ".", 0x7FFFFFFF)) {
      SStrCopy(relPath, pathSlash, sizeof(relPath));
      SStrPack(relPath, file.fileName, sizeof(relPath));
      SStrPack(relPath, "\\", sizeof(relPath));
      RemoveDirectoryRecurseData recurseData = {relPath, data->flags};
      OsFileList(relPath, "*", EnumRemoveDirectoryRecurse, &recurseData, 0);
      if (!OsRemoveDirectory(relPath)) {
        return 1;
      }
    }
  } else {
    SStrCopy(relPath, pathSlash, sizeof(relPath));
    SStrPack(relPath, file.fileName, sizeof(relPath));
    if (data->flags & 1) {
      OsSetFileAttributes(relPath, FILE_ATTRIBUTE_NORMAL);
    }
    OsDeleteFile(relPath);
  }
  return 0;
}

int OsRemoveDirectoryRecurse(LPCSTR pathName, DWORD flags) {
  FATALASSERT(pathName);
  char pathSlash[MAX_PATH];
  SStrCopy(pathSlash, pathName, sizeof(pathSlash));
  SStrPack(pathSlash, "\\", 0x7FFFFFFF);
  RemoveDirectoryRecurseData data = {pathSlash, flags};
  OsFileList(pathSlash, "*", EnumRemoveDirectoryRecurse, &data, 0);
  return OsRemoveDirectory(pathName);
}

int OsSetCurrentDirectory(LPCSTR pathName) {
  WORD dst[MAX_PATH];

  FATALASSERT(pathName);

  SUniConvertUTF8to16(dst, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return SetCurrentDirectoryW(reinterpret_cast<LPCWSTR>(dst));
}

int OsGetCurrentDirectory(DWORD pathLen, char *pathName) {
  FATALASSERT(pathName);
  WORD pathNameW[MAX_PATH];
  int  result = GetCurrentDirectoryW(MAX_PATH, reinterpret_cast<LPWSTR>(pathNameW));
  if (result) {
    UTF16ToUTF8(pathNameW, pathName, pathLen);
  }
  return result;
}

int OsFileAssocGetIdentifier(LPCSTR inFileExt, char *inBuffer, int inBufSize) {
  HKEY key;
  if (RegOpenKeyExA(HKEY_CLASSES_ROOT, inFileExt, 0, KEY_READ, &key)) {
    return 0;
  }
  DWORD type;
  DWORD bytesRead = inBufSize;
  long  result = RegQueryValueExA(key, "", 0, &type, reinterpret_cast<BYTE *>(inBuffer), &bytesRead);
  RegCloseKey(key);
  return type == REG_SZ && result == ERROR_SUCCESS;
}

void OsFileAssocSetIdentifier(LPCSTR inFileExt, LPCSTR inIdentifier) {
  HKEY key;
  if (!RegCreateKeyExA(HKEY_CLASSES_ROOT, inFileExt, 0, 0, 0, KEY_WRITE, 0, &key, 0)) {
    RegSetValueExA(key, "", 0, REG_SZ, reinterpret_cast<const BYTE *>(inIdentifier), SStrLen(inIdentifier) + 1);
    RegCloseKey(key);
  }
}

int OsFileAssocGetValue(LPCSTR inFileExt, int inAssocType, char *inBuffer, int inBufSize) {
  static LPCSTR sFileAssocKey[2] = {"", "\\shell\\open\\command"};
  FATALASSERT(inAssocType >= 0 && inAssocType < 2);

  char ident[MAX_PATH];
  char keyName[MAX_PATH];
  if (!OsFileAssocGetIdentifier(inFileExt, ident, sizeof(ident))) {
    return 0;
  }
  SStrCopy(keyName, ident, 0x7FFFFFFF);
  SStrPack(keyName, sFileAssocKey[inAssocType], 0x7FFFFFFF);

  HKEY key;
  if (RegOpenKeyExA(HKEY_CLASSES_ROOT, keyName, 0, KEY_READ, &key)) {
    return 0;
  }
  DWORD type;
  DWORD bytesRead = inBufSize;
  long  result = RegQueryValueExA(key, "", 0, &type, reinterpret_cast<BYTE *>(inBuffer), &bytesRead);
  RegCloseKey(key);
  return type == REG_SZ && result == ERROR_SUCCESS;
}

void OsFileAssocSetValue(LPCSTR inFileExt, int inAssocType, LPCSTR inValue) {
  static LPCSTR sFileAssocKey[2] = {"", "\\shell\\open\\command"};
  FATALASSERT(inAssocType >= 0 && inAssocType < 2);

  char ident[MAX_PATH];
  if (OsFileAssocGetIdentifier(inFileExt, ident, sizeof(ident))) {
    char keyName[MAX_PATH];
    SStrCopy(keyName, ident, 0x7FFFFFFF);
    SStrPack(keyName, sFileAssocKey[inAssocType], 0x7FFFFFFF);
    HKEY key;
    if (!RegCreateKeyExA(HKEY_CLASSES_ROOT, keyName, 0, 0, 0, KEY_WRITE, 0, &key, 0)) {
      RegSetValueExA(key, "", 0, REG_SZ, reinterpret_cast<const BYTE *>(inValue), SStrLen(inValue) + 1);
      RegCloseKey(key);
    }
  }
}

LONGLONG OsFileFreeSpace(LPCSTR path) {
  if (!path) {
    return 0;
  }

  char pathstr[MAX_PATH];
  SStrCopy(pathstr, path, sizeof(pathstr));
  char *slash = SStrChrR(pathstr, '\\');
  if (slash) {
    *slash = 0;
  }

  WORD           path16[MAX_PATH];
  ULARGE_INTEGER freeSpace;
  ULARGE_INTEGER totalBytes;
  freeSpace.QuadPart = 0;
  totalBytes.QuadPart = 0;
  SUniConvertUTF8to16(path16, MAX_PATH, pathstr, 0x7FFFFFFF, 0, 0);
  if (!GetDiskFreeSpaceExW(reinterpret_cast<LPCWSTR>(path16), &freeSpace, &totalBytes, 0)) {
    return 0;
  }
  return freeSpace.QuadPart;
}
