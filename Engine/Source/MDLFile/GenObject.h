#pragma once

#include "MDLTypes.h"

class CMsgBuffer;
class CMDLStatus;
class Parser;
class TSet;
union UTokenData;

void ReadFloatKeyData(
    Parser &parse,
    float *entry,
    unsigned int elements
);
unsigned int ReadFloatTrackHeader(
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
unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C4Quaternion> *track,
    const char **tokenText,
    UTokenData *tokenData
);
unsigned int ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<float> *track,
    const char **tokenText,
    UTokenData *tokenData
);
void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track
);
void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<float> *track
);
void ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<C3Color> *track
);
void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C3Vector> &track,
    TSGrowableArray<char> &buffer
);
void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    TSGrowableArray<char> &buffer
);
void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<float> &track,
    TSGrowableArray<char> &buffer
);
void WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<C3Color> &track,
    TSGrowableArray<char> &buffer
);
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
