#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <math.h>
#include <storm.h>

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void ReadFloatKeyData(
    Parser &parse,
    float *entry,
    unsigned int elements
) {
  FATALASSERT(elements > 0);
  if (elements > 1) {
    parse.Expect('{');
  }
  *entry++ = parse.ExpectFloat();
  for (unsigned int i = 1; i < elements; ++i) {
    parse.Expect(',');
    *entry++ = parse.ExpectFloat();
  }
  if (elements > 1) {
    parse.Expect('}');
  }
}

#define READ_FLOAT_TRACK_HEADER(parse, track, tokenText, tokenData, result) \
  do {                                                                      \
    (result) = (parse).Token((tokenText), (tokenData));                      \
    for (;;) {                                                              \
      if ((result) == 0x128) {                                              \
        if (track) {                                                        \
          (track)->type = TRACK_BEZIER;                                     \
        }                                                                   \
      } else if ((result) == 0x140) {                                       \
        if (track) {                                                        \
          (track)->type = TRACK_DONT_INTERP;                                \
        }                                                                   \
      } else if ((result) == 0x152) {                                       \
        if (track) {                                                        \
          (track)->globalSeqId = (parse).ExpectInt();                       \
        } else {                                                            \
          (parse).ExpectInt();                                              \
        }                                                                   \
      } else if ((result) == 0x15B) {                                       \
        if (track) {                                                        \
          (track)->type = TRACK_HERMITE;                                    \
        }                                                                   \
      } else if ((result) == 0x167) {                                       \
        if (track) {                                                        \
          (track)->type = TRACK_LINEAR;                                     \
        }                                                                   \
      } else {                                                              \
        break;                                                              \
      }                                                                     \
      (parse).Expect(',');                                                  \
      (result) = (parse).Token((tokenText), (tokenData));                    \
    }                                                                       \
  } while (0)

unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER(parse, track, tokenText, tokenData, token);
  return token;
}

unsigned int ReadIntTrackHeader(
    Parser &parse,
    MDLSIMPLEKEYTRACK<MDLINTKEY> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token = parse.Token(tokenText, tokenData);
  while (token == 0x140 || token == 0x152) {
    if (token == 0x152) {
      unsigned int globalSeqId = parse.ExpectInt();
      if (track) {
        track->globalSeqId = globalSeqId;
      }
    }
    parse.Expect(',');
    token = parse.Token(tokenText, tokenData);
  }
  return token;
}

unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C4Quaternion> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER(parse, track, tokenText, tokenData, token);
  return token;
}

unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<float> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER(parse, track, tokenText, tokenData, token);
  return token;
}

#define READ_OBJECT_FLOAT_KEYFRAMES(parse, track, elements, keyType)                    \
  do {                                                                                  \
    unsigned int readToken;                                                             \
    const char *readTokenText;                                                          \
    UTokenData readTokenData;                                                           \
    long readExpected = (parse).GetOptionalInt(&readToken, &readTokenText, &readTokenData); \
    if (readExpected > 0 && (track)) {                                                  \
      (track)->keys.Reserve(readExpected);                                              \
    }                                                                                   \
    (parse).Expect('{', readToken, readTokenText);                                      \
    READ_FLOAT_TRACK_HEADER((parse), (track), &readTokenText, &readTokenData, readToken); \
    long readActual = 0;                                                               \
    while (readToken == 0x100) {                                                       \
      MDLKEYFRAME<keyType> *readKey = (track)->keys.New();                             \
      readKey->time = readTokenData.lVal;                                              \
      (parse).Expect(':');                                                              \
      ReadFloatKeyData(                                                                 \
          (parse),                                                                      \
          reinterpret_cast<float *>(&readKey->value),                                  \
          (elements)                                                                    \
      );                                                                                \
      (parse).Expect(',');                                                              \
      ++readActual;                                                                     \
      MDLTRACKTYPE readType = (track)->type;                                            \
      if (readType > TRACK_LINEAR) {                                                    \
        (parse).Expect(0x15E);                                                          \
        ReadFloatKeyData(                                                               \
            (parse),                                                                    \
            reinterpret_cast<float *>(&readKey->inTan),                                \
            (elements)                                                                  \
        );                                                                              \
        (parse).Expect(',');                                                            \
        (parse).Expect(0x18A);                                                          \
        ReadFloatKeyData(                                                               \
            (parse),                                                                    \
            reinterpret_cast<float *>(&readKey->outTan),                               \
            (elements)                                                                  \
        );                                                                              \
        (parse).Expect(',');                                                            \
      }                                                                                 \
      readToken = (parse).Token(&readTokenText, &readTokenData);                        \
    }                                                                                   \
    (parse).Expect('}', readToken, readTokenText);                                      \
    if (readExpected >= 0 && readActual != readExpected) {                              \
      (parse).WarningCount("key frames", readExpected, readActual);                    \
    }                                                                                   \
  } while (0)

void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES(parse, track, 3, NTempest::C3Vector);
}

void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<float> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES(parse, track, 1, float);
}

void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<C3Color> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES(parse, track, 3, C3Color);
}

#undef READ_OBJECT_FLOAT_KEYFRAMES
#undef READ_FLOAT_TRACK_HEADER

const float *WriteKeyData(
    TSGrowableArray<char> &buffer,
    const float *entry,
    unsigned int elements
) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%g,\n", *entry);
    return entry + 1;
  }
  MDL::WriteLine(buffer, "{ %g", *entry++);
  for (unsigned int i = 1; i < elements; ++i) {
    MDL::WriteLine(buffer, ", %g", *entry++);
  }
  MDL::WriteLine(buffer, " },\n");
  return entry;
}

const unsigned int *WriteUintKeyData(
    TSGrowableArray<char> &buffer,
    const unsigned int *entry,
    unsigned int elements
) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%u,\n", *entry);
    return entry + 1;
  }
  MDL::WriteLine(buffer, "{ %u", *entry++);
  for (unsigned int i = 1; i < elements; ++i) {
    MDL::WriteLine(buffer, ", %u", *entry++);
  }
  MDL::WriteLine(buffer, " },\n");
  return entry;
}

void AddObjectErrors(TSet &errors) {
  errors.Add(0x187, 0, 0);
  errors.Add(0x18B, 0, 0);
  errors.Add(0x1A5, 0, 0);
  errors.Add(0x1C7, 0, 0);
  errors.Add(0x1AD, 0, 0);
  errors.Add(0x1AF, 0, 0);
  errors.Add(0x129, 0, 0);
  errors.Add(0x12B, 0, 0);
  errors.Add(0x12C, 0, 0);
  errors.Add(0x1A7, 0, 0);
}

void ReadObjectName(Parser &parse, char *name) {
  FATALASSERT(name);
  const char *value = parse.ExpectString();
  if (value) {
    SStrCopy(name, value, 80);
  }
}

static void AddDontIneritErrors(TSet &errors) {
  errors.Add(0x1C7, 0, 0);
  errors.Add(0x1AD, 0, 0);
  errors.Add(0x1AF, 0, 0);
}

static void IReadDontInherit(
    Parser &parse,
    MDLGENOBJECT *object,
    CMDLStatus *status
) {
  TSet errors;
  AddDontIneritErrors(errors);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  do {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case 0x1AD: object->flags |= 4; break;
      case 0x1AF: object->flags |= 2; break;
      case 0x1C7: object->flags |= 1; break;
      default:
        parse.FatalUnexpected(tokenText);
        break;
    }
  } while (parse.GetOptionalToken(',', &token, &tokenText));
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

#define READ_OBJECT_TRACK(parse, track, elements, item, keyType)                   \
  do {                                                                              \
    unsigned int objectTrackToken;                                                  \
    const char *objectTrackTokenText;                                               \
    UTokenData objectTrackTokenData;                                                \
    long objectTrackExpected =                                                      \
        (parse).GetOptionalInt(&objectTrackToken, &objectTrackTokenText, &objectTrackTokenData); \
    if (objectTrackExpected > 0) {                                                  \
      (track).keys.Reserve(objectTrackExpected);                                    \
    }                                                                               \
    (parse).Expect('{', objectTrackToken, objectTrackTokenText);                    \
    objectTrackToken =                                                             \
        ReadFloatTrackHeader((parse), &(track), &objectTrackTokenText, &objectTrackTokenData); \
    long objectTrackActual = 0;                                                    \
    while (objectTrackToken == 0x100) {                                            \
      MDLKEYFRAME<keyType> *objectTrackKey = (track).keys.New();                   \
      objectTrackKey->time = objectTrackTokenData.lVal;                            \
      (parse).Expect(':');                                                         \
      ReadFloatKeyData(                                                            \
          (parse),                                                                 \
          reinterpret_cast<float *>(&objectTrackKey->value),                       \
          (elements)                                                               \
      );                                                                           \
      (parse).Expect(',');                                                         \
      ++objectTrackActual;                                                         \
      if ((track).type > TRACK_LINEAR) {                                           \
        (parse).Expect(0x15E);                                                     \
        ReadFloatKeyData(                                                          \
            (parse),                                                               \
            reinterpret_cast<float *>(&objectTrackKey->inTan),                     \
            (elements)                                                             \
        );                                                                         \
        (parse).Expect(',');                                                       \
        (parse).Expect(0x18A);                                                     \
        ReadFloatKeyData(                                                          \
            (parse),                                                               \
            reinterpret_cast<float *>(&objectTrackKey->outTan),                    \
            (elements)                                                             \
        );                                                                         \
        (parse).Expect(',');                                                       \
      }                                                                            \
      objectTrackToken = (parse).Token(&objectTrackTokenText, &objectTrackTokenData); \
    }                                                                              \
    (parse).Expect('}', objectTrackToken, objectTrackTokenText);                   \
    if (objectTrackExpected >= 0 && objectTrackActual != objectTrackExpected) {     \
      (parse).WarningCount((item), objectTrackExpected, objectTrackActual);         \
    }                                                                              \
  } while (0)

static void INormalizeQuats(
    TSGrowableArray<MDLKEYFRAME<NTempest::C4Quaternion> > *keys
) {
  for (unsigned int i = 0; i < keys->Count(); ++i) {
    MDLKEYFRAME<NTempest::C4Quaternion> &key = keys->Ptr()[i];
    key.value.Normalize();
    key.inTan.Normalize();
    key.outTan.Normalize();
  }
}

int ReadObjectBody(
    Parser &parse,
    unsigned int savedToken,
    NTempest::C3Vector *pivot,
    MDLGENOBJECT *object,
    CMDLStatus *status
) {
  switch (savedToken) {
    case 0x129:
      object->flags |= 8;
      parse.Expect(',');
      return 1;
    case 0x12A:
      object->flags |= 0x10;
      parse.Expect(',');
      return 1;
    case 0x12B:
      object->flags |= 0x20;
      parse.Expect(',');
      return 1;
    case 0x12C:
      object->flags |= 0x40;
      parse.Expect(',');
      return 1;
    case 0x13F:
      IReadDontInherit(parse, object, status);
      parse.Expect(',');
      return 1;
    case 0x187:
      object->objectId = parse.ExpectInt();
      parse.Expect(',');
      return 1;
    case 0x18B:
      object->parentId = parse.ExpectInt();
      parse.Expect(',');
      return 1;
    case 0x1A5:
      if (!pivot) {
        return 0;
      }
      ReadFloatKeyData(parse, &pivot->x, 3);
      parse.Expect(',');
      return 1;
    case 0x1A7:
      object->flags |= 0x4000;
      parse.Expect(',');
      return 1;
    case 0x1AD:
      READ_OBJECT_TRACK(parse, object->rotkeys, 4, "key frames", NTempest::C4Quaternion);
      INormalizeQuats(&object->rotkeys.keys);
      return 1;
    case 0x1AF:
      ReadObjectFloatKeyframes(parse, &object->scalekeys);
      return 1;
    case 0x1C7:
      ReadObjectFloatKeyframes(parse, &object->transkeys);
      return 1;
    default:
      return 0;
  }
}

#undef READ_OBJECT_TRACK

void ReadObjectEnd(
    TSet &errors,
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
) {
  FATALASSERT(object);
  if (errors.NotFound(0x187)) {
    object->objectId = data.objects.Count();
  }
  if (object->objectId >= data.objects.Count()) {
    unsigned int oldCount = data.objects.Count();
    data.objects.SetCount(object->objectId + 1);
    for (unsigned int i = oldCount; i < data.objects.Count(); ++i) {
      data.objects[i] = 0;
    }
  }
  data.objects[object->objectId] =
      reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

int IExpectAnimation(
    Parser &parse,
    unsigned int *savedToken,
    const char **tokenText
) {
  if (*savedToken == 0x1BB) {
    *savedToken = parse.Token(tokenText, 0);
    return 0;
  }
  return 1;
}

#define WRITE_TRACK_HEADER(indent, type, globalSeqId, buffer)                \
  do {                                                                        \
    unsigned int writeTrackToken = 0;                                         \
    switch (type) {                                                           \
      case TRACK_DONT_INTERP: writeTrackToken = 0x140; break;                \
      case TRACK_LINEAR:      writeTrackToken = 0x167; break;                \
      case TRACK_HERMITE:     writeTrackToken = 0x15B; break;                \
      case TRACK_BEZIER:      writeTrackToken = 0x128; break;                \
      default: break;                                                         \
    }                                                                         \
    if (writeTrackToken) {                                                    \
      MDL::WriteLine((buffer), "%s\t%s,\n", (indent), MDL::TokenText(writeTrackToken)); \
    }                                                                         \
    if ((globalSeqId) != static_cast<unsigned int>(-1)) {                     \
      MDL::WriteLine(                                                         \
          (buffer),                                                           \
          "%s\t%s %d,\n",                                                   \
          (indent),                                                           \
          MDL::TokenText(0x152),                                              \
          (globalSeqId)                                                       \
      );                                                                      \
    }                                                                         \
  } while (0)

void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C3Vector> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER(indent, track.type, track.globalSeqId, buffer);
}

void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER(indent, track.type, track.globalSeqId, buffer);
}

void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<float> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER(indent, track.type, track.globalSeqId, buffer);
}

void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<C3Color> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER(indent, track.type, track.globalSeqId, buffer);
}

void WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<float> &keyframes,
    TSGrowableArray<char> &buffer
) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(
      buffer,
      "%s%s %d {\n",
      indent,
      MDL::TokenText(title),
      keyframes.keys.Count()
  );
  WriteTrackHeader(indent, keyframes, buffer);
  for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
    const MDLKEYFRAME<float> &key = keyframes.keys.Ptr()[i];
    MDL::WriteLine(buffer, "%s\t%d: ", indent, key.time);
    WriteKeyData(buffer, &key.value, 1);
    if (keyframes.type > TRACK_LINEAR) {
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(0x15E));
      WriteKeyData(buffer, &key.inTan, 1);
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(0x18A));
      WriteKeyData(buffer, &key.outTan, 1);
    }
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

void WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<C3Color> &keyframes,
    TSGrowableArray<char> &buffer
) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(
      buffer,
      "%s%s %d {\n",
      indent,
      MDL::TokenText(title),
      keyframes.keys.Count()
  );
  WriteTrackHeader(indent, keyframes, buffer);
  for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
    const MDLKEYFRAME<C3Color> &key = keyframes.keys.Ptr()[i];
    MDL::WriteLine(buffer, "%s\t%d: ", indent, key.time);
    WriteKeyData(buffer, &key.value.b, 3);
    if (keyframes.type > TRACK_LINEAR) {
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(0x15E));
      WriteKeyData(buffer, &key.inTan.b, 3);
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(0x18A));
      WriteKeyData(buffer, &key.outTan.b, 3);
    }
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

void WriteIntKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes,
    TSGrowableArray<char> &buffer
) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(
      buffer,
      "%s%s %u {\n",
      indent,
      MDL::TokenText(title),
      keyframes.keys.Count()
  );
  if (keyframes.globalSeqId != static_cast<unsigned int>(-1)) {
    MDL::WriteLine(
        buffer,
        "%s\t%s %d,\n",
        indent,
        MDL::TokenText(0x152),
        keyframes.globalSeqId
    );
  }
  for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
    const MDLINTKEY &key = keyframes.keys.Ptr()[i];
    MDL::WriteLine(buffer, "%s\t%d: %u,\n", indent, key.time, key.value);
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

#define WRITE_TRACK(title, track, elements, buffer, keyType)                     \
  do {                                                                            \
    if ((track).keys.Count()) {                                                   \
      MDL::WriteLine(                                                             \
          (buffer),                                                               \
          "\t%s %d {\n",                                                        \
          MDL::TokenText((title)),                                                \
          (track).keys.Count()                                                    \
      );                                                                          \
      WriteTrackHeader("\t", (track), (buffer));                                \
      for (unsigned int writeTrackIndex = 0;                                      \
           writeTrackIndex < (track).keys.Count();                                \
           ++writeTrackIndex) {                                                   \
        const MDLKEYFRAME<keyType> &writeTrackKey = (track).keys[writeTrackIndex]; \
        MDL::WriteLine((buffer), "\t\t%d: ", writeTrackKey.time);                \
        const float *writeTrackNext = WriteKeyData(                               \
            (buffer),                                                             \
            reinterpret_cast<const float *>(&writeTrackKey.value),                \
            (elements)                                                            \
        );                                                                        \
        if ((track).type > TRACK_LINEAR) {                                        \
          MDL::WriteLine((buffer), "\t\t\t%s ", MDL::TokenText(0x15E));         \
          writeTrackNext = WriteKeyData((buffer), writeTrackNext, (elements));     \
          MDL::WriteLine((buffer), "\t\t\t%s ", MDL::TokenText(0x18A));         \
          WriteKeyData((buffer), writeTrackNext, (elements));                     \
        }                                                                         \
      }                                                                           \
      MDL::WriteLine((buffer), "\t}\n");                                         \
    }                                                                             \
  } while (0)

void WriteObjectTrailer(
    const MDLGENOBJECT &object,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK(0x1C7, object.transkeys, 3, buffer, NTempest::C3Vector);
  WRITE_TRACK(0x1AD, object.rotkeys, 4, buffer, NTempest::C4Quaternion);
  WRITE_TRACK(0x1AF, object.scalekeys, 3, buffer, NTempest::C3Vector);
  MDL::WriteLine(buffer, "}\n");
}

#undef WRITE_TRACK
#undef WRITE_TRACK_HEADER

static void IWriteObjectFlags(
    unsigned int flags,
    TSGrowableArray<char> &buffer
) {
  static const unsigned int masks[] = { 8, 16, 32, 64, 0x4000 };
  static const unsigned int tokens[] = { 0x129, 0x12A, 0x12B, 0x12C, 0x1A7 };
  for (unsigned int i = 0; i < 5; ++i) {
    if (flags & masks[i]) {
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(tokens[i]));
    }
  }
}

void WriteObjectHeader(
    const MDLDATA &data,
    const MDLGENOBJECT &object,
    unsigned int title,
    int writeIndex,
    TSGrowableArray<char> &buffer
) {
  MDL::WriteLine(
      buffer,
      "%s \"%s\" {\n",
      MDL::TokenText(title),
      static_cast<const char *>(object.name)
  );
  if (writeIndex) {
    MDL::WriteLine(
        buffer,
        "\t%s %d,\n",
        MDL::TokenText(0x187),
        object.objectId
    );
  }
  if (object.parentId != static_cast<unsigned int>(-1)) {
    MDL::WriteLine(
        buffer,
        "\t%s %d,\t// \"%s\"\n",
        MDL::TokenText(0x18B),
        object.parentId,
        static_cast<const char *>(data.objects[object.parentId]->name)
    );
  }
  IWriteObjectFlags(object.flags, buffer);
  if (object.flags & 7) {
    MDL::WriteLine(buffer, "\t%s { ", MDL::TokenText(0x13F));
    int needComma = 0;
    if (object.flags & 1) {
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1C7));
      needComma = 1;
    }
    if (object.flags & 2) {
      if (needComma) MDL::WriteLine(buffer, ", ");
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AF));
      needComma = 1;
    }
    if (object.flags & 4) {
      if (needComma) MDL::WriteLine(buffer, ", ");
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AD));
    }
    MDL::WriteLine(buffer, " },\n");
  }
}

void WriteOptionalVertex(
    unsigned int token,
    const char *indent,
    const NTempest::C3Vector &vertex,
    TSGrowableArray<char> &buffer
) {
  if (fabs(vertex.x) >= 2.3841858e-7f
      || fabs(vertex.y) >= 2.3841858e-7f
      || fabs(vertex.z) >= 2.3841858e-7f) {
    MDL::WriteLine(
        buffer, "%s%s { %g, %g, %g },\n",
        indent, MDL::TokenText(token), vertex.x, vertex.y, vertex.z
    );
  }
}

void WriteOptionalFloat(
    unsigned int token,
    const char *indent,
    float value,
    TSGrowableArray<char> &buffer
) {
  if (fabs(value) >= 2.3841858e-7f) {
    MDL::WriteLine(buffer, "%s%s %g,\n", indent, MDL::TokenText(token), value);
  }
}

void WriteBounds(
    const CMdlBounds &bounds,
    const char *indent,
    TSGrowableArray<char> &buffer
) {
  WriteOptionalVertex(0x170, indent, bounds.extent.b, buffer);
  WriteOptionalVertex(0x16F, indent, bounds.extent.t, buffer);
  WriteOptionalFloat(0x134, indent, bounds.radius, buffer);
}

void SkipUnknown(CMsgBuffer &buffer, unsigned int &totalRead) {
  unsigned int size = buffer.GetUint();
  totalRead += 4;
  unsigned int bytes = size - 4;
  if (bytes > static_cast<unsigned int>(buffer.Bytes())) {
    bytes = buffer.Bytes();
  }
  buffer.GetData(bytes);
  totalRead += bytes;
}

unsigned int GetBinQuatKeyFramesSize(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track
) {
  if (!track.keys.Count()) {
    return 0;
  }
  unsigned int dataSize = track.type > TRACK_LINEAR ? 24 : 8;
  return 16 + track.keys.Count() * (4 + dataSize);
}

#define READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, elements, keyType)        \
  do {                                                                                \
    if ((buffer).Bytes() < 12) {                                                      \
      return 0;                                                                       \
    }                                                                                 \
    unsigned int binFloatCount = (buffer).GetUint();                                  \
    (totalRead) += 4;                                                                 \
    if (!binFloatCount) {                                                             \
      return 0;                                                                       \
    }                                                                                 \
    (track).type = static_cast<MDLTRACKTYPE>((buffer).GetUint());                     \
    (track).globalSeqId = (buffer).GetUint();                                         \
    (totalRead) += 8;                                                                 \
    (track).keys.SetCount(binFloatCount);                                             \
    unsigned int binFloatValues =                                                     \
        (track).type > TRACK_LINEAR ? (elements) * 3 : (elements);                   \
    unsigned int binFloatBytesPerKey = 4 + binFloatValues * 4;                       \
    if (binFloatCount * binFloatBytesPerKey                                           \
        > static_cast<unsigned int>((buffer).Bytes())) {                              \
      return 0;                                                                       \
    }                                                                                 \
    for (unsigned int binFloatIndex = 0; binFloatIndex < binFloatCount; ++binFloatIndex) { \
      MDLKEYFRAME<keyType> &binFloatKey = (track).keys.Ptr()[binFloatIndex];          \
      binFloatKey.time = (buffer).GetInt();                                           \
      (buffer).GetFloatArray(reinterpret_cast<float *>(&binFloatKey.value), binFloatValues); \
      (totalRead) += binFloatBytesPerKey;                                             \
    }                                                                                 \
    return 1;                                                                         \
  } while (0)

void WriteBinFloatKeyFrames(
    const MDLKEYTRACK<float> &track,
    unsigned long magic,
    CMsgBuffer &buffer
) {
  if (!track.keys.Count()) {
    return;
  }
  buffer.AddDword(magic);
  buffer.AddUint(track.keys.Count());
  buffer.AddUint(track.type);
  buffer.AddUint(track.globalSeqId);
  unsigned int values = track.type > TRACK_LINEAR ? 3 : 1;
  for (unsigned int i = 0; i < track.keys.Count(); ++i) {
    const MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
    buffer.AddInt(key.time);
    buffer.AddFloatArray(&key.value, values);
  }
}

int ReadBinFloatKeyFrames(
    MDLKEYTRACK<float> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 1, float);
}

int ReadBinFloatKeyFrames(
    MDLKEYTRACK<C3Color> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 3, C3Color);
}

int ReadBinFloatKeyFrames(
    MDLKEYTRACK<NTempest::C3Vector> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 3, NTempest::C3Vector);
}

#undef READ_BIN_FLOAT_KEYFRAMES

int ReadBinUintKeyFrames(
    MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
) {
  if (buffer.Bytes() < 8) {
    return 0;
  }
  unsigned int count = buffer.GetUint();
  totalRead += 4;
  if (!count) {
    return 0;
  }
  buffer.GetUint();
  track.globalSeqId = buffer.GetUint();
  totalRead += 8;
  if (count * 8 > static_cast<unsigned int>(buffer.Bytes())) {
    return 0;
  }
  track.keys.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    MDLINTKEY &key = track.keys.Ptr()[i];
    key.time = buffer.GetInt();
    key.value = buffer.GetUint();
    totalRead += 8;
  }
  return 1;
}

void WriteBinUintKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    unsigned long magic,
    CMsgBuffer &buffer
) {
  if (!track.keys.Count()) {
    return;
  }
  buffer.AddDword(magic);
  buffer.AddUint(track.keys.Count());
  buffer.AddUint(0);
  buffer.AddUint(track.globalSeqId);
  for (unsigned int i = 0; i < track.keys.Count(); ++i) {
    const MDLINTKEY &key = track.keys.Ptr()[i];
    buffer.AddInt(key.time);
    buffer.AddUintArray(&key.value, 1);
  }
}

unsigned int GetBinGenObjectSize(const MDLGENOBJECT &object) {
  unsigned int size = 96;
  if (object.transkeys.keys.Count()) {
    unsigned int dataSize = object.transkeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + object.transkeys.keys.Count() * (4 + dataSize);
  }
  size += GetBinQuatKeyFramesSize(object.rotkeys);
  if (object.scalekeys.keys.Count()) {
    unsigned int dataSize = object.scalekeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + object.scalekeys.keys.Count() * (4 + dataSize);
  }
  return size;
}

void WriteBinQuatKeyFrames(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    unsigned long magic,
    CMsgBuffer &buffer
) {
  if (!track.keys.Count()) {
    return;
  }
  buffer.AddDword(magic);
  buffer.AddUint(track.keys.Count());
  buffer.AddUint(track.type);
  buffer.AddUint(track.globalSeqId);

  for (unsigned int i = 0; i < track.keys.Count(); ++i) {
    const MDLKEYFRAME<NTempest::C4Quaternion> &key = track.keys[i];
    buffer.AddInt(key.time);

    const NTempest::C4Quaternion *quaternion = &key.value;
    unsigned int values = track.type > TRACK_LINEAR ? 3 : 1;
    for (unsigned int j = 0; j < values; ++j, ++quaternion) {
      int sign = quaternion->w >= 0.0f ? 1 : -1;
      __int64 x = static_cast<__int64>(quaternion->x * 2097152.0f);
      __int64 y = static_cast<__int64>(quaternion->y * 1048576.0f);
      __int64 z = static_cast<__int64>(quaternion->z * 1048576.0f);
      x *= sign;
      y *= sign;
      z *= sign;
      unsigned __int64 packed =
          (static_cast<unsigned __int64>(x) << 42)
          | ((static_cast<unsigned __int64>(y) & 0x1FFFFF) << 21)
          | (static_cast<unsigned __int64>(z) & 0x1FFFFF);
      buffer.AddLongLong(packed);
    }
  }
}

int ReadBinQuatKeyFrames(
    MDLKEYTRACK<NTempest::C4Quaternion> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
) {
  if (buffer.Bytes() < 12) {
    return 0;
  }
  unsigned int count = buffer.GetUint();
  totalRead += 4;
  if (!count) {
    return 0;
  }
  track.type = static_cast<MDLTRACKTYPE>(buffer.GetUint());
  track.globalSeqId = buffer.GetUint();
  totalRead += 8;
  track.keys.SetCount(count);

  unsigned int values = track.type > TRACK_LINEAR ? 3 : 1;
  unsigned int bytesPerKey = 4 + values * 8;
  if (count * bytesPerKey > static_cast<unsigned int>(buffer.Bytes())) {
    return 0;
  }

  for (unsigned int i = 0; i < count; ++i) {
    MDLKEYFRAME<NTempest::C4Quaternion> &key = track.keys[i];
    key.time = buffer.GetInt();
    totalRead += 4;
    NTempest::C4Quaternion *quaternion = &key.value;
    for (unsigned int j = 0; j < values; ++j, ++quaternion) {
      unsigned __int64 packed = buffer.GetUlongLong();
      int z = static_cast<int>(static_cast<__int64>(packed << 43) >> 43);
      int y = static_cast<int>(
          static_cast<__int64>(((packed >> 21) & 0x1FFFFF) << 43) >> 43
      );
      int x = static_cast<int>(static_cast<__int64>(packed) >> 42);
      quaternion->x = x * 0.00000047683716f;
      quaternion->y = y * 0.00000095367432f;
      quaternion->z = z * 0.00000095367432f;
      float square = quaternion->x * quaternion->x
          + quaternion->y * quaternion->y
          + quaternion->z * quaternion->z;
      quaternion->w = fabs(square - 1.0f) >= 0.00000095367432f
          ? static_cast<float>(sqrt(1.0f - square))
          : 0.0f;
      totalRead += 8;
    }
  }
  return 1;
}

int WriteBinGenObject(
    const MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *
) {
  buffer.AddUint(GetBinGenObjectSize(object));
  buffer.AddTcharArray(object.name, 80, 1);
  buffer.AddUint(object.objectId);
  buffer.AddUint(object.parentId);
  buffer.AddUint(object.flags);

  if (object.transkeys.keys.Count()) {
    buffer.AddDword('RTGK');
    buffer.AddUint(object.transkeys.keys.Count());
    buffer.AddUint(object.transkeys.type);
    buffer.AddUint(object.transkeys.globalSeqId);
    unsigned int values = object.transkeys.type > TRACK_LINEAR ? 9 : 3;
    for (unsigned int i = 0; i < object.transkeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = object.transkeys.keys[i];
      buffer.AddInt(key.time);
      buffer.AddFloatArray(&key.value.x, values);
    }
  }
  WriteBinQuatKeyFrames(object.rotkeys, 'RTRK', buffer);
  if (object.scalekeys.keys.Count()) {
    buffer.AddDword('CSGK');
    buffer.AddUint(object.scalekeys.keys.Count());
    buffer.AddUint(object.scalekeys.type);
    buffer.AddUint(object.scalekeys.globalSeqId);
    unsigned int values = object.scalekeys.type > TRACK_LINEAR ? 9 : 3;
    for (unsigned int i = 0; i < object.scalekeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = object.scalekeys.keys[i];
      buffer.AddInt(key.time);
      buffer.AddFloatArray(&key.value.x, values);
    }
  }
  return 1;
}

int ReadBinGenObject(
    MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *status,
    unsigned int &totalRead
) {
  if (!status) {
    return 0;
  }
  unsigned int totalSize = buffer.GetUint();
  buffer.GetTcharArray(object.name, 80);
  object.objectId = buffer.GetUint();
  object.parentId = buffer.GetUint();
  object.flags = buffer.GetUint();
  unsigned int localRead = 96;

  while (localRead < totalSize) {
    unsigned long tag = buffer.GetDword();
    localRead += 4;
    if (tag == 'RTGK' || tag == 'CSGK') {
      MDLKEYTRACK<NTempest::C3Vector> &track =
          tag == 'RTGK' ? object.transkeys : object.scalekeys;
      if (buffer.Bytes() < 12) {
        status->Add(
            STATUS_ERROR,
            tag == 'RTGK' ? "Could not read Translation keyframes.\n"
                          : "Could not read Scaling keyframes.\n"
        );
        return 0;
      }
      unsigned int count = buffer.GetUint();
      localRead += 4;
      if (!count) {
        status->Add(
            STATUS_ERROR,
            tag == 'RTGK' ? "Could not read Translation keyframes.\n"
                          : "Could not read Scaling keyframes.\n"
        );
        return 0;
      }
      track.type = static_cast<MDLTRACKTYPE>(buffer.GetUint());
      track.globalSeqId = buffer.GetUint();
      localRead += 8;
      track.keys.SetCount(count);
      unsigned int values = track.type > TRACK_LINEAR ? 9 : 3;
      unsigned int bytesPerKey = 4 + 4 * values;
      if (count * bytesPerKey > static_cast<unsigned int>(buffer.Bytes())) {
        status->Add(
            STATUS_ERROR,
            tag == 'RTGK' ? "Could not read Translation keyframes.\n"
                          : "Could not read Scaling keyframes.\n"
        );
        return 0;
      }
      for (unsigned int i = 0; i < count; ++i) {
        track.keys[i].time = buffer.GetInt();
        buffer.GetFloatArray(&track.keys[i].value.x, values);
        localRead += bytesPerKey;
      }
    } else if (tag == 'RTRK') {
      if (!ReadBinQuatKeyFrames(object.rotkeys, buffer, localRead)) {
        status->Add(STATUS_ERROR, "Could not read Rotation keyframes.\n");
        return 0;
      }
    } else {
      SkipUnknown(buffer, localRead);
    }
    if (localRead > totalSize) {
      status->FatalOverran("Generic Object", -1);
      return 0;
    }
  }
  totalRead += localRead;
  return 1;
}

void ReadBinObjectEnd(
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
) {
  FATALASSERT(object);

  object->objectId = data.objects.Count();
  if (object->objectId >= data.objects.Count()) {
    unsigned int oldCount = data.objects.Count();
    data.objects.SetCount(object->objectId + 1);
    for (unsigned int i = oldCount; i < data.objects.Count(); ++i) {
      data.objects[i] = 0;
    }
  }
  data.objects[object->objectId] =
      reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

int ReadObjectPtrs(MDLDATA *data, CMDLStatus *status) {
  for (unsigned int i = 0; i < data->objects.Count(); ++i) {
    unsigned int encoded = reinterpret_cast<unsigned int>(data->objects[i]);
    unsigned int index = encoded & 0x0FFFFFFF;

    switch (encoded & 0xF0000000) {
      case 0x10000000:
        data->objects[i] = &data->helpers[index];
        break;
      case 0x20000000:
        data->objects[i] = &data->lights[index];
        break;
      case 0x30000000:
        data->objects[i] = &data->bones[index];
        break;
      case 0x40000000:
        data->objects[i] = &data->attachments[index];
        break;
      case 0x50000000:
        data->objects[i] = &data->particleEmitters[index];
        break;
      case 0x60000000:
        data->objects[i] = &data->events[index];
        break;
      case 0x70000000:
        data->objects[i] = &data->particleEmitters2[index];
        break;
      case 0x80000000:
        data->objects[i] = &data->hitTestShapes[index];
        break;
      case 0x90000000:
        data->objects[i] = &data->ribbonEmitters[index];
        break;
      default:
        status->Add(STATUS_FATAL, "Found gaps in sequence of object ID's. Unable to fix up object pointers.\n");
        return 0;
    }
  }
  return 1;
}

void MDLBASE::RebuildObjectPtrs() {
  unsigned int i;
  for (i = 0; i < bones.Count(); ++i) {
    objects[bones[i].objectId] = &bones[i];
  }
  for (i = 0; i < lights.Count(); ++i) {
    objects[lights[i].objectId] = &lights[i];
  }
  for (i = 0; i < helpers.Count(); ++i) {
    objects[helpers[i].objectId] = &helpers[i];
  }
  for (i = 0; i < attachments.Count(); ++i) {
    objects[attachments[i].objectId] = &attachments[i];
  }
  for (i = 0; i < particleEmitters.Count(); ++i) {
    objects[particleEmitters[i].objectId] = &particleEmitters[i];
  }
  for (i = 0; i < events.Count(); ++i) {
    objects[events[i].objectId] = &events[i];
  }
  for (i = 0; i < particleEmitters2.Count(); ++i) {
    objects[particleEmitters2[i].objectId] = &particleEmitters2[i];
  }
  for (i = 0; i < hitTestShapes.Count(); ++i) {
    objects[hitTestShapes[i].objectId] = &hitTestShapes[i];
  }
  for (i = 0; i < ribbonEmitters.Count(); ++i) {
    objects[ribbonEmitters[i].objectId] = &ribbonEmitters[i];
  }
}
