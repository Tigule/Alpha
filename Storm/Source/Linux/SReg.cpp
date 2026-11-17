#include <storm.h>
#include <stpl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NODEDECL(REGISTRYENTRY) {
  char key[0x80];
  char value[0x400];
};

static LISTDECL(REGISTRYENTRY, s_registry);

static const char s_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static FILE *OpenRegistryFile(LPCSTR mode) {
  LPCSTR home = getenv("HOME");
  char   path[0x1000];

  if (!home) {
    home = ".";
  }

  SStrPrintf(path, sizeof(path), "%s/.blizzard/registry.txt", home);
  return fopen(path, mode);
}

static void LoadRegistry() {
  FILE *file;
  char  line[0x1000];

  file = OpenRegistryFile("r");

  ASSERT(s_registry.IsEmpty());

  if (!file) {
    return;
  }

  while (fgets(line, sizeof(line), file)) {
    REGISTRYENTRY *entry;
    char          *value;

    line[SStrLen(line) - 1] = 0;

    value = SStrChr(line, '=');
    if (!value) {
      continue;
    }

    *value = 0;

    entry = s_registry.NewNode(LIST_TAIL, 0, 0);
    SStrCopy(entry->key, line, sizeof(entry->key));

    char *dest = entry->value;
    char *end = entry->value + sizeof(entry->value) - 1;
    for (++value; *value && dest < end; ++value) {
      if (*value == '\\' && value[1]) {
        ++value;
        *dest++ = *value == 'n' ? '\n' : *value == 'r' ? '\r' : *value;
      } else {
        *dest++ = *value;
      }
    }
    *dest = 0;
  }

  fclose(file);
}

static void SaveRegistry() {
  FILE *file = OpenRegistryFile("w");

  if (!file) {
    return;
  }

  ITERATELIST(REGISTRYENTRY, s_registry, entry) {
    fprintf(file, "%s=", entry->key);
    for (LPCSTR value = entry->value; *value; ++value) {
      if (*value == '\n') {
        fputs("\\n", file);
      } else if (*value == '\r') {
        fputs("\\r", file);
      } else if (*value == '\\') {
        fputs("\\\\", file);
      } else {
        fputc(*value, file);
      }
    }
    fputc('\n', file);
  }

  fclose(file);
}

static BOOL LoadRegistryEntry(LPCSTR key, char *buffer, DWORD buffersize) {
  BOOL result = FALSE;

  LoadRegistry();

  ITERATELIST(REGISTRYENTRY, s_registry, entry) {
    if (!SStrCmpI(entry->key, key, 0x7FFFFFFF)) {
      SStrCopy(buffer, entry->value, buffersize);
      result = TRUE;
      break;
    }
  }

  s_registry.Clear();
  return result;
}

static BOOL CreateRegistryEntry(LPCSTR key, LPCSTR value) {
  REGISTRYENTRY *found = 0;

  LoadRegistry();

  ITERATELIST(REGISTRYENTRY, s_registry, entry) {
    if (!SStrCmpI(entry->key, key, 0x7FFFFFFF)) {
      found = entry;
      break;
    }
  }

  if (!found) {
    found = s_registry.NewNode(LIST_TAIL, 0, 0);
    SStrCopy(found->key, key, sizeof(found->key));
  }

  SStrCopy(found->value, value, sizeof(found->value));
  SaveRegistry();

  s_registry.Clear();
  return TRUE;
}

static void DeleteRegistryEntries(LPCSTR prefix) {
  DWORD          length = SStrLen(prefix);
  REGISTRYENTRY *entry;
  REGISTRYENTRY *next;

  LoadRegistry();

  for (entry = s_registry.Head(); entry; entry = next) {
    next = s_registry.Next(entry);

    if (!SStrCmpI(entry->key, prefix, length)) {
      s_registry.DeleteNode(entry);
    }
  }

  SaveRegistry();
  s_registry.Clear();
}

static void CreateKey(LPCSTR keyname, LPCSTR valuename, char *key, UINT keysize) {
  int index;

  VALIDATEBEGIN;
  VALIDATE(keyname && *keyname);
  VALIDATE(valuename);
  VALIDATEENDVOID;

  SStrCopy(key, keyname, keysize);

  for (index = SStrLen(key) - 1; index >= 0; --index) {
    if (key[index] == '\\') {
      key[index] = ':';
    }
  }

  SStrPack(key, ":", keysize);
  SStrPack(key, valuename, keysize);
}

static void EncodeBase64(const BYTE *data, DWORD bytes, char **result) {
  char *output = (char *)SMemAlloc((bytes * 4) / 3 + 5, "new[]", -1, 0);
  DWORD outchars = 0;
  DWORD index;
  BYTE  group[4];
  DWORD count = 0;

  *result = output;

  for (index = 0; index < bytes; ++index) {
    switch (count++) {
      case 0:
        group[0] = (data[index] & 0xFC) >> 2;
        group[1] = (data[index] & 0x03) << 4;
        break;

      case 1:
        group[1] |= (data[index] & 0xF0) >> 4;
        group[2] = (data[index] & 0x0F) << 2;
        break;

      case 2:
        group[2] |= (data[index] & 0xC0) >> 6;
        group[3] = data[index] & 0x3F;

        for (count = 0; count <= 3; ++count) {
          output[outchars++] = s_base64[group[count]];
        }

        count = 0;
        break;
    }
  }

  if (count) {
    DWORD pad;

    for (pad = 0; pad <= count; ++pad) {
      output[outchars++] = s_base64[group[pad]];
    }

    while (count <= 2) {
      ++count;
      output[outchars++] = '=';
    }
  }

  output[outchars] = 0;
}

static BOOL DecodeBase64(LPCSTR string, BYTE **result, DWORD *bytes) {
  DWORD length;
  BYTE *output;
  DWORD outbytes = 0;
  DWORD count = 0;
  BYTE  group[4];
  DWORD index;

  if (!string[0] || string[0] == '\n') {
    *bytes = 0;
    return FALSE;
  }

  length = SStrLen(string);
  output = (BYTE *)SMemAlloc((length * 3) / 4 + 1, "new[]", -1, 0);
  *result = output;

  for (index = 0; index < length; ++index) {
    char c = string[index];

    if (c >= 'A' && c <= 'Z') {
      group[count] = c - 'A';
    } else if (c >= 'a' && c <= 'z') {
      group[count] = c - 'a' + 26;
    } else if (c >= '0' && c <= '9') {
      group[count] = c - '0' + 52;
    } else if (c == '+') {
      group[count] = 62;
    } else if (c == '/') {
      group[count] = 63;
    } else if (c == '=') {
      if (count == 2) {
        output[outbytes++] = ((group[0] & 0x3F) << 2) | ((group[1] & 0x30) >> 4);
      } else if (count == 3) {
        output[outbytes++] = ((group[0] & 0x3F) << 2) | ((group[1] & 0x30) >> 4);
        output[outbytes++] = ((group[1] & 0x0F) << 4) | ((group[2] & 0x3C) >> 2);
      }

      *bytes = outbytes;
      return FALSE;
    } else {
      continue;
    }

    if (++count == 4) {
      output[outbytes++] = ((group[0] & 0x3F) << 2) | ((group[1] & 0x30) >> 4);
      output[outbytes++] = ((group[1] & 0x0F) << 4) | ((group[2] & 0x3C) >> 2);
      output[outbytes++] = ((group[2] & 0x03) << 6) | (group[3] & 0x3F);
      count = 0;
    }
  }

  *bytes = outbytes;
  return TRUE;
}

static void FreeBase64(LPVOID data) {
  if (data) {
    SMemFree(data, "delete[]", -1, 0);
  }
}

extern "C" BOOL APIENTRY SRegLoadData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPVOID buffer, DWORD buffersize, LPDWORD bytesread) {
  char  string[0x400];
  BYTE *data;
  DWORD bytes;

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  if (!SRegLoadString(keyname, valuename, flags, string, sizeof(string))) {
    return FALSE;
  }

  DecodeBase64(string, &data, &bytes);
  memcpy(buffer, data, buffersize > bytes ? bytes : buffersize);
  FreeBase64(data);

  if (bytesread) {
    *bytesread = bytes;
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SRegLoadString(LPCSTR keyname, LPCSTR valuename, UINT flags, char *buffer, DWORD buffersize) {
  char key[0x80];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(buffer);
  VALIDATE(buffersize);
  VALIDATEEND;

  CreateKey(keyname, valuename, key, sizeof(key));
  return LoadRegistryEntry(key, buffer, buffersize);
}

extern "C" BOOL APIENTRY SRegLoadValue(LPCSTR keyname, LPCSTR valuename, UINT flags, LPDWORD value) {
  char string[0x20];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(value);
  VALIDATEEND;

  if (!SRegLoadString(keyname, valuename, flags, string, sizeof(string))) {
    return FALSE;
  }

  *value = SStrToInt(string);
  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveData(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCVOID data, DWORD databytes) {
  char  key[0x80];
  char *string;

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  CreateKey(keyname, valuename, key, sizeof(key));
  EncodeBase64((const BYTE *)data, databytes, &string);
  CreateRegistryEntry(key, string);
  FreeBase64(string);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveString(LPCSTR keyname, LPCSTR valuename, UINT flags, LPCSTR string) {
  char key[0x80];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATE(string);
  VALIDATEEND;

  CreateKey(keyname, valuename, key, sizeof(key));
  CreateRegistryEntry(key, string);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegSaveValue(LPCSTR keyname, LPCSTR valuename, UINT flags, DWORD value) {
  char key[0x80];
  char string[0x20];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  CreateKey(keyname, valuename, key, sizeof(key));
  SStrPrintf(string, sizeof(string), "%d", value);
  CreateRegistryEntry(key, string);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegDeleteValue(LPCSTR keyname, LPCSTR valuename, UINT flags) {
  char key[0x80];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATE(valuename);
  VALIDATE(*valuename);
  VALIDATEEND;

  CreateKey(keyname, valuename, key, sizeof(key));
  DeleteRegistryEntries(key);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegDeleteKey(LPCSTR keyname, UINT flags) {
  char key[0x80];

  VALIDATEBEGIN;
  VALIDATE(keyname);
  VALIDATE(*keyname);
  VALIDATEEND;

  CreateKey(keyname, "", key, sizeof(key));
  DeleteRegistryEntries(key);

  return TRUE;
}

extern "C" BOOL APIENTRY SRegDestroy() {
  return TRUE;
}
