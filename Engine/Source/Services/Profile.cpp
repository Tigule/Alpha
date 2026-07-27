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

namespace ProfileInternal {

  static const char SECTION_OPEN_CHAR = '[';
  static const char SECTION_CLOSE_CHAR = ']';
  static const char ASSIGNMENT_CHAR = '=';
  static const char COMMENT_BEGIN[] = "//";
  static const char NEWLINE_CHARS[] = "\r\n";
  static char       buf[256];
  static const char TRUESTR[] = "true";
  static const char FALSESTR[] = "false";

  struct STRINGBLOCK : public TSLinkedNode<STRINGBLOCK> {
    int Contains(const char *string) const {
      return string >= m_data && string < m_data + m_dataSize;
    }

    static STRINGBLOCK *AllocBlock(unsigned long chars);
    static char *AllocString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> > &stringBlockList, const char *string, int inSitu);
    static void FreeString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> > &stringBlockList, char *string);

    unsigned long m_refCount;
    unsigned long m_dataSize;
    unsigned long m_dataUsed;
    char          m_data[4];
  };

  struct KEYVALUE : public TSHashObject<KEYVALUE, HASHKEY_CONSTSTRI> {
    TSGrowableArray<char *> values;
  };

  struct SECTION : public TSHashObject<SECTION, HASHKEY_CONSTSTRI> {
    ~SECTION() {}

    TSHashTable<KEYVALUE, HASHKEY_CONSTSTRI> keyTable;
  };

  struct PROFILE : public CHandleObject {
    virtual ~PROFILE() {
      STRINGBLOCK *stringBlock;

      sectionTable.Clear();

      while ((stringBlock = stringBlockList.Head()) != 0) {
        DEL(stringBlock);
      }
    }

    TSHashTable<SECTION, HASHKEY_CONSTSTRI>      sectionTable;
    TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> > stringBlockList;
  };

  static int IReadFile(PROFILE *profile, const char *rawPath);
  static int IWriteFile(PROFILE *profile, const char *path);
  static int IReadBuffer(PROFILE *profile, const void *buffer, unsigned long bufferBytes);
  static void TokenizeStringValues(PROFILE *profile, const char *sectionName, const char *keyName, char *value);
  static void ISetValue(PROFILE *profile, const char *sectionName, const char *keyName, const char *value, int clear, int inSitu);
  static const char *IGetValue(PROFILE *profile, const char *sectionName, const char *keyName, unsigned int index);
  static unsigned int IGetNumValues(PROFILE *profile, const char *sectionName, const char *keyName);
  static KEYVALUE *GetKeyValue(PROFILE *profile, const char *sectionName, const char *keyName);
  static void                    WriteLine(TSGrowableArray<char> &buffer, const char *pszFmt, ...);
  static int WriteFileBuffer(const char *path, const TSGrowableArray<char> &buffer);
  static void WriteKey(KEYVALUE *key, TSGrowableArray<char> &buffer);
  static void WriteSection(SECTION *section, TSGrowableArray<char> &buffer);
  static int PrfStrToInt(const char *str);

  STRINGBLOCK *STRINGBLOCK::AllocBlock(unsigned long chars) {
    unsigned long dataChars = sizeof(((STRINGBLOCK *)0)->m_data);
    unsigned long allocChars = chars < dataChars ? dataChars : chars;

    STRINGBLOCK *block = new (ALLOC(sizeof(STRINGBLOCK) + allocChars - dataChars)) STRINGBLOCK;
    block->m_refCount = 0;
    block->m_dataSize = chars;
    block->m_dataUsed = 0;
    return block;
  }

  char *STRINGBLOCK::AllocString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> > &stringBlockList, const char *string, int inSitu) {
    STRINGBLOCK  *stringBlock;
    unsigned long chars;
    char         *dest;

    FATALASSERT(string);

    if (inSitu) {
      stringBlock = stringBlockList.Head();
      while (stringBlock) {
        if (stringBlock->Contains(string)) {
          ++stringBlock->m_refCount;
          return const_cast<char *>(string);
        }
        stringBlock = stringBlockList.Next(stringBlock);
      }
    }

    chars = SStrLen(string) + 1;
    stringBlock = stringBlockList.Head();
    if (!stringBlock || chars > stringBlock->m_dataSize - stringBlock->m_dataUsed) {
      stringBlock = AllocBlock(chars < 4096 ? 4096 : chars);
      stringBlockList.LinkNode(stringBlock, LIST_HEAD, 0);
    }

    dest = stringBlock->m_data + stringBlock->m_dataUsed;
    memcpy(dest, string, chars);
    stringBlock->m_dataUsed += chars;
    ++stringBlock->m_refCount;
    return dest;
  }

  void STRINGBLOCK::FreeString(TSList<STRINGBLOCK, TSGetLink<STRINGBLOCK> > &stringBlockList, char *string) {
    STRINGBLOCK *stringBlock;

    FATALASSERT(string);

    stringBlock = stringBlockList.Head();
    while (stringBlock && !stringBlock->Contains(string)) {
      stringBlock = stringBlockList.Next(stringBlock);
    }

    ASSERT(stringBlock);

    if (!--stringBlock->m_refCount) {
      stringBlock->m_dataUsed = 0;
      stringBlockList.UnlinkNode(stringBlock);
      stringBlockList.LinkNode(stringBlock, LIST_HEAD, 0);
    }
  }

  static void WriteLine(TSGrowableArray<char> &buffer, const char *pszFmt, ...) {
    va_list args;
    int     numchars;
    int     index;

    va_start(args, pszFmt);
    numchars = _vsnprintf(buf, sizeof(buf), pszFmt, args);
    va_end(args);

    if (numchars == sizeof(buf)) {
      numchars = sizeof(buf) - 1;
      buf[numchars] = 0;
    } else if (numchars <= 0) {
      return;
    }

    for (index = 0; index < numchars; ++index) {
      *buffer.New() = buf[index];
    }
  }

  static KEYVALUE *GetKeyValue(PROFILE *profile, const char *sectionName, const char *keyName) {
    SECTION *section = profile->sectionTable.Ptr(sectionName);

    if (!section) {
      return 0;
    }

    return section->keyTable.Ptr(keyName);
  }

  static unsigned int IGetNumValues(PROFILE *profile, const char *sectionName, const char *keyName) {
    KEYVALUE *keyValue = GetKeyValue(profile, sectionName, keyName);
    return keyValue ? keyValue->values.Count() : 0;
  }

  static const char *IGetValue(PROFILE *profile, const char *sectionName, const char *keyName, unsigned int index) {
    KEYVALUE *keyValue = GetKeyValue(profile, sectionName, keyName);

    if (!keyValue || index >= keyValue->values.Count()) {
      return 0;
    }

    return keyValue->values[index];
  }

  static void ISetValue(PROFILE *profile, const char *sectionName, const char *keyName, const char *value, int clear, int inSitu) {
    SECTION     *section;
    KEYVALUE    *keyValue;
    const char  *storedString;
    unsigned int index;

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

  static void TokenizeStringValues(PROFILE *profile, const char *sectionName, const char *keyName, char *value) {
    char *token;
    int   quoted;

    if (!sectionName || !keyName || !value || !*value) {
      return;
    }

    while (*value) {
      quoted = *value == '"';
      if (quoted) {
        ++value;
      }

      token = value;
      while (*value && ((quoted && *value != '"') || (!quoted && *value != ','))) {
        ++value;
      }

      if (*value) {
        *value++ = 0;
        if (quoted && *value == ',') {
          ++value;
        }
      }

      ISetValue(profile, sectionName, keyName, token, 0, 1);
    }
  }

  static int IReadBuffer(PROFILE *profile, const void *buffer, unsigned long bufferBytes) {
    enum {
      STATE_NEWLINE = 0,
      STATE_COMMENT = 1,
      STATE_SECTION = 2,
      STATE_STRIP_TRAILING = 3,
      STATE_KEY = 4,
      STATE_VALUE = 5
    };

    STRINGBLOCK *stringBlock;
    char        *cursor;
    const char  *sectionName = 0;
    const char  *curKey = 0;
    char        *curValue = 0;
    const char  *lastSection = 0;
    int          state = STATE_NEWLINE;

    stringBlock = STRINGBLOCK::AllocBlock(bufferBytes + 1);
    memcpy(stringBlock->m_data, buffer, bufferBytes);
    stringBlock->m_data[bufferBytes] = 0;
    stringBlock->m_dataUsed = stringBlock->m_dataSize;
    profile->stringBlockList.LinkNode(stringBlock, LIST_TAIL, 0);

    cursor = stringBlock->m_data;
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

        case STATE_COMMENT:
        case STATE_STRIP_TRAILING:
          if (SStrChr(NEWLINE_CHARS, *cursor)) {
            state = STATE_NEWLINE;
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

  static int IReadFile(PROFILE *profile, const char *rawPath) {
    void         *buffer = 0;
    unsigned long bufferBytes = 0;
    char          path[MAX_PATH];
    char         *end;
    int           result;

    SStrCopy(path, rawPath, sizeof(path));

    end = path + SStrLen(path) - 1;
    if (end >= path && _ismbcspace(*end)) {
      do {
        --end;
      } while (end >= path && _ismbcspace(*end));
      end[1] = 0;
    }

    if (!SFile::LoadFile(path, &buffer, &bufferBytes, 1, 0)) {
      return 0;
    }

    result = IReadBuffer(profile, buffer, bufferBytes);
    SFile::Unload(buffer);
    return result;
  }

  static int WriteFileBuffer(const char *path, const TSGrowableArray<char> &buffer) {
    FILE        *file;
    unsigned int bytesWritten;

    file = fopen(path, "wt");
    if (!file) {
      return 0;
    }

    bytesWritten = fwrite(buffer.Ptr(), 1, buffer.Count(), file);
    return !fclose(file) && bytesWritten == buffer.Count();
  }

  static void WriteKey(KEYVALUE *key, TSGrowableArray<char> &buffer) {
    unsigned int       loop;
    const char *const *value;

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

  static void WriteSection(SECTION *section, TSGrowableArray<char> &buffer) {
    KEYVALUE *key;

    WriteLine(buffer, "[%s]\n", section->GetString());
    key = section->keyTable.Head();
    while (key) {
      WriteKey(key, buffer);
      key = section->keyTable.Next(key);
    }
    WriteLine(buffer, "\n");
  }

  static int IWriteFile(PROFILE *profile, const char *path) {
    TSGrowableArray<char> buffer;
    SECTION              *section;

    buffer.ReserveSpace(4096);
    buffer.SetChunkSize(4096);

    section = profile->sectionTable.Head();
    while (section) {
      WriteSection(section, buffer);
      section = profile->sectionTable.Next(section);
    }

    return WriteFileBuffer(path, buffer);
  }

  static int PrfStrToInt(const char *str) {
    int          value = 0;
    unsigned int index;

    if (*str != '\'') {
      return SStrToInt(str);
    }

    ++str;
    for (index = 0; index < 4 && *str && *str != '\''; ++index, ++str) {
      value = (value << 8) | static_cast<unsigned char>(*str);
    }

    return value;
  }

}  // namespace ProfileInternal

HPROFILE ProfileCreate() {
  return NEW(ProfileInternal::PROFILE);
}

int ProfileReadFile(HPROFILE handle, const char *path) {
  FATALASSERT(path);

  return ProfileInternal::IReadFile(static_cast<ProfileInternal::PROFILE *>(handle), path);
}

int ProfileWriteFile(HPROFILE handle, const char *path) {
  FATALASSERT(path);

  return ProfileInternal::IWriteFile(static_cast<ProfileInternal::PROFILE *>(handle), path);
}

int ProfileReadBuffer(HPROFILE handle, const void *buffer, unsigned long bufferBytes) {
  return ProfileInternal::IReadBuffer(static_cast<ProfileInternal::PROFILE *>(handle), buffer, bufferBytes);
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, bool value) {
  FATALASSERT(section);
  FATALASSERT(key);

  return ProfileAddValue(handle, section, key, value ? ProfileInternal::TRUESTR : ProfileInternal::FALSESTR);
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, int value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%d", value);
  return ProfileAddValue(handle, section, key, strValue);
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, __int64 value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%I64d", value);
  return ProfileAddValue(handle, section, key, strValue);
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, float value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%f", value);
  return ProfileAddValue(handle, section, key, strValue);
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, const unreal &value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  unreal::asString(value, strValue, 1, -1);
  ProfileInternal::ISetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, strValue, 0, 0);
  return 1;
}

int ProfileAddValue(HPROFILE handle, const char *section, const char *key, const char *value) {
  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  ProfileInternal::ISetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, value, 0, 0);
  return 1;
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, bool value) {
  FATALASSERT(section);
  FATALASSERT(key);

  return ProfileSetValue(handle, section, key, value ? ProfileInternal::TRUESTR : ProfileInternal::FALSESTR);
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, int value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%d", value);
  return ProfileSetValue(handle, section, key, strValue);
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, __int64 value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%I64d", value);
  return ProfileSetValue(handle, section, key, strValue);
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, float value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  SStrPrintf(strValue, sizeof(strValue), "%f", value);
  return ProfileSetValue(handle, section, key, strValue);
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, const unreal &value) {
  FATALASSERT(section);
  FATALASSERT(key);

  char strValue[256];
  unreal::asString(value, strValue, 1, -1);
  ProfileInternal::ISetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, strValue, 1, 0);
  return 1;
}

int ProfileSetValue(HPROFILE handle, const char *section, const char *key, const char *value) {
  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  ProfileInternal::ISetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, value, 1, 0);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, bool *value, unsigned int index) {
  const char *string;

  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  *value = false;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  *value = !SStrCmpI(string, ProfileInternal::TRUESTR, 0x7FFFFFFF);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, int *value, unsigned int index) {
  const char *string;

  FATALASSERT(section);

  FATALASSERT(key);

  FATALASSERT(value);

  *value = 0;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  *value = ProfileInternal::PrfStrToInt(string);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, __int64 *value, unsigned int index) {
  const char *string;

  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  *value = 0;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  *value = SStrToInt64(string);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, float *value, unsigned int index) {
  const char *string;

  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  *value = 0.0f;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  *value = SStrToFloat(string);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, unreal *value, unsigned int index) {
  const char *string;

  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  *value = u_0;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  *value = unreal::fromString(string);
  return 1;
}

int ProfileGetValue(HPROFILE handle, const char *section, const char *key, char *value, unsigned int maxChars, unsigned int index) {
  const char *string;

  FATALASSERT(section);

  FATALASSERT(key);

  FATALASSERT(value);

  value[0] = 0;
  string = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  if (!string) {
    return 0;
  }

  SStrCopy(value, string, maxChars);
  return 1;
}

const char *ProfileGetValueNoCopy(HPROFILE handle, const char *section, const char *key, unsigned int index) {
  FATALASSERT(section);
  FATALASSERT(key);

  return ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
}

unsigned int ProfileGetNumValues(HPROFILE handle, const char *section, const char *key) {
  FATALASSERT(section);
  FATALASSERT(key);

  return ProfileInternal::IGetNumValues(static_cast<ProfileInternal::PROFILE *>(handle), section, key);
}

int ProfileGetValueIndex(HPROFILE handle, const char *section, const char *key, const char *value) {
  unsigned int index;
  const char  *candidate;

  FATALASSERT(section);
  FATALASSERT(key);
  FATALASSERT(value);

  index = 0;
  candidate = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, index);
  while (candidate) {
    if (!SStrCmp(candidate, value, 0x7FFFFFFF)) {
      return index;
    }
    candidate = ProfileInternal::IGetValue(static_cast<ProfileInternal::PROFILE *>(handle), section, key, ++index);
  }
  return -1;
}

void ProfileEnumKeys(HPROFILE handle, const char *sectionName, PROFILEENUMKEYCALLBACK callback, void *opaqueData) {
  ProfileInternal::PROFILE  *profile;
  ProfileInternal::SECTION  *pSection;
  ProfileInternal::KEYVALUE *key;

  profile = static_cast<ProfileInternal::PROFILE *>(handle);
  pSection = profile->sectionTable.Ptr(sectionName);
  FATALASSERT(pSection);

  key = pSection->keyTable.Head();
  while (key) {
    callback(key->GetString(), key->values[0], opaqueData);
    key = pSection->keyTable.Next(key);
  }
}

void ProfileEnumSections(HPROFILE handle, PROFILEENUMSECTIONCALLBACK callback, void *opaqueData) {
  ProfileInternal::PROFILE *profile;
  ProfileInternal::SECTION *section;

  profile = static_cast<ProfileInternal::PROFILE *>(handle);
  section = profile->sectionTable.Head();
  while (section) {
    callback(section->GetString(), opaqueData);
    section = profile->sectionTable.Next(section);
  }
}

int ProfileSectionExists(HPROFILE profile, const char *section) {
  ProfileInternal::PROFILE *profilePtr;

  profilePtr = static_cast<ProfileInternal::PROFILE *>(profile);
  FATALASSERT(profilePtr);

  return profilePtr->sectionTable.Ptr(section) != 0;
}

void ProfileClose(HPROFILE handle) {
  if (handle) {
    DEL(static_cast<ProfileInternal::PROFILE *>(handle));
  }
}
