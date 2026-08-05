#include <storm.h>
#include <stpl.h>

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
    DWORD  value;
    char  *string;
  };
  DWORD size;
};

struct RegistryFileEntry : TSHashObject<RegistryFileEntry, HASHKEY_STRI> {
};

struct WriteRegistryEntry : TSHashObject<WriteRegistryEntry, HASHKEY_STRI> {
  FILE *file;
  char  path[0x400];
  int   written;
};

static const char k_localMachineSubdirectory[] = "";
static const char k_battleNetSubdirectory[] = "";

static TSHashTable<RegistryFileEntry, HASHKEY_STRI> s_fileTable;
static TSHashTable<RegistryEntry, HASHKEY_STRI>     s_entryTable;

static const char *AutoGetUserPath() {
  static char s_path[0x400];
  static int  s_valid;

  if (!s_valid) {
    FSRef ref;
    OSErr err;

    s_path[0] = 0;

    err = FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &ref);
    ASSERT(err == noErr);

    if (!err) {
      OSStatus status = FSRefMakePath(&ref, reinterpret_cast<UInt8 *>(s_path), sizeof(s_path));
      ASSERT(status == noErr);
    }

    s_valid = 1;
  }

  return s_path;
}

static const char *AutoGetSharedPath() {
  static char s_path[0x400];
  static int  s_valid;

  if (!s_valid) {
    FSRef ref;
    OSErr err;
    UInt8 usersPath[0x400];

    s_path[0] = 0;

    err = FSFindFolder(kUserDomain, kUsersFolderType, kCreateFolder, &ref);
    ASSERT(err == noErr);

    if (!err) {
      OSStatus status = FSRefMakePath(&ref, usersPath, sizeof(usersPath));
      ASSERT(status == noErr);

      if (!status) {
        SStrPrintf(s_path, sizeof(s_path), "%s/Shared", usersPath);
        OsCreateDirectory(s_path, 1);
      }
    }

    s_valid = 1;
  }

  return s_path;
}

static void BuildKeyAndPath(
    const char *keyname,
    const char *valuename,
    DWORD       flags,
    char       *key,
    DWORD       keysize,
    char       *path,
    DWORD       pathsize
) {
  char        group[0x400];
  char       *separator;
  const char *scope;
  const char *directory;
  const char *subdirectory;

  SStrCopy(group, keyname, sizeof(group));

  separator = SStrChr(group, '\\');
  if (separator) {
    *separator = 0;
  }

  if (flags & SREG_FLAG_BATTLENET) {
    scope = "Battle.net";
    directory = AutoGetSharedPath();
    subdirectory = k_battleNetSubdirectory;
  } else {
    scope = "Current User";
    directory = AutoGetUserPath();
    subdirectory = "";
  }

  SStrPrintf(key, keysize, "%s\\%s\\%s", scope, keyname, valuename);
  SStrPrintf(path, pathsize, "%s%s/com.blizzard.%s.prefs", directory, subdirectory, group);
}

// Values are stored one per pair of lines.  Non printable bytes are escaped as
// %XX hex pairs, and a literal percent is written as %%.
static int HexDigit(int c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }

  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }

  return 0;
}

static void ReadEscapedValue(FILE *file, RegistryEntry *entry, int terminate) {
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
      pending = 1;

      if (highNibble) {
        *data.New() = static_cast<char>(16 * HexDigit(c));
        highNibble = 0;
      } else {
        data[data.Count() - 1] |= HexDigit(c);
        highNibble = 1;
      }
    } else {
      *data.New() = static_cast<char>(c);
    }
  }

  if (terminate) {
    *data.New() = 0;
  }

  entry->size = data.Count();
  entry->string = static_cast<char *>(SMemAlloc(data.Count(), __FILE__, __LINE__, 0));
  memcpy(entry->string, &data[0], data.Count());
}

static void WriteEscapedValue(FILE *file, const char *data, DWORD size) {
  DWORD index;

  for (index = 0; index < size; ++index) {
    unsigned char byte = static_cast<unsigned char>(data[index]);

    if (byte == '%') {
      fprintf(file, "%%%%");
    } else if (byte < 0x20 || byte >= 0x7F) {
      fprintf(file, "%%%02X%%", byte);
    } else {
      fputc(byte, file);
    }
  }

  fputc('\n', file);
}

static void LoadRegistryFile(const char *path) {
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
        ReadEscapedValue(file, entry, 1);
      } else if (sigil == REGSIGIL_BINARY) {
        entry->type = REGTYPE_BINARY;
        ReadEscapedValue(file, entry, 0);
      } else if (sigil == REGSIGIL_DWORD) {
        entry->type = REGTYPE_DWORD;
        fscanf(file, "%d\n", &entry->value);
      }
    }

    fclose(file);
  }

  s_fileTable.New(path, 0, 0);
}

static RegistryEntry *FindEntry(const char *key, const char *path) {
  RegistryEntry *entry = s_entryTable.Ptr(key);

  if (entry) {
    return entry;
  }

  LoadRegistryFile(path);
  return s_entryTable.Ptr(key);
}

static RegistryEntry *CreateEntry(const char *key, const char *path) {
  RegistryEntry *entry = FindEntry(key, path);

  if (entry) {
    if (entry->type == REGTYPE_BINARY || entry->type == REGTYPE_STRING) {
      if (entry->string) {
        SMemFree(entry->string, __FILE__, __LINE__, 0);
      }
    }

    entry->type = 0;
    entry->string = 0;
    entry->size = 0;
    return entry;
  }

  return s_entryTable.New(key, 0, 0);
}

extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize) {
  RegistryEntry *entry;
  char           key[0x400];
  char           path[0x400];

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(buffer);
  FATALASSERT(buffersize);

  BuildKeyAndPath(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = FindEntry(key, path);
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

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(value);

  BuildKeyAndPath(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = FindEntry(key, path);
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

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);
  FATALASSERT(string);

  bytes = SStrLen(string) + 1;

  BuildKeyAndPath(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = CreateEntry(key, path);
  entry->type = REGTYPE_STRING;
  entry->string = static_cast<char *>(SMemAlloc(bytes, __FILE__, __LINE__, 0));
  entry->size = bytes;
  memcpy(entry->string, string, bytes);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value) {
  RegistryEntry *entry;
  char           key[0x400];
  char           path[0x400];

  FATALASSERT(keyname);
  FATALASSERT(*keyname);
  FATALASSERT(valuename);
  FATALASSERT(*valuename);

  BuildKeyAndPath(keyname, valuename, flags, key, sizeof(key), path, sizeof(path));

  entry = CreateEntry(key, path);
  entry->type = REGTYPE_DWORD;
  entry->value = value;

  return TRUE;
}

extern "C" BOOL APIENTRY SRegDestroy() {
  TSHashTable<WriteRegistryEntry, HASHKEY_STRI> writeTable;
  RegistryEntry                                *entry;
  WriteRegistryEntry                           *writeEntry;

  while ((entry = s_entryTable.Head()) != 0) {
    const char         *fullKey = entry->Key().GetString();
    const char         *keyEnd;
    const char         *valueStart;
    char                group[0x400];

    valueStart = SStrChr(fullKey, '\\');
    keyEnd = valueStart ? SStrChr(valueStart + 1, '\\') : 0;

    if (!keyEnd) {
      s_entryTable.Delete(entry);
      continue;
    }

    SStrCopy(group, fullKey, keyEnd - fullKey + 1);

    writeEntry = writeTable.Ptr(group);
    if (!writeEntry) {
      const char *directory;
      const char *subdirectory;
      char        folder[0x400];

      if (group[0] == 'B') {
        directory = AutoGetSharedPath();
        subdirectory = k_battleNetSubdirectory;
      } else if (group[0] == 'L') {
        directory = AutoGetSharedPath();
        subdirectory = k_localMachineSubdirectory;
      } else if (group[0] == 'C') {
        directory = AutoGetUserPath();
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
        WriteEscapedValue(writeEntry->file, entry->string, SStrLen(entry->string));
      } else if (entry->type == REGTYPE_BINARY) {
        fprintf(writeEntry->file, "%c%s\n", REGSIGIL_BINARY, fullKey);
        WriteEscapedValue(writeEntry->file, entry->string, entry->size);
      } else if (entry->type == REGTYPE_DWORD) {
        fprintf(writeEntry->file, "%c%s\n%d\n", REGSIGIL_DWORD, fullKey, entry->value);
      }
    }

    s_entryTable.Delete(entry);
  }

  while ((writeEntry = writeTable.Head()) != 0) {
    if (writeEntry->file) {
      fclose(writeEntry->file);
    }

    writeTable.Delete(writeEntry);
  }

  s_fileTable.Destroy();
  s_entryTable.Destroy();

  return TRUE;
}
