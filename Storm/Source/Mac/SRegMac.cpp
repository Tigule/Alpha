#include <storm.h>
#include <stpl.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <CoreServices/CoreServices.h>

#include <Os/W32/OsFile.h>

#define REGTYPE_DWORD  1
#define REGTYPE_STRING 2
#define REGTYPE_BINARY 3

#define REGSIGIL_DWORD  '#'
#define REGSIGIL_STRING '$'
#define REGSIGIL_BINARY '@'

struct RegistryEntry : TSHashObject<RegistryEntry, HASHKEY_STRI> {
  DWORD type;
  union {
    DWORD value;
    char *string;
  };
  DWORD size;

  void Clear() {
    if (type == REGTYPE_BINARY || type == REGTYPE_STRING) {
      if (string) {
        SMemFree(string, __FILE__, __LINE__, 0);
      }
    }

    type = 0;
    string = NULL;
    size = 0;
  }
};

struct RegistryFileEntry : TSHashObject<RegistryFileEntry, HASHKEY_STRI> {};

struct WriteRegistryEntry : TSHashObject<WriteRegistryEntry, HASHKEY_STRI> {
  FILE *file;
  char  path[0x400];
  int   written;
};

const char *k_localMachineSubdirectory = "/Blizzard";
const char *k_battleNetSubdirectory = "/Battle.net";

static TSHashTable<RegistryFileEntry, HASHKEY_STRI> s_fileTable;
static TSHashTable<RegistryEntry, HASHKEY_STRI>     s_entryTable;

static LPCSTR GetGlobalPreferencesDirectory() {
  static struct AutoGetPath {
    char path[0x400];

    AutoGetPath() {
      FSRef ref;
      OSErr err;
      UInt8 usersPath[0x400];

      path[0] = 0;

      err = FSFindFolder(kOnAppropriateDisk, kUsersFolderType, kCreateFolder, &ref);
      if (!(err == noErr)) {
        SErrDisplayErrorFmt(
            STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
            isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
                ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                : "\"%s\", %s = %ld (0x%08X)",
            "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
        );
      }

      if (err == noErr) {
        err = FSRefMakePath(&ref, usersPath, sizeof(usersPath));
        if (!(err == noErr)) {
          SErrDisplayErrorFmt(
              STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
              isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
                  ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                  : "\"%s\", %s = %ld (0x%08X)",
              "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
          );
        }

        if (err == noErr) {
          SStrPrintf(path, sizeof(path), "%s/Shared", usersPath);
          OsCreateDirectory(path, 1);
        }
      }
    }
  } s_autoGetPath;

  return s_autoGetPath.path;
}

static LPCSTR GetCurrentUserPreferencesDirectory() {
  static struct AutoGetPath {
    char path[0x400];

    AutoGetPath() {
      FSRef ref;
      OSErr err;

      path[0] = 0;

      err = FSFindFolder(kOnAppropriateDisk, kPreferencesFolderType, kCreateFolder, &ref);
      if (!(err == noErr)) {
        SErrDisplayErrorFmt(
            STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
            isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
                ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                : "\"%s\", %s = %ld (0x%08X)",
            "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
        );
      }

      if (err == noErr) {
        err = FSRefMakePath(&ref, (UInt8 *)path, sizeof(path));
        if (!(err == noErr)) {
          SErrDisplayErrorFmt(
              STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
              isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
                  ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
                  : "\"%s\", %s = %ld (0x%08X)",
              "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
          );
        }
      }
    }
  } s_autoGetPath;

  return s_autoGetPath.path;
}

static void CreateKey(LPCSTR keyname, LPCSTR valuename, DWORD flags, char *key, UINT keysize, char *path, UINT pathsize) {
  char   group[0x400];
  char  *separator;
  LPCSTR scope;
  LPCSTR directory;
  LPCSTR subdirectory;

  SStrCopy(group, keyname, sizeof(group));

  separator = SStrChr(group, '\\');
  if (separator) {
    *separator = 0;
  }

  if (flags & SREG_FLAG_BATTLENET) {
    scope = "Battle.net";
    directory = GetGlobalPreferencesDirectory();
    subdirectory = k_battleNetSubdirectory;
  } else if (flags & 4) {
    scope = "Local Machine";
    directory = GetGlobalPreferencesDirectory();
    subdirectory = k_localMachineSubdirectory;
  } else {
    scope = "Current User";
    directory = GetCurrentUserPreferencesDirectory();
    subdirectory = "";
  }

  SStrPrintf(key, keysize, "%s\\%s\\%s", scope, keyname, valuename);
  SStrPrintf(path, pathsize, "%s%s/com.blizzard.%s.prefs", directory, subdirectory, group);
}

static void ReadEscapedString(FILE *file, RegistryEntry *entry, bool terminate) {
  TSGrowableArray<char> data;
  int                   escaped = 0;
  int                   highNibble = 0;
  int                   pending = 0;

  for (;;) {
    int c = fgetc(file);

    if (c == EOF || c == '\n') {
      break;
    }

    if (c == '%') {
      if (escaped && !pending) {
        *data.New() = '%';
      }

      escaped = !escaped;
      highNibble = 1;
      pending = 0;
    } else if (escaped) {
      int digit = (c >= '0' && c <= '9') ? c - '0' : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : 0;

      pending = 1;

      if (highNibble) {
        *data.New() = (char)(16 * digit);
        highNibble = 0;
      } else {
        data[data.Count() - 1] |= digit;
        highNibble = 1;
      }
    } else {
      *data.New() = (char)c;
    }
  }

  if (terminate) {
    *data.New() = 0;
  }

  entry->size = data.Count();
  entry->string = (char *)SMemAlloc(data.Count(), __FILE__, __LINE__, 0);
  memcpy(entry->string, &data[0], data.Count());
}

static void WriteEscapedString(FILE *file, LPCVOID data, UINT size) {
  UINT index;

  for (index = 0; index < size; ++index) {
    BYTE byte = ((const BYTE *)data)[index];

    if (byte == '%') {
      fputc('%', file);
      fputc('%', file);
    } else if (byte < 0x20 || byte >= 0x7F) {
      fputc('%', file);
      fputc("0123456789ABCDEF"[byte >> 4], file);
      fputc("0123456789ABCDEF"[byte & 0xF], file);
      fputc('%', file);
    } else {
      fputc(byte, file);
    }
  }

  fputc('\n', file);
}

static void LoadRegistryFile(LPCSTR path) {
  FILE *file;
  char  sigil;
  char  key[0x400];

  if (s_fileTable.Ptr(path)) {
    return;
  }

  file = fopen(path, "r");
  if (file) {
    while (fscanf(file, "%c%[^\n]\n", &sigil, key) == 2) {
      RegistryEntry *entry = s_entryTable.Ptr(key);

      if (entry) {
        continue;
      }

      entry = s_entryTable.New(key, 0, 0);

      if (sigil == REGSIGIL_STRING) {
        entry->type = REGTYPE_STRING;
        ReadEscapedString(file, entry, true);
      } else if (sigil == REGSIGIL_BINARY) {
        entry->type = REGTYPE_BINARY;
        ReadEscapedString(file, entry, false);
      } else if (sigil == REGSIGIL_DWORD) {
        entry->type = REGTYPE_DWORD;
        fscanf(file, "%d\n", &entry->value);
      }
    }

    fclose(file);
  }

  s_fileTable.New(path, 0, 0);
}

static RegistryEntry *LoadRegistryEntry(LPCSTR key, LPCSTR path) {
  RegistryEntry *entry = s_entryTable.Ptr(key);

  if (entry) {
    return entry;
  }

  LoadRegistryFile(path);
  return s_entryTable.Ptr(key);
}

static RegistryEntry *CreateRegistryEntry(LPCSTR key, LPCSTR path) {
  RegistryEntry *entry = LoadRegistryEntry(key, path);

  if (entry) {
    entry->Clear();
    return entry;
  }

  return s_entryTable.New(key, 0, 0);
}

extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize) {
  RegistryEntry *entry;
  char           key[0x400];
  char           path[0x400];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(buffer);
  VALIDATE(buffersize);
  VALIDATEEND;

  CreateKey(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = LoadRegistryEntry(key, path);
  if (entry && entry->type == REGTYPE_STRING) {
    SStrCopy(buffer, entry->string, buffersize);
    return TRUE;
  }

  return FALSE;
}

extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value) {
  RegistryEntry *entry;
  char           key[0x400];
  char           path[0x400];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(value);
  VALIDATEEND;

  CreateKey(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = LoadRegistryEntry(key, path);
  if (entry && entry->type == REGTYPE_DWORD) {
    *value = entry->value;
    return TRUE;
  }

  return FALSE;
}

extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string) {
  RegistryEntry *entry;
  DWORD          bytes;
  char           key[0x400];
  char           path[0x400];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(string);
  VALIDATEEND;

  bytes = SStrLen(string) + 1;

  CreateKey(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = CreateRegistryEntry(key, path);
  entry->type = REGTYPE_STRING;
  entry->string = (char *)SMemAlloc(bytes, __FILE__, __LINE__, 0);
  entry->size = bytes;
  memcpy(entry->string, string, bytes);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value) {
  RegistryEntry *entry;
  char           key[0x400];
  char           path[0x400];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  CreateKey(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = CreateRegistryEntry(key, path);
  entry->type = REGTYPE_DWORD;
  entry->value = value;

  return TRUE;
}

extern "C" BOOL APIENTRY SRegDestroy() {
  TSHashTable<WriteRegistryEntry, HASHKEY_STRI> writeTable;
  RegistryEntry                                *entry;
  WriteRegistryEntry                           *writeEntry;

  while ((entry = s_entryTable.Head()) != NULL) {
    LPCSTR fullKey = entry->GetString();
    LPCSTR keyEnd;
    LPCSTR valueStart;
    char   group[0x400];

    valueStart = SStrChr(fullKey, '\\');
    keyEnd = valueStart ? SStrChr(valueStart + 1, '\\') : 0;

    if (!keyEnd) {
      s_entryTable.Delete(entry);
      continue;
    }

    SStrCopy(group, fullKey, keyEnd - fullKey + 1);

    writeEntry = writeTable.Ptr(group);
    if (!writeEntry) {
      LPCSTR directory;
      LPCSTR subdirectory;
      char   folder[0x400];

      if (group[0] == 'B') {
        directory = GetGlobalPreferencesDirectory();
        subdirectory = k_battleNetSubdirectory;
      } else if (group[0] == 'L') {
        directory = GetGlobalPreferencesDirectory();
        subdirectory = k_localMachineSubdirectory;
      } else if (group[0] == 'C') {
        directory = GetCurrentUserPreferencesDirectory();
        subdirectory = "";
      } else {
        s_entryTable.Delete(entry);
        continue;
      }

      writeEntry = writeTable.New(group, 0, 0);

      SStrPrintf(folder, sizeof(folder), "%s%s", directory, subdirectory);
      SStrPrintf(writeEntry->path, sizeof(writeEntry->path), "%s/com.blizzard.%s.prefs", folder, SStrChr(group, '\\') + 1);
      OsCreateDirectory(folder, 1);

      writeEntry->written = 0;
      writeEntry->file = fopen(writeEntry->path, "w");
    }

    if (entry->type && writeEntry->file) {
      writeEntry->written = 1;

      if (entry->type == REGTYPE_STRING) {
        fprintf(writeEntry->file, "%c%s\n", REGSIGIL_STRING, fullKey);
        WriteEscapedString(writeEntry->file, entry->string, SStrLen(entry->string));
      } else if (entry->type == REGTYPE_BINARY) {
        fprintf(writeEntry->file, "%c%s\n", REGSIGIL_BINARY, fullKey);
        WriteEscapedString(writeEntry->file, entry->string, entry->size);
      } else if (entry->type == REGTYPE_DWORD) {
        fprintf(writeEntry->file, "%c%s\n%d\n", REGSIGIL_DWORD, fullKey, entry->value);
      }
    }

    s_entryTable.Delete(entry);
  }

  while ((writeEntry = writeTable.Head()) != NULL) {
    if (writeEntry->file) {
      fclose(writeEntry->file);
    }

    if (!writeEntry->written) {
      unlink(writeEntry->path);
    }

    writeTable.Delete(writeEntry);
  }

  s_fileTable.Destroy();
  s_entryTable.Destroy();

  return TRUE;
}
