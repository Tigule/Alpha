#pragma once

#include "MDLTypes.h"
#include "Parser.h"
#include "Base/MsgBuffer.h"

class CMDLStatus;
class Parser;
class TSet;
union UTokenData;

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void         ReadFloatKeyData(Parser &parse, float *entry, UINT elements);
BOOL         ReadObjectPtrs(MDLDATA *data, CMDLStatus *status);
const float *WriteKeyData(TSGrowableArray<char> &buffer, const float *entry, UINT elements);
const UINT  *WriteUintKeyData(TSGrowableArray<char> &buffer, const UINT *entry, UINT elements);
void         AddObjectErrors(TSet &errors);
void         ReadObjectName(Parser &parse, char *name);
BOOL         ReadObjectBody(Parser &parse, UINT savedToken, NTempest::C3Vector *pivot, MDLGENOBJECT *object, CMDLStatus *status);
void         ReadObjectEnd(TSet &errors, MDLDATA &data, MDLGENOBJECT *object, DWORD listIndex, DWORD listMask);
void         ReadBinObjectEnd(MDLDATA &data, MDLGENOBJECT *object, DWORD listIndex, DWORD listMask);
BOOL         IExpectAnimation(Parser &parse, UINT *savedToken, LPCSTR *tokenText);
void         WriteObjectTrailer(const MDLGENOBJECT &object, TSGrowableArray<char> &buffer);
void         WriteObjectHeader(const MDLDATA &data, const MDLGENOBJECT &object, UINT title, int writeIndex, TSGrowableArray<char> &buffer);
void         WriteOptionalVertex(UINT title, LPCSTR indent, const NTempest::C3Vector &vertex, TSGrowableArray<char> &buffer);
void         WriteOptionalFloat(UINT title, LPCSTR indent, float value, TSGrowableArray<char> &buffer);
void         WriteBounds(const CMdlBounds &bounds, LPCSTR indent, TSGrowableArray<char> &buffer);
void         SkipUnknown(CMsgBuffer &buffer, UINT &totalRead);
UINT         GetBinGenObjectSize(const MDLGENOBJECT &object);
BOOL         WriteBinGenObject(const MDLGENOBJECT &object, CMsgBuffer &buffer, CMDLStatus *status);
BOOL         ReadBinGenObject(MDLGENOBJECT &object, CMsgBuffer &buffer, CMDLStatus *status, UINT &totalRead);
void         WriteBinQuatKeyFrames(const MDLKEYTRACK<NTempest::C4Quaternion> &track, DWORD magic, CMsgBuffer &buffer);
BOOL         ReadBinQuatKeyFrames(MDLKEYTRACK<NTempest::C4Quaternion> &track, CMsgBuffer &buffer, UINT &totalRead);
UINT         GetBinQuatKeyFramesSize(const MDLKEYTRACK<NTempest::C4Quaternion> &track);
int          ReadBinFloatKeyFrames(MDLKEYTRACK<NTempest::C3Vector> &keyframes, CMsgBuffer &buf, UINT &totalRead);
UINT         ReadIntTrackHeader(Parser &parse, MDLSIMPLEKEYTRACK<MDLINTKEY> *keyTrack, LPCSTR *tokentext, UTokenData *value);
void         WriteIntKeyFrames(UINT title, LPCSTR indent, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, TSGrowableArray<char> &buffer);

template <class T>
inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<T> *keyTrack, LPCSTR *tokentext, UTokenData *value) {
  for (;;) {
    UINT token = parse.Token(tokentext, value);
    switch (token) {
      case MDLTOK_DONTINTERP:
        if (keyTrack) {
          keyTrack->type = TRACK_NO_INTERP;
        }
        break;
      case MDLTOK_LINEAR:
        if (keyTrack) {
          keyTrack->type = TRACK_LINEAR;
        }
        break;
      case MDLTOK_HERMITE:
        if (keyTrack) {
          keyTrack->type = TRACK_HERMITE;
        }
        break;
      case MDLTOK_BEZIER:
        if (keyTrack) {
          keyTrack->type = TRACK_BEZIER;
        }
        break;
      case MDLTOK_GLOBALSEQID:
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

template <class T>
inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<T> *keyframes) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData value;
  long       actual = 0;
  long       count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0 && keyframes) {
    keyframes->keys.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = ReadFloatTrackHeader(parse, keyframes, &tokentext, &value);
  int noTangents = keyframes->type <= TRACK_LINEAR;
  while (savedtoken == MDLTOK_LONG) {
    MDLKEYFRAME<T> *key = keyframes->keys.New();
    key->time = value.lVal;
    parse.Expect(':');
    ReadFloatKeyData(parse, (float *)&key->value, sizeof(T) / sizeof(float));
    parse.Expect(',');
    ++actual;
    if (!noTangents) {
      parse.Expect(MDLTOK_INTAN);
      ReadFloatKeyData(parse, (float *)&key->inTan, sizeof(T) / sizeof(float));
      parse.Expect(',');
      parse.Expect(MDLTOK_OUTTAN);
      ReadFloatKeyData(parse, (float *)&key->outTan, sizeof(T) / sizeof(float));
      parse.Expect(',');
    }
    savedtoken = parse.Token(&tokentext, &value);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("key frames", count, actual);
  }
}

template <class T>
inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<T> &keyframes, TSGrowableArray<char> &buffer) {
  switch (keyframes.type) {
    case TRACK_NO_INTERP:
      MDL::WriteLine(buffer, "%s\t%s,\n", indent, MDL::TokenText(MDLTOK_DONTINTERP));
      break;
    case TRACK_LINEAR:
      MDL::WriteLine(buffer, "%s\t%s,\n", indent, MDL::TokenText(MDLTOK_LINEAR));
      break;
    case TRACK_HERMITE:
      MDL::WriteLine(buffer, "%s\t%s,\n", indent, MDL::TokenText(MDLTOK_HERMITE));
      break;
    case TRACK_BEZIER:
      MDL::WriteLine(buffer, "%s\t%s,\n", indent, MDL::TokenText(MDLTOK_BEZIER));
      break;
  }
  if (keyframes.globalSeqId != (UINT)-1) {
    MDL::WriteLine(buffer, "%s\t%s %d,\n", indent, MDL::TokenText(MDLTOK_GLOBALSEQID), keyframes.globalSeqId);
  }
}

template <class T>
inline void WriteFloatKeyFrames(UINT title, LPCSTR indent, const MDLKEYTRACK<T> &keyframes, TSGrowableArray<char> &buffer) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  MDL::WriteLine(buffer, "%s%s %d {\n", indent, MDL::TokenText(title), numKeys);
  WriteTrackHeader(indent, keyframes, buffer);
  const MDLKEYFRAME<T> *key = keyframes.keys.Ptr();
  int                   noTangents = keyframes.type <= TRACK_LINEAR;
  for (UINT i = numKeys; i; --i, ++key) {
    MDL::WriteLine(buffer, "%s\t%d: ", indent, key->time);
    const float *data = WriteKeyData(buffer, (const float *)&key->value, sizeof(T) / sizeof(float));
    if (!noTangents) {
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_INTAN));
      data = WriteKeyData(buffer, data, sizeof(T) / sizeof(float));
      MDL::WriteLine(buffer, "%s\t\t%s ", indent, MDL::TokenText(MDLTOK_OUTTAN));
      data = WriteKeyData(buffer, data, sizeof(T) / sizeof(float));
    }
  }
  MDL::WriteLine(buffer, "%s}\n", indent);
}

template <class T>
inline void WriteBinFloatKeyFrames(const MDLKEYTRACK<T> &keyframes, DWORD magicParam, CMsgBuffer &buf) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  buf.AddDword(magicParam);
  buf.AddUint(numKeys);
  buf.AddUint(keyframes.type);
  buf.AddUint(keyframes.globalSeqId);
  UINT elements = sizeof(T) / sizeof(float);
  if (keyframes.type > TRACK_LINEAR) {
    elements = 3 * sizeof(T) / sizeof(float);
  }
  const MDLKEYFRAME<T> *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    buf.AddInt(key->time);
    buf.AddFloatArray((const float *)&key->value, elements);
  }
}

template <class T>
inline int ReadBinFloatKeyFrames(MDLKEYTRACK<T> &keyframes, CMsgBuffer &buf, UINT &totalRead) {
  if (buf.Bytes() < 3 * sizeof(UINT)) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  totalRead += sizeof(UINT);
  if (!numKeys) {
    return 0;
  }
  keyframes.type = (MDLTRACKTYPE)buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.globalSeqId = buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.keys.SetCount(numKeys);
  MDLKEYFRAME<T> *key = keyframes.keys.Ptr();
  int             keySize = sizeof(int) + sizeof(T);
  UINT            elements = sizeof(T) / sizeof(float);
  if (keyframes.type > TRACK_LINEAR) {
    keySize = sizeof(int) + 3 * sizeof(T);
    elements = 3 * sizeof(T) / sizeof(float);
  }
  if ((int)(keySize * numKeys) > buf.Bytes()) {
    return 0;
  }
  for (UINT i = numKeys; i; --i, ++key) {
    key->time = buf.GetInt();
    totalRead += sizeof(int);
    buf.GetFloatArray((float *)&key->value, elements);
    totalRead += elements * sizeof(float);
  }
  return 1;
}

inline int ReadBinUintKeyFrames(MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, CMsgBuffer &buf, UINT &totalRead) {
  if (buf.Bytes() < 2 * sizeof(UINT)) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  totalRead += sizeof(UINT);
  if (!numKeys) {
    return 0;
  }
  buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.globalSeqId = buf.GetUint();
  totalRead += sizeof(UINT);
  keyframes.keys.SetCount(numKeys);
  MDLINTKEY *key = keyframes.keys.Ptr();
  if ((int)(numKeys * sizeof(MDLINTKEY)) > buf.Bytes()) {
    return 0;
  }
  for (UINT i = numKeys; i; --i, ++key) {
    key->time = buf.GetInt();
    totalRead += sizeof(int);
    buf.GetUintArray(&key->value, 1);
    totalRead += sizeof(UINT);
  }
  return 1;
}

inline void WriteBinUintKeyFrames(const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, DWORD magicParam, CMsgBuffer &buf) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  buf.AddDword(magicParam);
  buf.AddUint(numKeys);
  buf.AddUint(0);
  buf.AddUint(keyframes.globalSeqId);
  const MDLINTKEY *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    buf.AddInt(key->time);
    buf.AddUintArray(&key->value, 1);
  }
}
