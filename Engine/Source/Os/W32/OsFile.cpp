#include <Base/Base.h>

#include "OsFile.h"

#include <malloc.h>
#include <storm.h>
#include <windows.h>

enum {
  OS_CREATE_NEW = 1,
  OS_TRUNCATE_EXISTING = 5
};

DWORD OsPathGetRootChars(LPCSTR path);
void  OsPathStripFilename(char *buffer);

static void UTF16ToUTF8(const WORD *src, char *dest, DWORD destLength) {
  DWORD stringLength;
  SUniConvertUTF16to8(dest, destLength, src, 0x7FFFFFFF, &stringLength, 0);
  dest[min(destLength - 1, stringLength)] = 0;
}

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType) {
  WORD fileName16[MAX_PATH];

  ASSERT(fileName);
  if (!fileName) {
    return (HOSFILE)INVALID_HANDLE_VALUE;
  }

  ASSERT(desiredAccess);
  if (!desiredAccess) {
    return (HOSFILE)INVALID_HANDLE_VALUE;
  }

  ASSERT(createDisposition >= OS_CREATE_NEW);
  ASSERT(createDisposition <= OS_TRUNCATE_EXISTING);

  if (createDisposition < OS_CREATE_NEW || createDisposition > OS_TRUNCATE_EXISTING) {
    return (HOSFILE)INVALID_HANDLE_VALUE;
  }

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);

  (void)extendedFileType;
  return (HOSFILE)CreateFileW(fileName16, desiredAccess, shareMode, 0, createDisposition, flagsAndAttributes, 0);
}

void OsCloseFile(HOSFILE fileHandle) {
  CloseHandle(fileHandle);
}

BOOL OsFileExists(LPCSTR path) {
  DWORD attributes;

  if (!path || !path[0]) {
    return 0;
  }

  attributes = OsGetFileAttributes(path);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

BOOL OsDirectoryExists(LPCSTR dirName) {
  if (!dirName || !dirName[0]) {
    return 0;
  }

  DWORD attributes = OsGetFileAttributes(dirName);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

BOOL OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead) {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bytesRead);
  VALIDATEEND;

  return ReadFile(fileHandle, buffer, bytesToRead, bytesRead, 0);
}

BOOL OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten) {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bytesWritten);
  VALIDATEEND;

  return WriteFile(fileHandle, buffer, bytesToWrite, bytesWritten, 0);
}

int OsFlushFile(HOSFILE__ *fileHandle) {
  return FlushFileBuffers(fileHandle);
}

DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod) {
  LARGE_INTEGER distance;

  distance.QuadPart = distanceToMove;
  distance.LowPart = SetFilePointer(fileHandle, distance.LowPart, &distance.HighPart, moveMethod);

  if (distance.LowPart == 0xFFFFFFFF && GetLastError()) {
    return (DWORDLONG)-1;
  }

  return distance.QuadPart;
}

DWORDLONG OsGetFileSize(HOSFILE__ *fileHandle) {
  LARGE_INTEGER size;
  size.LowPart = GetFileSize(fileHandle, (DWORD *)&size.HighPart);
  return size.QuadPart;
}

int OsGetFileTime(HOSFILE__ *fileHandle, OSFILETIME *createFileTime, OSFILETIME *accessFileTime, OSFILETIME *writeFileTime) {
  return GetFileTime(
      fileHandle, (FILETIME *)createFileTime, (FILETIME *)accessFileTime,
      (FILETIME *)writeFileTime
  );
}

int OsSetFileTime(HOSFILE__ *fileHandle, const OSFILETIME *createFileTime, const OSFILETIME *accessFileTime, const OSFILETIME *writeFileTime) {
  return SetFileTime(
      fileHandle, (const FILETIME *)createFileTime, (const FILETIME *)accessFileTime,
      (const FILETIME *)writeFileTime
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
  return SetEndOfFile(fileHandle);
}

DWORD OsGetFileAttributes(LPCSTR fileName) {
  WORD fileName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return GetFileAttributesW(fileName16);
}

int OsSetFileAttributes(LPCSTR fileName, DWORD attributes) {
  WORD fileName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return SetFileAttributesW(fileName16, attributes);
}

BOOL OsMoveFile(LPCSTR existingFileName, LPCSTR newFileName) {
  WORD existingFileName16[MAX_PATH];
  WORD newFileName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(existingFileName);
  VALIDATE(newFileName);
  VALIDATEEND;

  SUniConvertUTF8to16(newFileName16, MAX_PATH, newFileName, 0x7FFFFFFF, 0, 0);
  SUniConvertUTF8to16(existingFileName16, MAX_PATH, existingFileName, 0x7FFFFFFF, 0, 0);
  return MoveFileW(existingFileName16, newFileName16);
}

int OsCopyFile(LPCSTR existingFileName, LPCSTR newFileName, int failIfExists) {
  WORD existingFileName16[MAX_PATH];
  WORD newFileName16[MAX_PATH];
  SUniConvertUTF8to16(newFileName16, MAX_PATH, newFileName, 0x7FFFFFFF, 0, 0);
  SUniConvertUTF8to16(existingFileName16, MAX_PATH, existingFileName, 0x7FFFFFFF, 0, 0);
  return CopyFileW(existingFileName16, newFileName16, failIfExists);
}

BOOL OsDeleteFile(LPCSTR fileName) {
  WORD fileName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(fileName);
  VALIDATEEND;

  SUniConvertUTF8to16(fileName16, MAX_PATH, fileName, 0x7FFFFFFF, 0, 0);
  return DeleteFileW(fileName16);
}

BOOL OsCreateDirectory(LPCSTR pathName, int recursive) {
  char tempName[MAX_PATH];
  WORD pathName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(pathName);
  VALIDATEEND;

  if (recursive) {
    SStrCopy(tempName, pathName, MAX_PATH);

    char *slash = SStrChr(tempName + OsPathGetRootChars(tempName), '\\');
    while (slash) {
      *slash = 0;
      SUniConvertUTF8to16(pathName16, MAX_PATH, tempName, 0x7FFFFFFF, 0, 0);
      CreateDirectoryW(pathName16, 0);
      *slash = '\\';
      slash = SStrChr(slash + 1, '\\');
    }
  }

  SUniConvertUTF8to16(pathName16, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return CreateDirectoryW(pathName16, 0);
}

int OsRemoveDirectory(LPCSTR pathName) {
  WORD pathName16[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(pathName);
  VALIDATEEND;

  SUniConvertUTF8to16(pathName16, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return RemoveDirectoryW(pathName16);
}

struct RemoveDirectoryRecurseData {
  LPCSTR path;
  DWORD  flags;
};

BOOL OsFileList(LPCSTR inDir, LPCSTR inPattern, int (*inCallback)(OS_FILE_DATA &, LPVOID), LPVOID inCBParam, int returnHidden) {
  char             findPath[MAX_PATH];
  WIN32_FIND_DATAW findData;
  HANDLE           findHandle;

  SStrCopy(findPath, inDir, 0x7FFFFFFF);
  OsPathStripFilename(findPath);
  SStrPack(findPath, inPattern, 0x7FFFFFFF);
  {
    WORD findPath16[MAX_PATH];
    SUniConvertUTF8to16(findPath16, MAX_PATH, findPath, 0x7FFFFFFF, 0, 0);
    findHandle = FindFirstFileW(findPath16, &findData);
  }
  int result = 0;
  if (findHandle != INVALID_HANDLE_VALUE) {
    OS_FILE_DATA osfData;
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

static BOOL EnumRemoveDirectoryRecurse(OS_FILE_DATA &file, LPVOID param) {
  RemoveDirectoryRecurseData *data = (RemoveDirectoryRecurseData *)param;
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
  VALIDATEBEGIN;
  VALIDATE(pathName);
  VALIDATEEND;
  char pathSlash[MAX_PATH];
  SStrCopy(pathSlash, pathName, sizeof(pathSlash));
  SStrPack(pathSlash, "\\", 0x7FFFFFFF);
  RemoveDirectoryRecurseData data = {pathSlash, flags};
  OsFileList(pathSlash, "*", EnumRemoveDirectoryRecurse, &data, 0);
  return OsRemoveDirectory(pathName);
}

BOOL OsSetCurrentDirectory(LPCSTR pathName) {
  WORD dst[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(pathName);
  VALIDATEEND;

  SUniConvertUTF8to16(dst, MAX_PATH, pathName, 0x7FFFFFFF, 0, 0);
  return SetCurrentDirectoryW(dst);
}

BOOL OsGetCurrentDirectory(DWORD pathLen, char *pathName) {
  WORD pathNameW[MAX_PATH];

  VALIDATEBEGIN;
  VALIDATE(pathName);
  VALIDATEEND;

  if (!GetCurrentDirectoryW(pathLen, pathNameW)) {
    return 0;
  }

  UTF16ToUTF8(pathNameW, pathName, pathLen);
  return 1;
}

BOOL OsFileAssocGetIdentifier(LPCSTR inFileExt, char *inBuffer, int inBufSize) {
  HKEY key;
  if (RegOpenKeyExA(HKEY_CLASSES_ROOT, inFileExt, 0, KEY_READ, &key)) {
    return 0;
  }
  DWORD type;
  DWORD bytesRead = inBufSize;
  long  result = RegQueryValueExA(key, "", 0, &type, (BYTE *)inBuffer, &bytesRead);
  RegCloseKey(key);
  if (type != REG_SZ) {
    return 0;
  }
  return result == ERROR_SUCCESS;
}

void OsFileAssocSetIdentifier(LPCSTR inFileExt, LPCSTR inIdentifier) {
  HKEY key;
  if (!RegCreateKeyExA(HKEY_CLASSES_ROOT, inFileExt, 0, 0, 0, KEY_WRITE, 0, &key, 0)) {
    RegSetValueExA(key, "", 0, REG_SZ, (const BYTE *)inIdentifier, SStrLen(inIdentifier) + 1);
    RegCloseKey(key);
  }
}

static LPCSTR const sFileAssocKey[NUM_OSFILE_ASSOC] = {"", "\\shell\\open\\command"};

BOOL OsFileAssocGetValue(LPCSTR inFileExt, int inAssocType, char *inBuffer, int inBufSize) {
  VALIDATEBEGIN;
  VALIDATE(inAssocType >= 0 && inAssocType < NUM_OSFILE_ASSOC);
  VALIDATEEND;

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
  long  result = RegQueryValueExA(key, "", 0, &type, (BYTE *)inBuffer, &bytesRead);
  RegCloseKey(key);
  if (type != REG_SZ) {
    return 0;
  }
  return result == ERROR_SUCCESS;
}

void OsFileAssocSetValue(LPCSTR inFileExt, int inAssocType, LPCSTR inValue) {
  VALIDATEBEGIN;
  VALIDATE(inAssocType >= 0 && inAssocType < NUM_OSFILE_ASSOC);
  VALIDATEENDVOID;

  char ident[MAX_PATH];
  if (OsFileAssocGetIdentifier(inFileExt, ident, sizeof(ident))) {
    char keyName[MAX_PATH];
    SStrCopy(keyName, ident, 0x7FFFFFFF);
    SStrPack(keyName, sFileAssocKey[inAssocType], 0x7FFFFFFF);
    HKEY key;
    if (!RegCreateKeyExA(HKEY_CLASSES_ROOT, keyName, 0, 0, 0, KEY_WRITE, 0, &key, 0)) {
      RegSetValueExA(key, "", 0, REG_SZ, (const BYTE *)inValue, SStrLen(inValue) + 1);
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
  if (!GetDiskFreeSpaceExW(path16, &freeSpace, &totalBytes, 0)) {
    return 0;
  }
  return freeSpace.QuadPart;
}
