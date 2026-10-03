#include <storm.h>

#define SREG_FLAG_HKLM    0x04
#define SREG_FLAG_NO_BASE 0x10

#define SREG_ERROR_CANTOPEN 0x3F3

#define BASEKEY      "Software\\Blizzard Entertainment\\"
#define BATTLENETKEY "Software\\Battle.net\\"

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
  LONG  status;

  status = RegOpenKeyExA(parentKey, subKeyName, 0, KEY_READ | KEY_SET_VALUE, &key);
  if (status != ERROR_SUCCESS) {
    return status;
  }

  status = RegDeleteValueA(key, valueName);
  if (status == ERROR_SUCCESS) {
    SStrCopy(currentSubKey, subKeyName, sizeof(currentSubKey));
    status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeyCount, NULL, NULL, reinterpret_cast<LPDWORD>(&valueName), NULL, NULL, NULL, NULL);
    while (status == ERROR_SUCCESS) {
      if (subKeyCount || valueName) {
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
      status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeyCount, NULL, NULL, reinterpret_cast<LPDWORD>(&valueName), NULL, NULL, NULL, NULL);
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

static LONG ILoadValue(HKEY parentKey, LPCSTR subKeyName, LPCSTR valuename, LPDWORD datatype, LPBYTE buffer, DWORD bytes, LPDWORD bytesread) {
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

  success = FALSE;
  hkcuStatus = ERROR_SUCCESS;
  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  if (!(flags & SREG_FLAG_HKLM)) {
    hkcuStatus = IDeleteValue(HKEY_CURRENT_USER, fullkeyname, valuename);
    if (hkcuStatus == ERROR_SUCCESS) {
      success = TRUE;
    }
  }

  hklmStatus = ERROR_SUCCESS;
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

  deleted = FALSE;
  BuildFullKeyName(keyname, flags, fullkeyname, sizeof(fullkeyname));

  hkcuStatus = ERROR_SUCCESS;
  if (!(flags & SREG_FLAG_HKLM)) {
    hkcuStatus = IDeleteKey(HKEY_CURRENT_USER, fullkeyname);
    if (hkcuStatus == ERROR_SUCCESS) {
      deleted = TRUE;
    }
  }

  hklmStatus = ERROR_SUCCESS;
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

static BOOL InternalLoadEntry(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD datatype, LPVOID buffer, DWORD bytes, LPDWORD bytesread) {
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
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  return InternalDeleteEntry(keyname, valuename, flags);
}

extern "C" BOOL APIENTRY SRegDeleteKey(LPCSTR keyname, UINT flags) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATEEND;

  return InternalDeleteKey(keyname, flags);
}

extern "C" BOOL APIENTRY SRegGetBaseKey(UINT flags, char *buffer, UINT bufferChars) {
  VALIDATEBEGIN;
  VALIDATE(buffer);
  VALIDATE(bufferChars);
  VALIDATEEND;

  if (flags & SREG_FLAG_BATTLENET) {
    SStrCopy(buffer, BATTLENETKEY, bufferChars);
  } else {
    SStrCopy(buffer, BASEKEY, bufferChars);
  }
  return TRUE;
}

extern "C" BOOL APIENTRY SRegLoadData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPVOID buffer, DWORD buffersize, LPDWORD bytesread) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  DWORD datatype;
  DWORD localbytesread;
  return InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, buffersize, bytesread ? bytesread : &localbytesread);
}

extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(buffer);
  VALIDATE(buffersize);
  VALIDATEEND;

  DWORD datatype;
  DWORD bytesread;
  if (!InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, buffersize, &bytesread)) {
    return FALSE;
  }

  switch (datatype) {
    case REG_DWORD:
      SStrPrintf(buffer, buffersize, "%u", *(DWORD *)buffer);
      break;
    case REG_SZ:
      buffer[min(buffersize - 1, bytesread)] = 0;
      break;
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(value);
  VALIDATEEND;

  char  buffer[256];
  DWORD datatype;
  DWORD bytesread;
  buffer[0] = 0;
  if (!InternalLoadEntry(keyname, valuename, flags, &datatype, buffer, sizeof(buffer), &bytesread)) {
    return FALSE;
  }

  switch (datatype) {
    case REG_DWORD:
      *value = *(DWORD *)buffer;
      break;
    case REG_SZ:
      *value = strtoul(buffer, NULL, 0);
      break;
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCVOID data, DWORD databytes) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  if (!data) {
    data = "";
  }

  return InternalSaveEntry(keyname, valuename, flags, (flags & SREG_FLAG_MULTISZ) ? REG_MULTI_SZ : REG_BINARY, data, databytes);
}

extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(string);
  VALIDATEEND;

  return InternalSaveEntry(keyname, valuename, flags, REG_SZ, string, SStrLen(string) + 1);
}

extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value) {
  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  return InternalSaveEntry(keyname, valuename, flags, REG_DWORD, &value, sizeof(value));
}

extern "C" BOOL APIENTRY SRegEnumKey(LPCSTR baseKeyName, UINT flags, UINT subKeyIndex, char *keyNameBuffer, UINT bufferChars) {
  VALIDATEBEGIN;
  VALIDATE(baseKeyName);
  VALIDATE(*baseKeyName);
  VALIDATE(keyNameBuffer);
  VALIDATE(bufferChars);
  VALIDATEEND;

  char fullBaseKeyName[MAX_PATH];
  BuildFullKeyName(baseKeyName, flags, fullBaseKeyName, sizeof(fullBaseKeyName));

  HKEY baseKey;
  LONG status = RegOpenKeyExA((flags & SREG_FLAG_HKLM) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, fullBaseKeyName, 0, KEY_ENUMERATE_SUB_KEYS, &baseKey);
  if (status != ERROR_SUCCESS) {
    SetLastError(status);
    return FALSE;
  }

  char     keyName[MAX_PATH];
  DWORD    keyNameSize = sizeof(keyName);
  FILETIME lastWriteTime;
  status = RegEnumKeyExA(baseKey, subKeyIndex, keyName, &keyNameSize, NULL, NULL, NULL, &lastWriteTime);
  RegCloseKey(baseKey);
  if (status != ERROR_SUCCESS) {
    SetLastError(status);
    return FALSE;
  }

  SStrPrintf(keyNameBuffer, bufferChars, "%s\\%s", baseKeyName, keyName);
  return TRUE;
}

extern "C" BOOL APIENTRY SRegGetNumSubKeys(LPCSTR keyName, UINT flags, UINT *numSubKeys) {
  VALIDATEBEGIN;
  VALIDATE(keyName);
  VALIDATEANDBLANK(numSubKeys);
  VALIDATEEND;

  char fullKeyName[MAX_PATH];
  BuildFullKeyName(keyName, flags, fullKeyName, sizeof(fullKeyName));

  HKEY key;
  LONG status = RegOpenKeyExA((flags & SREG_FLAG_HKLM) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, fullKeyName, 0, KEY_QUERY_VALUE, &key);
  if (status != ERROR_SUCCESS) {
    SetLastError(status);
    return FALSE;
  }

  DWORD subKeys;
  status = RegQueryInfoKeyA(key, NULL, NULL, NULL, &subKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  RegCloseKey(key);
  if (status != ERROR_SUCCESS) {
    SetLastError(status);
    return FALSE;
  }

  *numSubKeys = subKeys;
  return TRUE;
}
