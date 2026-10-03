#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"
#include "Tempest/c4quaternioncompressed.h"

#include <math.h>
#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void ReadFloatKeyData(Parser &parse, float *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements <= 1) {
    *entry = parse.ExpectFloat();
  } else {
    parse.Expect('{');
    *entry++ = parse.ExpectFloat();
    for (UINT i = elements - 1; i; --i) {
      parse.Expect(',');
      *entry++ = parse.ExpectFloat();
    }
    parse.Expect('}');
  }
}

BOOL ReadObjectPtrs(MDLDATA *data, CMDLStatus *status) {
  UINT numElements = data->objects.Count();
  for (UINT i = 0; i < numElements; ++i) {
    UINT index = reinterpret_cast<UINT>(data->objects[i]) & 0x0FFFFFFF;
    switch (reinterpret_cast<UINT>(data->objects[i]) & 0xF0000000) {
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

const float *WriteKeyData(TSGrowableArray<char> &buffer, const float *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%g,\n", *entry++);
    return entry;
  }
  MDL::WriteLine(buffer, "{ %g", *entry++);
  for (UINT i = elements - 1; i; --i) {
    MDL::WriteLine(buffer, ", %g", *entry);
    ++entry;
  }
  MDL::WriteLine(buffer, " },\n");
  return entry;
}

const UINT *WriteUintKeyData(TSGrowableArray<char> &buffer, const UINT *entry, UINT elements) {
  FATALASSERT(elements > 0);
  if (elements == 1) {
    MDL::WriteLine(buffer, "%u,\n", *entry++);
    return entry;
  }
  MDL::WriteLine(buffer, "{ %u", *entry++);
  for (UINT i = elements - 1; i; --i) {
    MDL::WriteLine(buffer, ", %u", *entry);
    ++entry;
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

static void INormalizeQuats(TSGrowableArray<MDLKEYFRAME<NTempest::C4Quaternion> > *keyframes) {
  MDLKEYFRAME<NTempest::C4Quaternion> *key = keyframes->Ptr();
  for (UINT i = keyframes->Count(); i; --i, ++key) {
    key->value.Normalize();
    key->inTan.Normalize();
    key->outTan.Normalize();
  }
}

BOOL ReadObjectBody(Parser &parse, UINT savedtoken, NTempest::C3Vector *pivot, MDLGENOBJECT *obj, CMDLStatus *status) {
  switch (savedtoken) {
    case 0x187:
      obj->objectId = parse.ExpectInt();
      break;
    case 0x18B:
      obj->parentId = parse.ExpectInt();
      break;
    case 0x13F:
      IReadDontInherit(parse, obj, status);
      break;
    case 0x129:
      obj->flags |= 8;
      break;
    case 0x12A:
      obj->flags |= 0x10;
      break;
    case 0x12B:
      obj->flags |= 0x20;
      break;
    case 0x12C:
      obj->flags |= 0x40;
      break;
    case 0x1A7:
      obj->flags |= 0x4000;
      break;
    case 0x1C7:
      ReadObjectFloatKeyframes(parse, &obj->transkeys);
      return 1;
    case 0x1AD:
      ReadObjectFloatKeyframes(parse, &obj->rotkeys);
      INormalizeQuats(&obj->rotkeys.keys);
      return 1;
    case 0x1AF:
      ReadObjectFloatKeyframes(parse, &obj->scalekeys);
      return 1;
    case 0x1A5:
      if (!pivot) {
        return 0;
      }
      ReadFloatKeyData(parse, &pivot->x, 3);
      break;
    default:
      return 0;
  }
  parse.Expect(',');
  return 1;
}

void ReadObjectEnd(TSet &errors, MDLDATA &data, MDLGENOBJECT *obj, DWORD listIndex, DWORD listMask) {
  FATALASSERT(obj);
  if (errors.NotFound(0x187)) {
    obj->objectId = data.objects.Count();
  }
  data.objects.GrowToFit(obj->objectId, 1);
  data.objects[obj->objectId] = reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

void ReadBinObjectEnd(MDLDATA &data, MDLGENOBJECT *obj, DWORD listIndex, DWORD listMask) {
  FATALASSERT(obj);
  obj->objectId = data.objects.Count();
  data.objects.GrowToFit(obj->objectId, 1);
  data.objects[obj->objectId] = reinterpret_cast<MDLGENOBJECT *>(listMask | listIndex);
}

BOOL IExpectAnimation(Parser &parse, UINT *savedtoken, LPCSTR *tokenText) {
  BOOL result = 1;
  if (*savedtoken == 0x1BB) {
    result = 0;
    *savedtoken = parse.Token(tokenText, 0);
  }
  return result;
}

void WriteObjectTrailer(const MDLGENOBJECT &obj, TSGrowableArray<char> &buffer) {
  WriteFloatKeyFrames(0x1C7, "\t", obj.transkeys, buffer);
  WriteFloatKeyFrames(0x1AD, "\t", obj.rotkeys, buffer);
  WriteFloatKeyFrames(0x1AF, "\t", obj.scalekeys, buffer);
  MDL::WriteLine(buffer, "}\n");
}

static struct {
  UINT mask;
  UINT token;
} s_objectFlags[] = {
    {8,      0x129},
    {0x10,   0x12A},
    {0x20,   0x12B},
    {0x40,   0x12C},
    {0x4000, 0x1A7},
};

static void IWriteObjectFlags(UINT flags, TSGrowableArray<char> &buffer) {
  for (UINT i = 0; i < sizeof(s_objectFlags) / sizeof(s_objectFlags[0]); ++i) {
    if (s_objectFlags[i].mask & flags) {
      MDL::WriteLine(buffer, "\t%s,\n", MDL::TokenText(s_objectFlags[i].token));
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
    if (obj.flags & 1) {
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1C7));
    }
    if (obj.flags & 2) {
      if (obj.flags & 1) {
        MDL::WriteLine(buffer, ", ");
      }
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AF));
    }
    if (obj.flags & 4) {
      if (obj.flags & 3) {
        MDL::WriteLine(buffer, ", ");
      }
      MDL::WriteLine(buffer, "%s", MDL::TokenText(0x1AD));
    }
    MDL::WriteLine(buffer, " },\n");
  }
}

void WriteOptionalVertex(UINT title, LPCSTR indent, const NTempest::C3Vector &vertex, TSGrowableArray<char> &buffer) {
  if (NTempest::CMath::fnotequal_(vertex.x, 0.0f) || NTempest::CMath::fnotequal_(vertex.y, 0.0f) || NTempest::CMath::fnotequal_(vertex.z, 0.0f)) {
    MDL::WriteLine(buffer, "%s%s { %g, %g, %g },\n", indent, MDL::TokenText(title), vertex.x, vertex.y, vertex.z);
  }
}

void WriteOptionalFloat(UINT title, LPCSTR indent, float value, TSGrowableArray<char> &buffer) {
  if (NTempest::CMath::fnotequal_(value, 0.0f)) {
    MDL::WriteLine(buffer, "%s%s %g,\n", indent, MDL::TokenText(title), value);
  }
}

void WriteBounds(const CMdlBounds &bounds, LPCSTR indent, TSGrowableArray<char> &buffer) {
  WriteOptionalVertex(0x170, indent, bounds.extent.b, buffer);
  WriteOptionalVertex(0x16F, indent, bounds.extent.t, buffer);
  WriteOptionalFloat(0x134, indent, bounds.radius, buffer);
}

void SkipUnknown(CMsgBuffer &buf, UINT &totalRead) {
  UINT size = buf.GetUint() - 4;
  totalRead += 4;
  if (buf.Bytes() < size) {
    size = buf.Bytes();
  }
  buf.GetData(size);
  totalRead += size;
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

BOOL WriteBinGenObject(const MDLGENOBJECT &obj, CMsgBuffer &buf, CMDLStatus *status) {
  buf.AddUint(GetBinGenObjectSize(obj));
  buf.AddTcharArray(obj.name, 80, 1);
  buf.AddUint(obj.objectId);
  buf.AddUint(obj.parentId);
  buf.AddUint(obj.flags);
  WriteBinFloatKeyFrames(obj.transkeys, 'RTGK', buf);
  WriteBinQuatKeyFrames(obj.rotkeys, 'TRGK', buf);
  WriteBinFloatKeyFrames(obj.scalekeys, 'CSGK', buf);
  return 1;
}

BOOL ReadBinGenObject(MDLGENOBJECT &obj, CMsgBuffer &buf, CMDLStatus *status, UINT &totalRead) {
  if (!status) {
    return 0;
  }
  UINT totalSize = buf.GetUint();
  UINT localBytesRead = 4;
  buf.GetTcharArray(obj.name, 80);
  localBytesRead += 80;
  obj.objectId = buf.GetUint();
  obj.parentId = buf.GetUint();
  obj.flags = buf.GetUint();
  localBytesRead += 12;
  while (localBytesRead < totalSize) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;
    switch (tag) {
      case 'RTGK':
        if (!ReadBinFloatKeyFrames(obj.transkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Could not read trans keys in gen object.\n");
          return 0;
        }
        break;
      case 'TRGK':
        if (!ReadBinQuatKeyFrames(obj.rotkeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Could not read rot keys in gen object.\n");
          return 0;
        }
        break;
      case 'CSGK':
        if (!ReadBinFloatKeyFrames(obj.scalekeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Could not read scale keys in gen object.\n");
          return 0;
        }
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }
    if (localBytesRead > totalSize) {
      status->FatalOverran("Genobject keys", -1);
      return 0;
    }
  }
  totalRead += localBytesRead;
  return 1;
}

void MDLBASE::RebuildObjectPtrs() {
  memset(objects.Ptr(), 0, objects.Count() * sizeof(MDLGENOBJECT *));
  UINT numElements;
  UINT i;
  numElements = bones.Count();
  for (i = 0; i < numElements; ++i) {
    objects[bones[i].objectId] = &bones[i];
  }
  numElements = lights.Count();
  for (i = 0; i < numElements; ++i) {
    objects[lights[i].objectId] = &lights[i];
  }
  numElements = helpers.Count();
  for (i = 0; i < numElements; ++i) {
    objects[helpers[i].objectId] = &helpers[i];
  }
  numElements = attachments.Count();
  for (i = 0; i < numElements; ++i) {
    objects[attachments[i].objectId] = &attachments[i];
  }
  numElements = particleEmitters.Count();
  for (i = 0; i < numElements; ++i) {
    objects[particleEmitters[i].objectId] = &particleEmitters[i];
  }
  numElements = events.Count();
  for (i = 0; i < numElements; ++i) {
    objects[events[i].objectId] = &events[i];
  }
  numElements = particleEmitters2.Count();
  for (i = 0; i < numElements; ++i) {
    objects[particleEmitters2[i].objectId] = &particleEmitters2[i];
  }
  numElements = hitTestShapes.Count();
  for (i = 0; i < numElements; ++i) {
    objects[hitTestShapes[i].objectId] = &hitTestShapes[i];
  }
  numElements = ribbonEmitters.Count();
  for (i = 0; i < numElements; ++i) {
    objects[ribbonEmitters[i].objectId] = &ribbonEmitters[i];
  }
}

void WriteBinQuatKeyFrames(const MDLKEYTRACK<NTempest::C4Quaternion> &keyframes, DWORD magicParam, CMsgBuffer &buf) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  buf.AddDword(magicParam);
  buf.AddUint(numKeys);
  buf.AddUint(keyframes.type);
  buf.AddUint(keyframes.globalSeqId);

  const MDLKEYFRAME<NTempest::C4Quaternion> *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    buf.AddInt(key->time);
    buf.AddLongLong(NTempest::C4QuaternionCompressed(key->value).Raw());
    if (keyframes.type > TRACK_LINEAR) {
      buf.AddLongLong(NTempest::C4QuaternionCompressed(key->inTan).Raw());
      buf.AddLongLong(NTempest::C4QuaternionCompressed(key->outTan).Raw());
    }
  }
}

BOOL ReadBinQuatKeyFrames(MDLKEYTRACK<NTempest::C4Quaternion> &keyframes, CMsgBuffer &buf, UINT &totalRead) {
  if (buf.Bytes() < 3 * sizeof(UINT)) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  totalRead += sizeof(UINT);
  if (!numKeys) {
    return 0;
  }
  keyframes.type = static_cast<MDLTRACKTYPE>(buf.GetUint());
  totalRead += sizeof(UINT);
  keyframes.globalSeqId = buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.keys.SetCount(numKeys);

  MDLKEYFRAME<NTempest::C4Quaternion> *key = keyframes.keys.Ptr();
  int                                  keySize = sizeof(int) + sizeof(LONGLONG);
  if (keyframes.type > TRACK_LINEAR) {
    keySize = sizeof(int) + 3 * sizeof(LONGLONG);
  }
  if (static_cast<int>(keySize * numKeys) > buf.Bytes()) {
    return 0;
  }

  for (UINT i = numKeys; i; --i, ++key) {
    key->time = buf.GetInt();
    totalRead += sizeof(int);
    key->value = NTempest::C4QuaternionCompressed(buf.GetLongLong());
    totalRead += sizeof(LONGLONG);
    if (keyframes.type > TRACK_LINEAR) {
      key->inTan = NTempest::C4QuaternionCompressed(buf.GetLongLong());
      key->outTan = NTempest::C4QuaternionCompressed(buf.GetLongLong());
      totalRead += 2 * sizeof(LONGLONG);
    }
  }
  return 1;
}

UINT GetBinQuatKeyFramesSize(const MDLKEYTRACK<NTempest::C4Quaternion> &keyframes) {
  if (!keyframes.keys.Count()) {
    return 0;
  }
  UINT dataSize = 8;
  if (keyframes.type > TRACK_LINEAR) {
    dataSize = 24;
  }
  return (dataSize + 4) * keyframes.keys.Count() + 16;
}

UINT ReadIntTrackHeader(Parser &parse, MDLSIMPLEKEYTRACK<MDLINTKEY> *keyTrack, LPCSTR *tokentext, UTokenData *value) {
  for (;;) {
    UINT token = parse.Token(tokentext, value);
    switch (token) {
      case 0x140:
        break;
      case 0x152:
        if (keyTrack) {
          keyTrack->globalSeqId = parse.ExpectInt();
        } else {
          parse.ExpectInt();
        }
        break;
      default:
        return token;
    }
    parse.Expect(',');
  }
}

void WriteIntKeyFrames(UINT title, LPCSTR indent, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, TSGrowableArray<char> &buffer) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  MDL::WriteLine(buffer, "%s%s %u {\n", indent, MDL::TokenText(title), numKeys);
  if (keyframes.globalSeqId != static_cast<UINT>(-1)) {
    MDL::WriteLine(buffer, "%s\t%s %d,\n", indent, MDL::TokenText(0x152), keyframes.globalSeqId);
  }
  const MDLINTKEY *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    MDL::WriteLine(buffer, "%s\t%d: %u,\n", indent, key->time, key->value);
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

int ReadBinFloatKeyFrames(MDLKEYTRACK<NTempest::C3Vector> &keyframes, CMsgBuffer &buf, UINT &totalRead) {
  if (buf.Bytes() < 3 * sizeof(UINT)) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  totalRead += sizeof(UINT);
  if (!numKeys) {
    return 0;
  }
  keyframes.type = static_cast<MDLTRACKTYPE>(buf.GetUint());
  totalRead += sizeof(UINT);
  keyframes.globalSeqId = buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.keys.SetCount(numKeys);
  MDLKEYFRAME<NTempest::C3Vector> *key = keyframes.keys.Ptr();
  int                              keySize = sizeof(int) + sizeof(NTempest::C3Vector);
  UINT                             elements = sizeof(NTempest::C3Vector) / sizeof(float);
  if (keyframes.type > TRACK_LINEAR) {
    keySize = sizeof(int) + 3 * sizeof(NTempest::C3Vector);
    elements = 3 * sizeof(NTempest::C3Vector) / sizeof(float);
  }
  if (static_cast<int>(keySize * numKeys) > buf.Bytes()) {
    return 0;
  }
  for (UINT i = numKeys; i; --i, ++key) {
    key->time = buf.GetInt();
    totalRead += sizeof(int);
    buf.GetFloatArray(reinterpret_cast<float *>(&key->value), elements);
    totalRead += elements * sizeof(float);
  }
  return 1;
}
