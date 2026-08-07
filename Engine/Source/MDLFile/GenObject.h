#pragma once

#include "MDLTypes.h"
#include "Parser.h"

class CMsgBuffer;
class CMDLStatus;
class Parser;
class TSet;
union UTokenData;

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void        ReadFloatKeyData(Parser &parse, float *entry, UINT elements);
inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<NTempest::C3Vector> *track, LPCSTR *tokenText, UTokenData *tokenData);
UINT        ReadIntTrackHeader(Parser &parse, MDLSIMPLEKEYTRACK<MDLINTKEY> *track, LPCSTR *tokenText, UTokenData *tokenData);
inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<NTempest::C4Quaternion> *track, LPCSTR *tokenText, UTokenData *tokenData);
inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<float> *track, LPCSTR *tokenText, UTokenData *tokenData);
inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<NTempest::C3Vector> *track);
inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<float> *track);
inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<C3Color> *track);
inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<NTempest::C3Vector> &track, TSGrowableArray<char> &buffer);
inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<NTempest::C4Quaternion> &track, TSGrowableArray<char> &buffer);
inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<float> &track, TSGrowableArray<char> &buffer);
inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<C3Color> &track, TSGrowableArray<char> &buffer);

#define READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, result) \
  do {                                                                             \
    (result) = (parse).Token((tokenText), (tokenData));                            \
    for (;;) {                                                                     \
      if ((result) == 0x128) {                                                     \
        if (track) {                                                               \
          (track)->type = TRACK_BEZIER;                                            \
        }                                                                          \
      } else if ((result) == 0x140) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_NO_INTERP;                                         \
        }                                                                          \
      } else if ((result) == 0x152) {                                              \
        if (track) {                                                               \
          (track)->globalSeqId = (parse).ExpectInt();                              \
        } else {                                                                   \
          (parse).ExpectInt();                                                     \
        }                                                                          \
      } else if ((result) == 0x15B) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_HERMITE;                                           \
        }                                                                          \
      } else if ((result) == 0x167) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_LINEAR;                                            \
        }                                                                          \
      } else {                                                                     \
        break;                                                                     \
      }                                                                            \
      (parse).Expect(',');                                                         \
      (result) = (parse).Token((tokenText), (tokenData));                          \
    }                                                                              \
  } while (0)

inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<NTempest::C3Vector> *track, LPCSTR *tokenText, UTokenData *tokenData) {
  UINT token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<NTempest::C4Quaternion> *track, LPCSTR *tokenText, UTokenData *tokenData) {
  UINT token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

inline UINT ReadFloatTrackHeader(Parser &parse, MDLKEYTRACK<float> *track, LPCSTR *tokenText, UTokenData *tokenData) {
  UINT token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

#define READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, elements, keyType)                       \
  do {                                                                                            \
    UINT       readToken;                                                                         \
    LPCSTR     readTokenText;                                                                     \
    UTokenData readTokenData;                                                                     \
    long       readExpected = (parse).GetOptionalInt(&readToken, &readTokenText, &readTokenData); \
    if (readExpected > 0 && (track)) {                                                            \
      (track)->keys.ReserveSpace(readExpected);                                                   \
    }                                                                                             \
    (parse).Expect('{', readToken, readTokenText);                                                \
    READ_FLOAT_TRACK_HEADER_INLINE((parse), (track), &readTokenText, &readTokenData, readToken);  \
    long readActual = 0;                                                                          \
    while (readToken == 0x100) {                                                                  \
      MDLKEYFRAME<keyType> *readKey = (track)->keys.New();                                        \
      readKey->time = readTokenData.lVal;                                                         \
      (parse).Expect(':');                                                                        \
      ReadFloatKeyData((parse), reinterpret_cast<float *>(&readKey->value), (elements));          \
      (parse).Expect(',');                                                                        \
      ++readActual;                                                                               \
      MDLTRACKTYPE readType = (track)->type;                                                      \
      if (readType > TRACK_LINEAR) {                                                              \
        (parse).Expect(0x15E);                                                                    \
        ReadFloatKeyData((parse), reinterpret_cast<float *>(&readKey->inTan), (elements));        \
        (parse).Expect(',');                                                                      \
        (parse).Expect(0x18A);                                                                    \
        ReadFloatKeyData((parse), reinterpret_cast<float *>(&readKey->outTan), (elements));       \
        (parse).Expect(',');                                                                      \
      }                                                                                           \
      readToken = (parse).Token(&readTokenText, &readTokenData);                                  \
    }                                                                                             \
    (parse).Expect('}', readToken, readTokenText);                                                \
    if (readExpected >= 0 && readActual != readExpected) {                                        \
      (parse).WarningCount("key frames", readExpected, readActual);                               \
    }                                                                                             \
  } while (0)

inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<NTempest::C3Vector> *track) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 3, NTempest::C3Vector);
}

inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<float> *track) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 1, float);
}

inline void ReadObjectFloatKeyframes(Parser &parse, MDLKEYTRACK<C3Color> *track) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 3, C3Color);
}

#define WRITE_TRACK_HEADER_INLINE(indent, trackType, globalSeqId, buffer)                       \
  do {                                                                                          \
    UINT writeTrackToken = 0;                                                                   \
    switch (trackType) {                                                                        \
      case TRACK_NO_INTERP:                                                                     \
        writeTrackToken = 0x140;                                                                \
        break;                                                                                  \
      case TRACK_LINEAR:                                                                        \
        writeTrackToken = 0x167;                                                                \
        break;                                                                                  \
      case TRACK_HERMITE:                                                                       \
        writeTrackToken = 0x15B;                                                                \
        break;                                                                                  \
      case TRACK_BEZIER:                                                                        \
        writeTrackToken = 0x128;                                                                \
        break;                                                                                  \
      default:                                                                                  \
        break;                                                                                  \
    }                                                                                           \
    if (writeTrackToken) {                                                                      \
      MDL::WriteLine((buffer), "%s\t%s,\n", (indent), MDL::TokenText(writeTrackToken));         \
    }                                                                                           \
    if ((globalSeqId) != static_cast<UINT>(-1)) {                                               \
      MDL::WriteLine((buffer), "%s\t%s %d,\n", (indent), MDL::TokenText(0x152), (globalSeqId)); \
    }                                                                                           \
  } while (0)

inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<NTempest::C3Vector> &track, TSGrowableArray<char> &buffer) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<NTempest::C4Quaternion> &track, TSGrowableArray<char> &buffer) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<float> &track, TSGrowableArray<char> &buffer) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(LPCSTR indent, const MDLKEYTRACK<C3Color> &track, TSGrowableArray<char> &buffer) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

#undef WRITE_TRACK_HEADER_INLINE
#undef READ_OBJECT_FLOAT_KEYFRAMES_INLINE
#undef READ_FLOAT_TRACK_HEADER_INLINE

void         WriteFloatKeyFrames(UINT title, LPCSTR indent, const MDLKEYTRACK<float> &keyframes, TSGrowableArray<char> &buffer);
void         WriteIntKeyFrames(UINT title, LPCSTR indent, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes, TSGrowableArray<char> &buffer);
void         WriteFloatKeyFrames(UINT title, LPCSTR indent, const MDLKEYTRACK<C3Color> &keyframes, TSGrowableArray<char> &buffer);
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
void         WriteBinFloatKeyFrames(const MDLKEYTRACK<float> &track, DWORD magic, CMsgBuffer &buffer);
int          ReadBinFloatKeyFrames(MDLKEYTRACK<float> &track, CMsgBuffer &buffer, UINT &totalRead);
BOOL         ReadBinUintKeyFrames(MDLSIMPLEKEYTRACK<MDLINTKEY> &track, CMsgBuffer &buffer, UINT &totalRead);
void         WriteBinUintKeyFrames(const MDLSIMPLEKEYTRACK<MDLINTKEY> &track, DWORD magic, CMsgBuffer &buffer);
int          ReadBinFloatKeyFrames(MDLKEYTRACK<C3Color> &track, CMsgBuffer &buffer, UINT &totalRead);
int          ReadBinFloatKeyFrames(MDLKEYTRACK<NTempest::C3Vector> &track, CMsgBuffer &buffer, UINT &totalRead);
UINT         GetBinQuatKeyFramesSize(const MDLKEYTRACK<NTempest::C4Quaternion> &track);
