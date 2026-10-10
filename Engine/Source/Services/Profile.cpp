#include <Base/Base.h>

#include "Profile.h"

#include "Base/Handle.h"
#include "Base/UnrealConstants.h"

#include <storm.h>
#include <stpl.h>

#include <mbstring.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char SECTION_OPEN_CHAR = '[';
static const char SECTION_CLOSE_CHAR = ']';
static const char ASSIGNMENT_CHAR = '=';

namespace ProfileInternal {

  static const char COMMENT_BEGIN[] = "//";
  static const char NEWLINE_CHARS[] = "\r\n";
  static char       buf[256];
  static const char TRUESTR[] = "true";
  static const char FALSESTR[] = "false";

  NODEDECL(STRINGBLOCK) {
    static STRINGBLOCK *AllocBlock(DWORD chars);
    static char        *AllocString(LIST(STRINGBLOCK) & stringBlockList, LPCSTR string, int inSitu);
    static void         FreeString(LIST(STRINGBLOCK) & stringBlockList, char *string);

    BOOL Contains(LPCSTR string) const {
      return string >= m_data && string < m_data + m_dataSize;
    }

    DWORD m_refCount;
    DWORD m_dataSize;
    DWORD m_dataUsed;
    char  m_data[4];
  };

  struct KEYVALUE : public TSHashObject<KEYVALUE, HASHKEY_CONSTSTRI> {
    TSGrowableArray<char *> values;
  };

  struct SECTION : public TSHashObject<SECTION, HASHKEY_CONSTSTRI> {
    ~SECTION() {
      keyTable.Clear();
    }

    TSHashTable<KEYVALUE, HASHKEY_CONSTSTRI> keyTable;
  };

  struct PROFILE : public CHandleObject {
    virtual ~PROFILE() {
      sectionTable.Clear();
      stringBlockList.Clear();
    }

    TSHashTable<SECTION, HASHKEY_CONSTSTRI> sectionTable;
    LISTDECL(STRINGBLOCK, stringBlockList);
  };

  static int       IReadFile(PROFILE *profile, LPCSTR rawPath);
  static BOOL      IWriteFile(PROFILE *profile, LPCSTR path);
  static BOOL      IReadBuffer(PROFILE *profile, LPCVOID buffer, DWORD bufferBytes);
  static void      TokenizeStringValues(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, char *value);
  static void      ISetValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, LPCSTR value, int clear, int inSitu);
  static LPCSTR    IGetValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, UINT index);
  static UINT      IGetNumValues(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName);
  static KEYVALUE *GetKeyValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName);
  static void      WriteLine(TSGrowableArray<char> &buffer, LPCSTR pszFmt, ...);
  static BOOL      WriteFileBuffer(LPCSTR path, const TSGrowableArray<char> &buffer);
  static void      WriteKey(KEYVALUE *key, TSGrowableArray<char> &buffer);
  static void      WriteSection(SECTION *section, TSGrowableArray<char> &buffer);
  static int       PrfStrToInt(LPCSTR str);

  STRINGBLOCK *STRINGBLOCK::AllocBlock(DWORD chars) {
    STRINGBLOCK *block = new (ALLOC(sizeof(STRINGBLOCK) - sizeof(((STRINGBLOCK *)0)->m_data) + max(chars, sizeof(((STRINGBLOCK *)0)->m_data)))) STRINGBLOCK;
    block->m_dataSize = chars;
    block->m_refCount = 0;
    block->m_dataUsed = 0;
    return block;
  }

  char *STRINGBLOCK::AllocString(LIST(STRINGBLOCK) & stringBlockList, LPCSTR string, int inSitu) {
    VALIDATEBEGIN;
    VALIDATE(string);
    VALIDATEEND;

    if (inSitu) {
      ITERATELIST(STRINGBLOCK, stringBlockList, stringBlock) {
        if (stringBlock->Contains(string)) {
          ++stringBlock->m_refCount;
          return (char *)string;
        }
      }
    }

    DWORD        chars = SStrLen(string) + 1;
    STRINGBLOCK *stringBlock = stringBlockList.Head();
    if (!stringBlock || chars > stringBlock->m_dataSize - stringBlock->m_dataUsed) {
      stringBlock = AllocBlock(max(chars, 4096));
      stringBlockList.LinkNode(stringBlock, LIST_HEAD, 0);
    }

    char *dest = stringBlock->m_data + stringBlock->m_dataUsed;
    memcpy(dest, string, chars);
    stringBlock->m_dataUsed += chars;
    ++stringBlock->m_refCount;
    return dest;
  }

  void STRINGBLOCK::FreeString(LIST(STRINGBLOCK) & stringBlockList, char *string) {
    VALIDATEBEGIN;
    VALIDATE(string);
    VALIDATEENDVOID;

    STRINGBLOCK *stringBlock = 0;
    ITERATELIST(STRINGBLOCK, stringBlockList, block) {
      if (block->Contains(string)) {
        stringBlock = block;
        break;
      }
    }

    ASSERT(stringBlock);

    if (!--stringBlock->m_refCount) {
      stringBlock->m_dataUsed = 0;
      stringBlockList.LinkNode(stringBlock, LIST_HEAD, 0);
    }
  }

}

HPROFILE ProfileCreate() {
  return NEW(ProfileInternal::PROFILE);
}

int ProfileReadFile(HPROFILE handle, LPCSTR path) {
  VALIDATEBEGIN;
  VALIDATE(path);
  VALIDATEEND;

  return ProfileInternal::IReadFile((ProfileInternal::PROFILE *)handle, path);
}

namespace ProfileInternal {

  static int IReadFile(PROFILE *profile, LPCSTR rawPath) {
    char  path[MAX_PATH];
    char *end;
    int   result;

    SStrCopy(path, rawPath, sizeof(path));

    end = path + SStrLen(path) - 1;
    if (end >= path) {
      while (_ismbcspace(*end)) {
        --end;
      }
      end[1] = 0;
    }

    DWORD  bufferBytes = 0;
    LPVOID buffer = 0;
    if (!SFile::LoadFile(path, &buffer, &bufferBytes, 1, 0)) {
      return 0;
    }

    result = IReadBuffer(profile, buffer, bufferBytes);
    SFile::Unload(buffer);
    return result;
  }

  static BOOL IReadBuffer(PROFILE *profile, LPCVOID buffer, DWORD bufferBytes) {
    enum {
      STATE_NEWLINE = 0,
      STATE_COMMENT = 1,
      STATE_SECTION = 2,
      STATE_STRIP_TRAILING = 3,
      STATE_KEY = 4,
      STATE_VALUE = 5
    };

    STRINGBLOCK *stringBlock = STRINGBLOCK::AllocBlock(bufferBytes + 1);
    memcpy(stringBlock->m_data, buffer, bufferBytes);
    stringBlock->m_data[bufferBytes] = 0;
    stringBlock->m_dataUsed = stringBlock->m_dataSize;
    profile->stringBlockList.LinkNode(stringBlock, LIST_TAIL, 0);

    LPCSTR sectionName = 0;
    LPCSTR lastSection = 0;
    LPCSTR curKey = 0;
    char  *curValue = 0;
    int    state = STATE_NEWLINE;
    char  *cursor = stringBlock->m_data;

    while (*cursor) {
      switch (state) {
        case STATE_NEWLINE:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            break;
          }

          if (!SStrCmp(cursor, COMMENT_BEGIN, 2)) {
            state = STATE_COMMENT;
          } else if (*cursor == SECTION_OPEN_CHAR) {
            sectionName = cursor + 1;
            state = STATE_SECTION;
          } else {
            curKey = cursor;
            state = STATE_KEY;
          }
          break;

        case STATE_SECTION:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            sectionName = lastSection;
            state = STATE_NEWLINE;
          } else if (*cursor == SECTION_CLOSE_CHAR) {
            *cursor = 0;
            lastSection = sectionName;
            state = STATE_STRIP_TRAILING;
          }
          break;

        case STATE_COMMENT:
        case STATE_STRIP_TRAILING:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            state = STATE_NEWLINE;
          }
          break;

        case STATE_KEY:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            state = STATE_NEWLINE;
            curKey = 0;
          } else if (*cursor == ASSIGNMENT_CHAR) {
            *cursor = 0;
            curValue = cursor + 1;
            state = STATE_VALUE;
          }
          break;

        case STATE_VALUE:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            *cursor = 0;
            TokenizeStringValues(profile, sectionName, curKey, curValue);
            state = STATE_NEWLINE;
          }
          break;
      }

      ++cursor;
    }

    if (state == STATE_VALUE) {
      TokenizeStringValues(profile, sectionName, curKey, curValue);
    }

    return 1;
  }

  static void TokenizeStringValues(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, char *value) {
    char *token;
    int   quoted;

    if (!sectionName || !keyName || !value || !*value) {
      return;
    }

    while (*value) {
      quoted = 0;
      if (*value == '"') {
        quoted = 1;
        ++value;
      }

      token = value;
      while (*value) {
        if (!quoted) {
          if (*value == ',') {
            *value++ = 0;
            break;
          }
        } else if (*value == '"') {
          *value++ = 0;
          if (*value == ',') {
            ++value;
          }
          break;
        }
        ++value;
      }

      ISetValue(profile, sectionName, keyName, token, 0, 1);
    }
  }

  static void ISetValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, LPCSTR value, int clear, int inSitu) {
    SECTION  *section;
    KEYVALUE *keyValue;
    LPCSTR    storedString;
    UINT      index;

    section = profile->sectionTable.Ptr(sectionName);
    if (!section) {
      storedString = STRINGBLOCK::AllocString(profile->stringBlockList, sectionName, inSitu);
      section = profile->sectionTable.New(storedString, 0, 0);
    }

    keyValue = section->keyTable.Ptr(keyName);
    if (!keyValue) {
      storedString = STRINGBLOCK::AllocString(profile->stringBlockList, keyName, inSitu);
      keyValue = section->keyTable.New(storedString, 0, 0);
    }

    if (clear) {
      index = keyValue->values.Count();
      while (index) {
        --index;
        STRINGBLOCK::FreeString(profile->stringBlockList, keyValue->values[index]);
      }
      keyValue->values.Clear();
    }

    *keyValue->values.New() = STRINGBLOCK::AllocString(profile->stringBlockList, value, inSitu);
  }

}

int ProfileWriteFile(HPROFILE handle, LPCSTR path) {
  VALIDATEBEGIN;
  VALIDATE(path);
  VALIDATEEND;

  return ProfileInternal::IWriteFile((ProfileInternal::PROFILE *)handle, path);
}

namespace ProfileInternal {

  static BOOL IWriteFile(PROFILE *profile, LPCSTR path) {
    TSGrowableArray<char> buffer;

    buffer.ReserveSpace(4096);
    buffer.SetChunkSize(4096);

    ITERATELIST(SECTION, profile->sectionTable, section) {
      WriteSection(section, buffer);
    }

    return WriteFileBuffer(path, buffer);
  }

  static BOOL WriteFileBuffer(LPCSTR path, const TSGrowableArray<char> &buffer) {
    FILE *file;
    UINT  bytesWritten;

    file = fopen(path, "wt");
    if (!file) {
      return 0;
    }

    bytesWritten = fwrite(buffer.Ptr(), 1, buffer.Count(), file);
    if (fclose(file)) {
      return 0;
    }

    return bytesWritten == buffer.Count();
  }

  static void WriteSection(SECTION *section, TSGrowableArray<char> &buffer) {
    WriteLine(buffer, "[%s]\n", section->GetString());
    ITERATELIST(KEYVALUE, section->keyTable, key) {
      WriteKey(key, buffer);
    }
    WriteLine(buffer, "\n");
  }

  static void WriteLine(TSGrowableArray<char> &buffer, LPCSTR pszFmt, ...) {
    va_list args;
    int     numchars;

    va_start(args, pszFmt);
    numchars = _vsnprintf(buf, sizeof(buf), pszFmt, args);
    va_end(args);

    if (numchars == sizeof(buf)) {
      numchars = sizeof(buf) - 1;
      buf[numchars] = 0;
    } else if (numchars <= 0) {
      return;
    }

    buffer.Add(numchars, buf);
  }

  static void WriteKey(KEYVALUE *key, TSGrowableArray<char> &buffer) {
    UINT          loop;
    LPCSTR const *value;

    WriteLine(buffer, "%s=", key->GetString());
    value = key->values.Ptr();
    for (loop = key->values.Count(); loop; --loop, ++value) {
      if (SStrChr(*value, ',')) {
        *buffer.New() = '"';
        WriteLine(buffer, *value);
        *buffer.New() = '"';
      } else {
        WriteLine(buffer, *value);
      }

      if (loop > 1) {
        *buffer.New() = ',';
      }
    }
    WriteLine(buffer, "\n");
  }

}

int ProfileReadBuffer(HPROFILE handle, LPCVOID buffer, DWORD bufferBytes) {
  return ProfileInternal::IReadBuffer((ProfileInternal::PROFILE *)handle, buffer, bufferBytes);
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, bool value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, value ? ProfileInternal::TRUESTR : ProfileInternal::FALSESTR, 0, 0);
  return 1;
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%d", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 0, 0);
  return 1;
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%I64d", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 0, 0);
  return 1;
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%f", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 0, 0);
  return 1;
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  unreal::asString(value, strValue, 1, -1);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 0, 0);
  return 1;
}

BOOL ProfileAddValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATE(value);
  VALIDATEEND;

  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, value, 0, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, bool value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, value ? ProfileInternal::TRUESTR : ProfileInternal::FALSESTR, 1, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%d", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 1, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%I64d", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 1, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%f", value);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 1, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, const unreal &value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  char strValue[256];
  unreal::asString(value, strValue, 1, -1);
  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, strValue, 1, 0);
  return 1;
}

BOOL ProfileSetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATE(value);
  VALIDATEEND;

  ProfileInternal::ISetValue((ProfileInternal::PROFILE *)handle, section, key, value, 1, 0);
  return 1;
}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, bool *value, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEANDBLANK(value);
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  *value = !SStrCmpI(string, ProfileInternal::TRUESTR, 0x7FFFFFFF);
  return 1;
}

namespace ProfileInternal {

  static LPCSTR IGetValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName, UINT index) {
    KEYVALUE *keyValue = GetKeyValue(profile, sectionName, keyName);

    if (keyValue) {
      return index < keyValue->values.Count() ? keyValue->values[index] : 0;
    }

    return 0;
  }

  static KEYVALUE *GetKeyValue(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName) {
    SECTION *section = profile->sectionTable.Ptr(sectionName);

    if (section) {
      return section->keyTable.Ptr(keyName);
    }

    return 0;
  }

}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, int *value, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEANDBLANK(value);
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  *value = ProfileInternal::PrfStrToInt(string);
  return 1;
}

namespace ProfileInternal {

  static int PrfStrToInt(LPCSTR str) {
    int  value = 0;
    UINT index;

    if (*str == '\'') {
      ++str;
      for (index = 4; index-- && *str && *str != '\''; ++str) {
        value = (value << 8) | (BYTE)*str;
      }
    } else {
      value = SStrToInt(str);
    }

    return value;
  }

}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, LONGLONG *value, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEANDBLANK(value);
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  *value = SStrToInt64(string);
  return 1;
}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, float *value, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEANDBLANK(value);
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  *value = SStrToFloat(string);
  return 1;
}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, unreal *value, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATE(value);
  *value = u_0;
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  *value = unreal::fromString(string);
  return 1;
}

BOOL ProfileGetValue(HPROFILE handle, LPCSTR section, LPCSTR key, char *value, UINT maxChars, UINT index) {
  LPCSTR string;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEANDBLANK(value);
  VALIDATEEND;

  string = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  if (!string) {
    return 0;
  }

  SStrCopy(value, string, maxChars);
  return 1;
}

LPCSTR ProfileGetValueNoCopy(HPROFILE handle, LPCSTR section, LPCSTR key, UINT index) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  return ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
}

UINT ProfileGetNumValues(HPROFILE handle, LPCSTR section, LPCSTR key) {
  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATEEND;

  return ProfileInternal::IGetNumValues((ProfileInternal::PROFILE *)handle, section, key);
}

namespace ProfileInternal {

  static UINT IGetNumValues(PROFILE *profile, LPCSTR sectionName, LPCSTR keyName) {
    KEYVALUE *keyValue = GetKeyValue(profile, sectionName, keyName);
    return keyValue ? keyValue->values.Count() : 0;
  }

}  // namespace ProfileInternal

int ProfileGetValueIndex(HPROFILE handle, LPCSTR section, LPCSTR key, LPCSTR value) {
  UINT   index;
  LPCSTR candidate;

  VALIDATEBEGIN;
  VALIDATE(section);
  VALIDATE(key);
  VALIDATE(value);
  VALIDATEEND;

  index = 0;
  candidate = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, index);
  while (candidate) {
    if (!SStrCmpI(candidate, value, 0x7FFFFFFF)) {
      return index;
    }
    candidate = ProfileInternal::IGetValue((ProfileInternal::PROFILE *)handle, section, key, ++index);
  }
  return -1;
}

void ProfileEnumKeys(HPROFILE handle, LPCSTR sectionName, PROFILEENUMKEYCALLBACK callback, LPVOID opaqueData) {
  ProfileInternal::PROFILE  *profile;
  ProfileInternal::SECTION  *pSection;

  profile = (ProfileInternal::PROFILE *)handle;
  pSection = profile->sectionTable.Ptr(sectionName);
  VALIDATEBEGIN;
  VALIDATE(pSection);
  VALIDATEENDVOID;

  ITERATELIST(ProfileInternal::KEYVALUE, pSection->keyTable, key) {
    callback(key->GetString(), key->values[0], opaqueData);
  }
}

void ProfileEnumSections(HPROFILE handle, PROFILEENUMSECTIONCALLBACK callback, LPVOID opaqueData) {
  ProfileInternal::PROFILE *profile = (ProfileInternal::PROFILE *)handle;

  ITERATELIST(ProfileInternal::SECTION, profile->sectionTable, section) {
    callback(section->GetString(), opaqueData);
  }
}

BOOL ProfileSectionExists(HPROFILE profile, LPCSTR section) {
  ProfileInternal::PROFILE *profilePtr;

  profilePtr = (ProfileInternal::PROFILE *)profile;
  VALIDATEBEGIN;
  VALIDATE(profilePtr);
  VALIDATEEND;

  return profilePtr->sectionTable.Ptr(section) != 0;
}

void ProfileClose(HPROFILE handle) {
  if (handle) {
    DEL((ProfileInternal::PROFILE *)handle);
  }
}
