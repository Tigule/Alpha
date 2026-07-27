#include <storm.h>

#define SREG_FLAG_HKLM    0x04
#define SREG_FLAG_NO_BASE 0x10

#define SREG_ERROR_CANTOPEN 0x3F3

#define BASEKEY      "Software\\Blizzard Entertainment\\"
#define BATTLENETKEY "Software\\Battle.net\\"

#undef FATALASSERT
#define FATALASSERT(a)                       \
  if (!(a)) {                                \
    SErrPrepareAppFatal(__FILE__, __LINE__); \
    SErrDisplayAppFatal(#a);                 \
  }

static void BuildFullKeyName(LPCSTR keyname, UINT flags, char *buffer, UINT bufferChars) {
  buffer[0] = 0;
  if (!(flags & SREG_FLAG_NO_BASE)) {
    SRegGetBaseKey(flags, buffer, bufferChars);
  }

  SStrPack(buffer, keyname, bufferChars);
}

static LONG IDeleteValue(HKEY parentKey, LPCSTR subKeyName, LPCSTR valueName) {
  char  currentSubKey[MAX_PATH];
  char *slash;
  DWORD subKeyCount;
  HKEY  key;
  DWORD valueCount;
  LONG  status;

  status = RegOpenKeyExA(parentKey, subKeyName, 0, KEY_READ | KEY_SET_VALUE, &key);
  if (status != ERROR_SUCCESS) {
    return status;
  }

  status = RegDeleteValueA(key, valueName);
  if (status == ERROR_SUCCESS) {
    SStrCopy(currentSubKey, subKeyName, sizeof(currentSubKey));
    status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeyCount, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);
    while (status == ERROR_SUCCESS) {
      if (subKeyCount || valueCount) {
        break;
      }
      status = RegDeleteKeyA(parentKey, currentSubKey);
      if (status != ERROR_SUCCESS) {
        break;
      }
      RegCloseKey(key);
      slash = SStrChrR(currentSubKey, '\\');
      if (!slash) {
        break;
      }
      *slash = 0;
      status = RegOpenKeyExA(parentKey, currentSubKey, 0, KEY_READ | KEY_SET_VALUE, &key);
      if (status != ERROR_SUCCESS) {
        return status;
      }
      status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeyCount, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);
    }
  }
  RegCloseKey(key);
  return status;
}

static LONG IDeleteKey(HKEY parentKey, LPCSTR subKeyName) {
  HKEY key;
  LONG status;

  status = RegOpenKeyExA(parentKey, NULL, 0, 0x30019, &key);
  if (status != ERROR_SUCCESS) {
    return status;
  }

  status = RegDeleteKeyA(key, subKeyName);
  RegCloseKey(key);
  return status;
}

static LONG
ILoadValue(HKEY parentKey, LPCSTR subKeyName, LPCSTR valuename, LPDWORD datatype, LPBYTE buffer, DWORD bytes, LPDWORD bytesread) {
  HKEY key;
  LONG status;

  status = RegOpenKeyExA(parentKey, subKeyName, 0, KEY_READ, &key);
  if (status != ERROR_SUCCESS) {
    return status;
  }
  *bytesread = bytes;
  status = RegQueryValueExA(key, valuename, NULL, datatype, buffer, bytesread);
  RegCloseKey(key);
  return status;
}

static BOOL InternalDeleteEntry(LPCSTR keyname, LPCSTR valuename, UINT flags) {
  char fullkeyname[MAX_PATH];
  BOOL success;
  LONG hkcuStatus;
  LONG hklmStatus;
  LONG status;

  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  success = FALSE;
  hkcuStatus = ERROR_SUCCESS;
  hklmStatus = ERROR_SUCCESS;

  if (!(flags & SREG_FLAG_HKLM)) {
    hkcuStatus = IDeleteValue(HKEY_CURRENT_USER, fullkeyname, valuename);
    if (hkcuStatus == ERROR_SUCCESS) {
      success = TRUE;
    }
  }

  if (!(flags & SREG_FLAG_USERSPECIFIC)) {
    hklmStatus = IDeleteValue(HKEY_LOCAL_MACHINE, fullkeyname, valuename);
    if (hklmStatus == ERROR_SUCCESS) {
      success = TRUE;
    }
  }

  if (success) {
    return TRUE;
  }

  status = hkcuStatus != ERROR_SUCCESS ? hkcuStatus : hklmStatus;
  SetLastError((DWORD)status);
  return FALSE;
}

static BOOL InternalDeleteKey(LPCSTR keyname, UINT flags) {
  char fullkeyname[MAX_PATH];
  BOOL deleted;
  LONG hkcuStatus;
  LONG hklmStatus;
  LONG status;

  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  deleted = FALSE;
  hkcuStatus = ERROR_SUCCESS;
  hklmStatus = ERROR_SUCCESS;

  if (!(flags & SREG_FLAG_HKLM)) {
    hkcuStatus = IDeleteKey(HKEY_CURRENT_USER, fullkeyname);
    if (hkcuStatus == ERROR_SUCCESS) {
      deleted = TRUE;
    }
  }

  if (!(flags & SREG_FLAG_USERSPECIFIC)) {
    hklmStatus = IDeleteKey(HKEY_LOCAL_MACHINE, fullkeyname);
    if (hklmStatus == ERROR_SUCCESS) {
      deleted = TRUE;
    }
  }

  if (deleted) {
    return TRUE;
  }

  status = hkcuStatus != ERROR_SUCCESS ? hkcuStatus : hklmStatus;
  SetLastError((DWORD)status);
  return FALSE;
}

static BOOL
InternalLoadEntry(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD datatype, LPVOID buffer, DWORD bytes, LPDWORD bytesread) {
  char fullkeyname[MAX_PATH];
  LONG status;

  *bytesread = 0;
  *datatype = REG_DWORD;
  status = SREG_ERROR_CANTOPEN;

  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  if (!(flags & SREG_FLAG_HKLM)) {
    status = ILoadValue(HKEY_CURRENT_USER, fullkeyname, valuename, datatype, (LPBYTE)buffer, bytes, bytesread);
  }

  if (status != ERROR_SUCCESS && !(flags & SREG_FLAG_USERSPECIFIC)) {
    status = ILoadValue(HKEY_LOCAL_MACHINE, fullkeyname, valuename, datatype, (LPBYTE)buffer, bytes, bytesread);
  }

  if (status == ERROR_SUCCESS) {
    return TRUE;
  }

  SetLastError((DWORD)status);
  return FALSE;
}

static BOOL InternalSaveEntry(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD datatype, LPCVOID buffer, DWORD bytes) {
  char  fullkeyname[MAX_PATH];
  DWORD disposition;
  HKEY  key;
  LONG  status;

  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  status = RegCreateKeyExA(
      (flags & SREG_FLAG_HKLM) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, fullkeyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key,
      &disposition
  );
  if (status == ERROR_SUCCESS) {
    status = RegSetValueExA(key, valuename, 0, datatype, (const BYTE *)buffer, bytes);
    if (status == ERROR_SUCCESS && (flags & SREG_FLAG_FLUSHTODISK)) {
      status = RegFlushKey(key);
    }
    if (status == ERROR_SUCCESS) {
      status = RegCloseKey(key);
      if (status == ERROR_SUCCESS) {
        return TRUE;
      }
    }
    RegCloseKey(key);
  }

  SetLastError((DWORD)status);
  return FALSE;
}

extern "C" BOOL APIENTRY SRegDeleteValue(LPCSTR keyname, LPCSTR valuename, UINT flags) {
  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  return InternalDeleteEntry(keyname, valuename, flags);
}

extern "C" BOOL APIENTRY SRegDeleteKey(LPCSTR keyname, UINT flags) {
  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  return InternalDeleteKey(keyname, flags);
}

extern "C" BOOL APIENTRY SRegGetBaseKey(UINT flags, char *buffer, DWORD bufferChars) {
  FATALASSERT(buffer);
  FATALASSERT(bufferChars);

  if (flags & SREG_FLAG_BATTLENET) {
    SStrCopy(buffer, BATTLENETKEY, bufferChars);
  } else {
    SStrCopy(buffer, BASEKEY, bufferChars);
  }
  return TRUE;
}

extern "C" BOOL APIENTRY SRegLoadData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPVOID buffer, DWORD buffersize, LPDWORD bytesread) {
  DWORD localbytesread;
  DWORD datatype;

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);

  if (!bytesread) {
    bytesread = &localbytesread;
  }
  return InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, buffersize, bytesread);
}

extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize) {
  DWORD datatype;
  DWORD bytesread;

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(buffer);
  FATALASSERT(buffersize);

  if (!InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, buffersize, &bytesread)) {
    return FALSE;
  }

  if (datatype == REG_SZ) {
    buffer[bytesread < buffersize ? bytesread : buffersize - 1] = 0;
  } else if (datatype == REG_DWORD) {
    SStrPrintf(buffer, buffersize, "%u", *(DWORD *)buffer);
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value) {
  char  buffer[256];
  DWORD datatype;
  DWORD bytesread;

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(value);

  buffer[0] = 0;
  if (!InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, sizeof(buffer), &bytesread)) {
    return FALSE;
  }

  if (datatype == REG_DWORD) {
    *value = *(DWORD *)buffer;
  } else if (datatype == REG_SZ) {
    *value = strtoul(buffer, NULL, 0);
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCVOID data, DWORD databytes) {
  DWORD type;

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);

  if (!data) {
    data = "";
  }

  type = (flags & SREG_FLAG_MULTISZ) ? REG_MULTI_SZ : REG_BINARY;
  return InternalSaveEntry(keyname, valuename, flags, type, data, databytes);
}

extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string) {
  DWORD bytes;

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(string);

  bytes = SStrLen(string) + 1;
  return InternalSaveEntry(keyname, valuename, flags, REG_SZ, string, bytes);
}

extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value) {
  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);

  return InternalSaveEntry(keyname, valuename, flags, REG_DWORD, &value, sizeof(value));
}

extern "C" BOOL APIENTRY SRegEnumKey(LPCSTR baseKeyName, UINT flags, UINT subKeyIndex, char *keyNameBuffer, UINT bufferChars) {
  char     keyName[MAX_PATH];
  char     fullBaseKeyName[MAX_PATH];
  FILETIME lastWriteTime;
  HKEY     baseKey;
  DWORD    keyNameSize;
  LONG     status;

  FATALASSERT(baseKeyName);
  FATALASSERT(*baseKeyName);
  FATALASSERT(keyNameBuffer);
  FATALASSERT(bufferChars);

  BuildFullKeyName(baseKeyName, flags, fullBaseKeyName, sizeof(fullBaseKeyName));

  status = RegOpenKeyExA((flags & SREG_FLAG_HKLM) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, fullBaseKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &baseKey);
  if (status != ERROR_SUCCESS) {
    SetLastError((DWORD)status);
    return FALSE;
  }

  keyNameSize = sizeof(keyName);
  status = RegEnumKeyExA(baseKey, subKeyIndex, keyName, &keyNameSize, NULL, NULL, NULL, &lastWriteTime);
  RegCloseKey(baseKey);

  if (status != ERROR_SUCCESS) {
    SetLastError((DWORD)status);
    return FALSE;
  }

  SStrPrintf(keyNameBuffer, bufferChars, "%s\\%s", baseKeyName, keyName);
  return TRUE;
}

extern "C" BOOL APIENTRY SRegGetNumSubKeys(LPCSTR keyName, UINT flags, UINT *numSubKeys) {
  char  fullKeyName[MAX_PATH];
  HKEY  key;
  DWORD subKeys;
  LONG  status;

  FATALASSERT(keyName);
  FATALASSERT(numSubKeys);

  *numSubKeys = 0;
  BuildFullKeyName(keyName, flags, fullKeyName, sizeof(fullKeyName));

  status = RegOpenKeyExA((flags & SREG_FLAG_HKLM) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, fullKeyName, 0, KEY_QUERY_VALUE, &key);
  if (status != ERROR_SUCCESS) {
    SetLastError((DWORD)status);
    return FALSE;
  }

  subKeys = 0;
  status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  RegCloseKey(key);

  if (status == ERROR_SUCCESS) {
    *numSubKeys = subKeys;
  }

  if (status != ERROR_SUCCESS) {
    SetLastError((DWORD)status);
  }
  return status == ERROR_SUCCESS;
}
