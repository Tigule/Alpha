#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <math.h>
#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void ReadFloatKeyData(Parser &parse, float *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements > 1) {
    parse.Expect('{');
  }
  *entry++ = parse.ExpectFloat();
  for (UINT i = 1; i < elements; ++i) {
    parse.Expect(',');
    *entry++ = parse.ExpectFloat();
  }
  if (elements > 1) {
    parse.Expect('}');
  }
}

UINT ReadIntTrackHeader(Parser &parse, MDLSIMPLEKEYTRACK<MDLINTKEY> *track, LPCSTR *tokenText, UTokenData *tokenData) {
  UINT token = parse.Token(tokenText, tokenData);
  while (token == 0x140 || token == 0x152) {
    if (token == 0x152) {
      UINT globalSeqId = parse.ExpectInt();
      if (track) {
        track->globalSeqId = globalSeqId;
      }
    }
    parse.Expect(',');
    token = parse.Token(tokenText, tokenData);
  }
  return token;
}

const float *WriteKeyData(TSGrowableArray<char> &buffer, const float *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%g,\n", *entry);
    return entry + 1;
  }
  MDL::WriteLine(buffer, "{ %g", *entry++);
  for (UINT i = 1; i < elements; ++i) {
    MDL::WriteLine(buffer, ", %g", *entry++);
  }
  MDL::WriteLine(buffer, " },\n");
  return entry;
}

const UINT *WriteUintKeyData(TSGrowableArray<char> &buffer, const UINT *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%u,\n", *entry);
    return entry + 1;
  }
  MDL::WriteLine(buffer, "{ %u", *entry++);
  for (UINT i = 1; i < elements; ++i) {
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
  LPCSTR value = parse.ExpectString();
  if (value) {
    SStrCopy(name, value, 80);
  }
}

static void AddDontIneritErrors(TSet &errors) {
  errors.Add(0x1C7, 0, 0);
  errors.Add(0x1AD, 0, 0);
  errors.Add(0x1AF, 0, 0);
}

static void IReadDontInherit(Parser &parse, MDLGENOBJECT *obj, CMDLStatus *status) {
  TSet errors;
  AddDontIneritErrors(errors);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   savedtoken = parse.Token(&tokentext, 0);
  do {
    if (!errors.Check(savedtoken)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (savedtoken) {
      case 0x1AD:
        obj->flags |= 4;
        break;
      case 0x1AF:
        obj->flags |= 2;
        break;
      case 0x1C7:
        obj->flags |= 1;
        break;
      default:
        parse.FatalUnexpected(tokentext);
        break;
    }
  } while (parse.GetOptionalToken(',', &savedtoken, &tokentext));
  parse.Expect('}', savedtoken, tokentext);
  errors.Complete(status);
}

#define READ_OBJECT_TRACK(parse, track, elements, item, keyType)                                                              \
  do {                                                                                                                        \
    UINT       objectTrackToken;                                                                                              \
    LPCSTR     objectTrackTokenText;                                                                                          \
    UTokenData objectTrackTokenData;                                                                                          \
    long       objectTrackExpected = (parse).GetOptionalInt(&objectTrackToken, &objectTrackTokenText, &objectTrackTokenData); \
    if (objectTrackExpected > 0) {                                                                                            \
      (track).keys.ReserveSpace(objectTrackExpected);                                                                         \
    }                                                                                                                         \
    (parse).Expect('{', objectTrackToken, objectTrackTokenText);                                                              \
    objectTrackToken = ReadFloatTrackHeader((parse), &(track), &objectTrackTokenText, &objectTrackTokenData);                 \
    long objectTrackActual = 0;                                                                                               \
    while (objectTrackToken == 0x100) {                                                                                       \
      MDLKEYFRAME<keyType> *objectTrackKey = (track).keys.New();                                                              \
      objectTrackKey->time = objectTrackTokenData.lVal;                                                                       \
      (parse).Expect(':');                                                                                                    \
      ReadFloatKeyData((parse), reinterpret_cast<float *>(&objectTrackKey->value), (elements));                               \
      (parse).Expect(',');                                                                                                    \
      ++objectTrackActual;                                                                                                    \
      if ((track).type > TRACK_LINEAR) {                                                                                      \
        (parse).Expect(0x15E);                                                                                                \
        ReadFloatKeyData((parse), reinterpret_cast<float *>(&objectTrackKey->inTan), (elements));                             \
        (parse).Expect(',');                                                                                                  \
        (parse).Expect(0x18A);                                                                                                \
        ReadFloatKeyData((parse), reinterpret_cast<float *>(&objectTrackKey->outTan), (elements));                            \
        (parse).Expect(',');                                                                                                  \
      }                                                                                                                       \
      objectTrackToken = (parse).Token(&objectTrackTokenText, &objectTrackTokenData);                                         \
    }                                                                                                                         \
    (parse).Expect('}', objectTrackToken, objectTrackTokenText);                                                              \
    if (objectTrackExpected >= 0 && objectTrackActual != objectTrackExpected) {                                               \
      (parse).WarningCount((item), objectTrackExpected, objectTrackActual);                                                   \
    }                                                                                                                         \
  } while (0)

static void INormalizeQuats(TSGrowableArray<MDLKEYFRAME<NTempest::C4Quaternion> > *keyframes) {
  for (UINT i = 0; i < keyframes->Count(); ++i) {
    MDLKEYFRAME<NTempest::C4Quaternion> &key = keyframes->Ptr()[i];
    key.value.Normalize();
    key.inTan.Normalize();
    key.outTan.Normalize();
  }
}

int ReadObjectBody(Parser &parse, UINT savedtoken, NTempest::C3Vector *pivot, MDLGENOBJECT *obj, CMDLStatus *status) {
  switch (savedtoken) {
    case 0x129:
      obj->flags |= 8;
      parse.Expect(',');
      return 1;
    case 0x12A:
      obj->flags |= 0x10;
      parse.Expect(',');
      return 1;
    case 0x12B:
      obj->flags |= 0x20;
      parse.Expect(',');
      return 1;
    case 0x12C:
      obj->flags |= 0x40;
      parse.Expect(',');
      return 1;
    case 0x13F:
      IReadDontInherit(parse, obj, status);
      parse.Expect(',');
      return 1;
    case 0x187:
      obj->objectId = parse.ExpectInt();
      parse.Expect(',');
      return 1;
    case 0x18B:
      obj->parentId = parse.ExpectInt();
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
      obj->flags |= 0x4000;
      parse.Expect(',');
      return 1;
    case 0x1AD:
      READ_OBJECT_TRACK(parse, obj->rotkeys, 4, "key frames", NTempest::C4Quaternion);
      INormalizeQuats(&obj->rotkeys.keys);
      return 1;
    case 0x1AF:
      ReadObjectFloatKeyframes(parse, &obj->scalekeys);
      return 1;
    case 0x1C7:
      ReadObjectFloatKeyframes(parse, &obj->transkeys);
      return 1;
    default:
      return 0;
  }
}

#undef READ_OBJECT_TRACK

void ReadObjectEnd(TSet &errors, MDLDATA &data, MDLGENOBJECT *object, DWORD listIndex, DWORD listMask) {
  FATALASSERT(object);
  if (errors.NotFound(0x187)) {
    object->objectId = data.objects.Count();
  }
  if (object->objectId >= data.objects.Count()) {
    UINT oldCount = data.objects.Count();
    data.objects.SetCount(object->objectId + 1);
    for (UINT i = oldCount; i < data.objects.Count(); ++i) {
      data.objects[i] = 0;
    }
  }
  data.objects[object->objectId] = reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

int IExpectAnimation(Parser &parse, UINT *savedtoken, LPCSTR *tokenText) {
  if (*savedtoken == 0x1BB) {
    *savedtoken = parse.Token(tokenText, 0);
    return 0;
  }
  return 1;
}

void WriteFloatKeyFrames(UINT title, LPCSTR indent, const MDLKEYTRACK<float> &keyframes, TSGrowableArray<char> &buffer) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(buffer, "%s%s %d {\n", indent, MDL::TokenText(title), keyframes.keys.Count());
  WriteTrackHeader(indent, keyframes, buffer);
  for (UINT i = 0; i < keyframes.keys.Count(); ++i) {
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

void WriteFloatKeyFrames(UINT title, LPCSTR indent, const MDLKEYTRACK<C3Color> &keyframes, TSGrowableArray<char> &buffer) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(buffer, "%s%s %d {\n", indent, MDL::TokenText(title), keyframes.keys.Count());
  WriteTrackHeader(indent, keyframes, buffer);
  for (UINT i = 0; i < keyframes.keys.Count(); ++i) {
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

void WriteIntKeyFrames(UINT title, LPCSTR indent, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, TSGrowableArray<char> &buffer) {
  if (!keyframes.keys.Count()) {
    return;
  }
  MDL::WriteLine(buffer, "%s%s %u {\n", indent, MDL::TokenText(title), keyframes.keys.Count());
  if (keyframes.globalSeqId != static_cast<UINT>(-1)) {
    MDL::WriteLine(buffer, "%s\t%s %d,\n", indent, MDL::TokenText(0x152), keyframes.globalSeqId);
  }
  for (UINT i = 0; i < keyframes.keys.Count(); ++i) {
    const MDLINTKEY &key = keyframes.keys.Ptr()[i];
    MDL::WriteLine(buffer, "%s\t%d: %u,\n", indent, key.time, key.value);
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

#define WRITE_TRACK(title, track, elements, buffer, keyType)                                                                     \
  do {                                                                                                                           \
    if ((track).keys.Count()) {                                                                                                  \
      MDL::WriteLine((buffer), "\t%s %d {\n", MDL::TokenText((title)), (track).keys.Count());                                    \
      WriteTrackHeader("\t", (track), (buffer));                                                                                 \
      for (UINT writeTrackIndex = 0; writeTrackIndex < (track).keys.Count(); ++writeTrackIndex) {                                \
        const MDLKEYFRAME<keyType> &writeTrackKey = (track).keys[writeTrackIndex];                                               \
        MDL::WriteLine((buffer), "\t\t%d: ", writeTrackKey.time);                                                                \
        const float *writeTrackNext = WriteKeyData((buffer), reinterpret_cast<const float *>(&writeTrackKey.value), (elements)); \
        if ((track).type > TRACK_LINEAR) {                                                                                       \
          MDL::WriteLine((buffer), "\t\t\t%s ", MDL::TokenText(0x15E));                                                          \
          writeTrackNext = WriteKeyData((buffer), writeTrackNext, (elements));                                                   \
          MDL::WriteLine((buffer), "\t\t\t%s ", MDL::TokenText(0x18A));                                                          \
          WriteKeyData((buffer), writeTrackNext, (elements));                                                                    \
        }                                                                                                                        \
      }                                                                                                                          \
      MDL::WriteLine((buffer), "\t}\n");                                                                                         \
    }                                                                                                                            \
  } while (0)

void WriteObjectTrailer(const MDLGENOBJECT &obj, TSGrowableArray<char> &buffer) {
  WRITE_TRACK(0x1C7, obj.transkeys, 3, buffer, NTempest::C3Vector);
  WRITE_TRACK(0x1AD, obj.rotkeys, 4, buffer, NTempest::C4Quaternion);
  WRITE_TRACK(0x1AF, obj.scalekeys, 3, buffer, NTempest::C3Vector);
  MDL::WriteLine(buffer, "}\n");
}

#undef WRITE_TRACK

static void IWriteObjectFlags(UINT flags, TSGrowableArray<char> &buffer) {
  static const UINT masks[] = {8, 16, 32, 64, 0x4000};
  static const UINT tokens[] = {0x129, 0x12A, 0x12B, 0x12C, 0x1A7};
  for (UINT i = 0; i < 5; ++i) {
    if (flags & masks[i]) {
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(tokens[i]));
    }
  }
}

void WriteObjectHeader(const MDLDATA &data, const MDLGENOBJECT &obj, UINT title, int writeIndex, TSGrowableArray<char> &buffer) {
  MDL::WriteLine(buffer, "%s \"%s\" {\n", MDL::TokenText(title), static_cast<LPCSTR>(obj.name));
  if (writeIndex) {
    MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(0x187), obj.objectId);
  }
  if (obj.parentId != static_cast<UINT>(-1)) {
    MDL::WriteLine(buffer, "\t%s %d,\t// \"%s\"\n", MDL::TokenText(0x18B), obj.parentId, static_cast<LPCSTR>(data.objects[obj.parentId]->name));
  }
  IWriteObjectFlags(obj.flags, buffer);
  if (obj.flags & 7) {
    MDL::WriteLine(buffer, "\t%s { ", MDL::TokenText(0x13F));
    int needComma = 0;
    if (obj.flags & 1) {
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1C7));
      needComma = 1;
    }
    if (obj.flags & 2) {
      if (needComma)
        MDL::WriteLine(buffer, ", ");
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AF));
      needComma = 1;
    }
    if (obj.flags & 4) {
      if (needComma)
        MDL::WriteLine(buffer, ", ");
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AD));
    }
    MDL::WriteLine(buffer, " },\n");
  }
}

void WriteOptionalVertex(UINT title, LPCSTR indent, const NTempest::C3Vector &vertex, TSGrowableArray<char> &buffer) {
  if (fabs(vertex.x) >= 2.3841858e-7f || fabs(vertex.y) >= 2.3841858e-7f || fabs(vertex.z) >= 2.3841858e-7f) {
    MDL::WriteLine(buffer, "%s%s { %g, %g, %g },\n", indent, MDL::TokenText(title), vertex.x, vertex.y, vertex.z);
  }
}

void WriteOptionalFloat(UINT title, LPCSTR indent, float value, TSGrowableArray<char> &buffer) {
  if (fabs(value) >= 2.3841858e-7f) {
    MDL::WriteLine(buffer, "%s%s %g,\n", indent, MDL::TokenText(title), value);
  }
}

void WriteBounds(const CMdlBounds &bounds, LPCSTR indent, TSGrowableArray<char> &buffer) {
  WriteOptionalVertex(0x170, indent, bounds.extent.b, buffer);
  WriteOptionalVertex(0x16F, indent, bounds.extent.t, buffer);
  WriteOptionalFloat(0x134, indent, bounds.radius, buffer);
}

void SkipUnknown(CMsgBuffer &buf, UINT &totalRead) {
  UINT size = buf.GetUint();
  totalRead += 4;
  UINT bytes = size - 4;
  if (bytes > static_cast<UINT>(buf.Bytes())) {
    bytes = buf.Bytes();
  }
  buf.GetData(bytes);
  totalRead += bytes;
}

UINT GetBinQuatKeyFramesSize(const MDLKEYTRACK<NTempest::C4Quaternion> &keyframes) {
  if (!keyframes.keys.Count()) {
    return 0;
  }
  UINT dataSize = keyframes.type > TRACK_LINEAR ? 24 : 8;
  return 16 + keyframes.keys.Count() * (4 + dataSize);
}

#define READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, elements, keyType)                \
  do {                                                                                       \
    if ((buffer).Bytes() < 12) {                                                             \
      return 0;                                                                              \
    }                                                                                        \
    UINT binFloatCount = (buffer).GetUint();                                                 \
    (totalRead) += 4;                                                                        \
    if (!binFloatCount) {                                                                    \
      return 0;                                                                              \
    }                                                                                        \
    (track).type = static_cast<MDLTRACKTYPE>((buffer).GetUint());                            \
    (track).globalSeqId = (buffer).GetUint();                                                \
    (totalRead) += 8;                                                                        \
    (track).keys.SetCount(binFloatCount);                                                    \
    UINT binFloatValues = (track).type > TRACK_LINEAR ? (elements) * 3 : (elements);         \
    UINT binFloatBytesPerKey = 4 + binFloatValues * 4;                                       \
    if (binFloatCount * binFloatBytesPerKey > static_cast<UINT>((buffer).Bytes())) {         \
      return 0;                                                                              \
    }                                                                                        \
    for (UINT binFloatIndex = 0; binFloatIndex < binFloatCount; ++binFloatIndex) {           \
      MDLKEYFRAME<keyType> &binFloatKey = (track).keys.Ptr()[binFloatIndex];                 \
      binFloatKey.time = (buffer).GetInt();                                                  \
      (buffer).GetFloatArray(reinterpret_cast<float *>(&binFloatKey.value), binFloatValues); \
      (totalRead) += binFloatBytesPerKey;                                                    \
    }                                                                                        \
    return 1;                                                                                \
  } while (0)

void WriteBinFloatKeyFrames(const MDLKEYTRACK<float> &track, DWORD magic, CMsgBuffer &buffer) {
  if (!track.keys.Count()) {
    return;
  }
  buffer.AddDword(magic);
  buffer.AddUint(track.keys.Count());
  buffer.AddUint(track.type);
  buffer.AddUint(track.globalSeqId);
  UINT values = track.type > TRACK_LINEAR ? 3 : 1;
  for (UINT i = 0; i < track.keys.Count(); ++i) {
    const MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
    buffer.AddInt(key.time);
    buffer.AddFloatArray(&key.value, values);
  }
}

int ReadBinFloatKeyFrames(MDLKEYTRACK<float> &track, CMsgBuffer &buffer, UINT &totalRead) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 1, float);
}

int ReadBinFloatKeyFrames(MDLKEYTRACK<C3Color> &track, CMsgBuffer &buffer, UINT &totalRead) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 3, C3Color);
}

int ReadBinFloatKeyFrames(MDLKEYTRACK<NTempest::C3Vector> &track, CMsgBuffer &buffer, UINT &totalRead) {
  READ_BIN_FLOAT_KEYFRAMES(track, buffer, totalRead, 3, NTempest::C3Vector);
}

#undef READ_BIN_FLOAT_KEYFRAMES

int ReadBinUintKeyFrames(MDLSIMPLEKEYTRACK<MDLINTKEY> &track, CMsgBuffer &buffer, UINT &totalRead) {
  if (buffer.Bytes() < 8) {
    return 0;
  }
  UINT count = buffer.GetUint();
  totalRead += 4;
  if (!count) {
    return 0;
  }
  buffer.GetUint();
  track.globalSeqId = buffer.GetUint();
  totalRead += 8;
  if (count * 8 > static_cast<UINT>(buffer.Bytes())) {
    return 0;
  }
  track.keys.SetCount(count);
  for (UINT i = 0; i < count; ++i) {
    MDLINTKEY &key = track.keys.Ptr()[i];
    key.time = buffer.GetInt();
    key.value = buffer.GetUint();
    totalRead += 8;
  }
  return 1;
}

void WriteBinUintKeyFrames(const MDLSIMPLEKEYTRACK<MDLINTKEY> &track, DWORD magic, CMsgBuffer &buffer) {
  if (!track.keys.Count()) {
    return;
  }
  buffer.AddDword(magic);
  buffer.AddUint(track.keys.Count());
  buffer.AddUint(0);
  buffer.AddUint(track.globalSeqId);
  for (UINT i = 0; i < track.keys.Count(); ++i) {
    const MDLINTKEY &key = track.keys.Ptr()[i];
    buffer.AddInt(key.time);
    buffer.AddUintArray(&key.value, 1);
  }
}

UINT GetBinGenObjectSize(const MDLGENOBJECT &obj) {
  UINT size = 96;
  if (obj.transkeys.keys.Count()) {
    UINT dataSize = obj.transkeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + obj.transkeys.keys.Count() * (4 + dataSize);
  }
  size += GetBinQuatKeyFramesSize(obj.rotkeys);
  if (obj.scalekeys.keys.Count()) {
    UINT dataSize = obj.scalekeys.type > TRACK_LINEAR ? 36 : 12;
    size += 16 + obj.scalekeys.keys.Count() * (4 + dataSize);
  }
  return size;
}

void WriteBinQuatKeyFrames(const MDLKEYTRACK<NTempest::C4Quaternion> &keyframes, DWORD magicParam, CMsgBuffer &buffer) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  buffer.AddDword(magicParam);
  buffer.AddUint(numKeys);
  buffer.AddUint(keyframes.type);
  buffer.AddUint(keyframes.globalSeqId);

  for (UINT key = 0; key < numKeys; ++key) {
    const MDLKEYFRAME<NTempest::C4Quaternion> &frame = keyframes.keys[key];
    buffer.AddInt(frame.time);

    const NTempest::C4Quaternion *quaternion = &frame.value;
    UINT                          values = keyframes.type > TRACK_LINEAR ? 3 : 1;
    for (UINT j = 0; j < values; ++j, ++quaternion) {
      int      sign = quaternion->w >= 0.0f ? 1 : -1;
      LONGLONG x = static_cast<LONGLONG>(quaternion->x * 2097152.0f);
      LONGLONG y = static_cast<LONGLONG>(quaternion->y * 1048576.0f);
      LONGLONG z = static_cast<LONGLONG>(quaternion->z * 1048576.0f);
      x *= sign;
      y *= sign;
      z *= sign;
      DWORDLONG packed = (static_cast<DWORDLONG>(x) << 42) | ((static_cast<DWORDLONG>(y) & 0x1FFFFF) << 21) | (static_cast<DWORDLONG>(z) & 0x1FFFFF);
      buffer.AddLongLong(packed);
    }
  }
}

int ReadBinQuatKeyFrames(MDLKEYTRACK<NTempest::C4Quaternion> &keyframes, CMsgBuffer &buf, UINT &totalRead) {
  if (buf.Bytes() < 12) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  totalRead += 4;
  if (!numKeys) {
    return 0;
  }
  keyframes.type = static_cast<MDLTRACKTYPE>(buf.GetUint());
  keyframes.globalSeqId = buf.GetUint();
  totalRead += 8;
  keyframes.keys.SetCount(numKeys);

  UINT key = keyframes.type > TRACK_LINEAR ? 3 : 1;
  UINT bytesPerKey = 4 + key * 8;
  if (numKeys * bytesPerKey > static_cast<UINT>(buf.Bytes())) {
    return 0;
  }

  for (UINT i = 0; i < numKeys; ++i) {
    MDLKEYFRAME<NTempest::C4Quaternion> &frame = keyframes.keys[i];
    frame.time = buf.GetInt();
    totalRead += 4;
    NTempest::C4Quaternion *quaternion = &frame.value;
    for (UINT j = 0; j < key; ++j, ++quaternion) {
      DWORDLONG packed = buf.GetUlongLong();
      int       z = static_cast<int>(static_cast<LONGLONG>(packed << 43) >> 43);
      int       y = static_cast<int>(static_cast<LONGLONG>(((packed >> 21) & 0x1FFFFF) << 43) >> 43);
      int       x = static_cast<int>(static_cast<LONGLONG>(packed) >> 42);
      quaternion->x = x * 0.00000047683716f;
      quaternion->y = y * 0.00000095367432f;
      quaternion->z = z * 0.00000095367432f;
      float square = quaternion->x * quaternion->x + quaternion->y * quaternion->y + quaternion->z * quaternion->z;
      quaternion->w = fabs(square - 1.0f) >= 0.00000095367432f ? static_cast<float>(sqrt(1.0f - square)) : 0.0f;
      totalRead += 8;
    }
  }
  return 1;
}

int WriteBinGenObject(const MDLGENOBJECT &obj, CMsgBuffer &buf, CMDLStatus *) {
  buf.AddUint(GetBinGenObjectSize(obj));
  buf.AddTcharArray(obj.name, 80, 1);
  buf.AddUint(obj.objectId);
  buf.AddUint(obj.parentId);
  buf.AddUint(obj.flags);

  if (obj.transkeys.keys.Count()) {
    buf.AddDword('RTGK');
    buf.AddUint(obj.transkeys.keys.Count());
    buf.AddUint(obj.transkeys.type);
    buf.AddUint(obj.transkeys.globalSeqId);
    UINT values = obj.transkeys.type > TRACK_LINEAR ? 9 : 3;
    for (UINT i = 0; i < obj.transkeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = obj.transkeys.keys[i];
      buf.AddInt(key.time);
      buf.AddFloatArray(&key.value.x, values);
    }
  }
  WriteBinQuatKeyFrames(obj.rotkeys, 'RTRK', buf);
  if (obj.scalekeys.keys.Count()) {
    buf.AddDword('CSGK');
    buf.AddUint(obj.scalekeys.keys.Count());
    buf.AddUint(obj.scalekeys.type);
    buf.AddUint(obj.scalekeys.globalSeqId);
    UINT values = obj.scalekeys.type > TRACK_LINEAR ? 9 : 3;
    for (UINT i = 0; i < obj.scalekeys.keys.Count(); ++i) {
      const MDLKEYFRAME<NTempest::C3Vector> &key = obj.scalekeys.keys[i];
      buf.AddInt(key.time);
      buf.AddFloatArray(&key.value.x, values);
    }
  }
  return 1;
}

int ReadBinGenObject(MDLGENOBJECT &object, CMsgBuffer &buffer, CMDLStatus *status, UINT &totalRead) {
  if (!status) {
    return 0;
  }
  UINT totalSize = buffer.GetUint();
  buffer.GetTcharArray(object.name, 80);
  object.objectId = buffer.GetUint();
  object.parentId = buffer.GetUint();
  object.flags = buffer.GetUint();
  UINT localRead = 96;

  while (localRead < totalSize) {
    DWORD tag = buffer.GetDword();
    localRead += 4;
    if (tag == 'RTGK' || tag == 'CSGK') {
      MDLKEYTRACK<NTempest::C3Vector> &track = tag == 'RTGK' ? object.transkeys : object.scalekeys;
      if (buffer.Bytes() < 12) {
        status->Add(STATUS_ERROR, tag == 'RTGK' ? "Could not read Translation keyframes.\n" : "Could not read Scaling keyframes.\n");
        return 0;
      }
      UINT count = buffer.GetUint();
      localRead += 4;
      if (!count) {
        status->Add(STATUS_ERROR, tag == 'RTGK' ? "Could not read Translation keyframes.\n" : "Could not read Scaling keyframes.\n");
        return 0;
      }
      track.type = static_cast<MDLTRACKTYPE>(buffer.GetUint());
      track.globalSeqId = buffer.GetUint();
      localRead += 8;
      track.keys.SetCount(count);
      UINT values = track.type > TRACK_LINEAR ? 9 : 3;
      UINT bytesPerKey = 4 + 4 * values;
      if (count * bytesPerKey > static_cast<UINT>(buffer.Bytes())) {
        status->Add(STATUS_ERROR, tag == 'RTGK' ? "Could not read Translation keyframes.\n" : "Could not read Scaling keyframes.\n");
        return 0;
      }
      for (UINT i = 0; i < count; ++i) {
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

void ReadBinObjectEnd(MDLDATA &data, MDLGENOBJECT *obj, DWORD listIndex, DWORD listMask) {
  FATALASSERT(obj);

  obj->objectId = data.objects.Count();
  if (obj->objectId >= data.objects.Count()) {
    UINT oldCount = data.objects.Count();
    data.objects.SetCount(obj->objectId + 1);
    for (UINT i = oldCount; i < data.objects.Count(); ++i) {
      data.objects[i] = 0;
    }
  }
  data.objects[obj->objectId] = reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

int ReadObjectPtrs(MDLDATA *data, CMDLStatus *status) {
  for (UINT i = 0; i < data->objects.Count(); ++i) {
    UINT encoded = reinterpret_cast<UINT>(data->objects[i]);
    UINT index = encoded & 0x0FFFFFFF;

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
  UINT i;
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
