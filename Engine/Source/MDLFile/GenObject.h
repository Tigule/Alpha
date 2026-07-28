#pragma once

#include "MDLTypes.h"
#include "Parser.h"

class CMsgBuffer;
class CMDLStatus;
class Parser;
class TSet;
union UTokenData;

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void ReadFloatKeyData(
    Parser &parse,
    float *entry,
    unsigned int elements
);
inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track,
    const char **tokenText,
    UTokenData *tokenData
);
unsigned int ReadIntTrackHeader(
    Parser &parse,
    MDLSIMPLEKEYTRACK<MDLINTKEY> *track,
    const char **tokenText,
    UTokenData *tokenData
);
inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C4Quaternion> *track,
    const char **tokenText,
    UTokenData *tokenData
);
inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<float> *track,
    const char **tokenText,
    UTokenData *tokenData
);
inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track
);
inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<float> *track
);
inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<C3Color> *track
);
inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C3Vector> &track,
    TSGrowableArray<char> &buffer
);
inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    TSGrowableArray<char> &buffer
);
inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<float> &track,
    TSGrowableArray<char> &buffer
);
inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<C3Color> &track,
    TSGrowableArray<char> &buffer
);

#define READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, result) \
  do {                                                                             \
    (result) = (parse).Token((tokenText), (tokenData));                             \
    for (;;) {                                                                     \
      if ((result) == 0x128) {                                                     \
        if (track) {                                                               \
          (track)->type = TRACK_BEZIER;                                             \
        }                                                                          \
      } else if ((result) == 0x140) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_NO_INTERP;                                          \
        }                                                                          \
      } else if ((result) == 0x152) {                                              \
        if (track) {                                                               \
          (track)->globalSeqId = (parse).ExpectInt();                              \
        } else {                                                                   \
          (parse).ExpectInt();                                                     \
        }                                                                          \
      } else if ((result) == 0x15B) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_HERMITE;                                            \
        }                                                                          \
      } else if ((result) == 0x167) {                                              \
        if (track) {                                                               \
          (track)->type = TRACK_LINEAR;                                             \
        }                                                                          \
      } else {                                                                     \
        break;                                                                     \
      }                                                                            \
      (parse).Expect(',');                                                         \
      (result) = (parse).Token((tokenText), (tokenData));                           \
    }                                                                              \
  } while (0)

inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C4Quaternion> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

inline unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<float> *track,
    const char **tokenText,
    UTokenData *tokenData
) {
  unsigned int token;
  READ_FLOAT_TRACK_HEADER_INLINE(parse, track, tokenText, tokenData, token);
  return token;
}

#define READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, elements, keyType)              \
  do {                                                                                   \
    unsigned int readToken;                                                              \
    const char *readTokenText;                                                           \
    UTokenData readTokenData;                                                            \
    long readExpected = (parse).GetOptionalInt(&readToken, &readTokenText, &readTokenData); \
    if (readExpected > 0 && (track)) {                                                   \
      (track)->keys.ReserveSpace(readExpected);                                          \
    }                                                                                    \
    (parse).Expect('{', readToken, readTokenText);                                       \
    READ_FLOAT_TRACK_HEADER_INLINE((parse), (track), &readTokenText, &readTokenData, readToken); \
    long readActual = 0;                                                                \
    while (readToken == 0x100) {                                                        \
      MDLKEYFRAME<keyType> *readKey = (track)->keys.New();                              \
      readKey->time = readTokenData.lVal;                                               \
      (parse).Expect(':');                                                               \
      ReadFloatKeyData(                                                                  \
          (parse),                                                                       \
          reinterpret_cast<float *>(&readKey->value),                                   \
          (elements)                                                                     \
      );                                                                                 \
      (parse).Expect(',');                                                               \
      ++readActual;                                                                      \
      MDLTRACKTYPE readType = (track)->type;                                             \
      if (readType > TRACK_LINEAR) {                                                     \
        (parse).Expect(0x15E);                                                           \
        ReadFloatKeyData(                                                                \
            (parse),                                                                     \
            reinterpret_cast<float *>(&readKey->inTan),                                 \
            (elements)                                                                   \
        );                                                                               \
        (parse).Expect(',');                                                             \
        (parse).Expect(0x18A);                                                           \
        ReadFloatKeyData(                                                                \
            (parse),                                                                     \
            reinterpret_cast<float *>(&readKey->outTan),                                \
            (elements)                                                                   \
        );                                                                               \
        (parse).Expect(',');                                                             \
      }                                                                                  \
      readToken = (parse).Token(&readTokenText, &readTokenData);                         \
    }                                                                                    \
    (parse).Expect('}', readToken, readTokenText);                                       \
    if (readExpected >= 0 && readActual != readExpected) {                               \
      (parse).WarningCount("key frames", readExpected, readActual);                     \
    }                                                                                    \
  } while (0)

inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 3, NTempest::C3Vector);
}

inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<float> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 1, float);
}

inline void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<C3Color> *track
) {
  READ_OBJECT_FLOAT_KEYFRAMES_INLINE(parse, track, 3, C3Color);
}

#define WRITE_TRACK_HEADER_INLINE(indent, trackType, globalSeqId, buffer)       \
  do {                                                                          \
    unsigned int writeTrackToken = 0;                                           \
    switch (trackType) {                                                        \
      case TRACK_NO_INTERP: writeTrackToken = 0x140; break;                    \
      case TRACK_LINEAR:      writeTrackToken = 0x167; break;                  \
      case TRACK_HERMITE:     writeTrackToken = 0x15B; break;                  \
      case TRACK_BEZIER:      writeTrackToken = 0x128; break;                  \
      default: break;                                                           \
    }                                                                           \
    if (writeTrackToken) {                                                      \
      MDL::WriteLine((buffer), "%s\t%s,\n", (indent), MDL::TokenText(writeTrackToken)); \
    }                                                                           \
    if ((globalSeqId) != static_cast<unsigned int>(-1)) {                       \
      MDL::WriteLine(                                                           \
          (buffer),                                                             \
          "%s\t%s %d,\n",                                                     \
          (indent),                                                             \
          MDL::TokenText(0x152),                                                \
          (globalSeqId)                                                         \
      );                                                                        \
    }                                                                           \
  } while (0)

inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C3Vector> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<float> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

inline void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<C3Color> &track,
    TSGrowableArray<char> &buffer
) {
  WRITE_TRACK_HEADER_INLINE(indent, track.type, track.globalSeqId, buffer);
}

#undef WRITE_TRACK_HEADER_INLINE
#undef READ_OBJECT_FLOAT_KEYFRAMES_INLINE
#undef READ_FLOAT_TRACK_HEADER_INLINE

void WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<float> &keyframes,
    TSGrowableArray<char> &buffer
);
void WriteIntKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes,
    TSGrowableArray<char> &buffer
);
void WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<C3Color> &keyframes,
    TSGrowableArray<char> &buffer
);
int ReadObjectPtrs(MDLDATA *data, CMDLStatus *status);
const float *WriteKeyData(
    TSGrowableArray<char> &buffer,
    const float *entry,
    unsigned int elements
);
const unsigned int *WriteUintKeyData(
    TSGrowableArray<char> &buffer,
    const unsigned int *entry,
    unsigned int elements
);
void AddObjectErrors(TSet &errors);
void ReadObjectName(Parser &parse, char *name);
int ReadObjectBody(
    Parser &parse,
    unsigned int savedToken,
    NTempest::C3Vector *pivot,
    MDLGENOBJECT *object,
    CMDLStatus *status
);
void ReadObjectEnd(
    TSet &errors,
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
);
void ReadBinObjectEnd(
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
);
int IExpectAnimation(
    Parser &parse,
    unsigned int *savedToken,
    const char **tokenText
);
void WriteObjectTrailer(
    const MDLGENOBJECT &object,
    TSGrowableArray<char> &buffer
);
void WriteObjectHeader(
    const MDLDATA &data,
    const MDLGENOBJECT &object,
    unsigned int title,
    int writeIndex,
    TSGrowableArray<char> &buffer
);
void WriteOptionalVertex(
    unsigned int title,
    const char *indent,
    const NTempest::C3Vector &vertex,
    TSGrowableArray<char> &buffer
);
void WriteOptionalFloat(
    unsigned int title,
    const char *indent,
    float value,
    TSGrowableArray<char> &buffer
);
void WriteBounds(
    const CMdlBounds &bounds,
    const char *indent,
    TSGrowableArray<char> &buffer
);
void SkipUnknown(CMsgBuffer &buffer, unsigned int &totalRead);
unsigned int GetBinGenObjectSize(const MDLGENOBJECT &object);
int WriteBinGenObject(
    const MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *status
);
int ReadBinGenObject(
    MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *status,
    unsigned int &totalRead
);
void WriteBinQuatKeyFrames(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int ReadBinQuatKeyFrames(
    MDLKEYTRACK<NTempest::C4Quaternion> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
void WriteBinFloatKeyFrames(
    const MDLKEYTRACK<float> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int ReadBinFloatKeyFrames(
    MDLKEYTRACK<float> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
int ReadBinUintKeyFrames(
    MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
void WriteBinUintKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int ReadBinFloatKeyFrames(
    MDLKEYTRACK<C3Color> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
int ReadBinFloatKeyFrames(
    MDLKEYTRACK<NTempest::C3Vector> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
unsigned int GetBinQuatKeyFramesSize(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track
);
