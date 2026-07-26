#pragma once

#include "MDLTypes.h"

class CMsgBuffer;
class CMDLStatus;
class Parser;
class TSet;
union UTokenData;

void __fastcall ReadFloatKeyData(
    Parser &parse,
    float *entry,
    unsigned int elements
);
unsigned int __fastcall ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track,
    const char **tokenText,
    UTokenData *tokenData
);
unsigned int __fastcall ReadIntTrackHeader(
    Parser &parse,
    MDLSIMPLEKEYTRACK<MDLINTKEY> *track,
    const char **tokenText,
    UTokenData *tokenData
);
unsigned int __fastcall ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<NTempest::C4Quaternion> *track,
    const char **tokenText,
    UTokenData *tokenData
);
unsigned int __fastcall ReadFloatTrackHeader(
    Parser &parse,
    MDLKEYTRACK<float> *track,
    const char **tokenText,
    UTokenData *tokenData
);
void __fastcall ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<NTempest::C3Vector> *track
);
void __fastcall ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<float> *track
);
void __fastcall ReadObjectFloatKeyframes(
    Parser &parse,
    MDLKEYTRACK<C3Color> *track
);
void __fastcall WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C3Vector> &track,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<float> &track,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteTrackHeader(
    const char *indent,
    const MDLKEYTRACK<C3Color> &track,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<float> &keyframes,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteIntKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyframes,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteFloatKeyFrames(
    unsigned int title,
    const char *indent,
    const MDLKEYTRACK<C3Color> &keyframes,
    TSGrowableArray<char> &buffer
);
int __fastcall ReadObjectPtrs(MDLDATA *data, CMDLStatus *status);
const float *__fastcall WriteKeyData(
    TSGrowableArray<char> &buffer,
    const float *entry,
    unsigned int elements
);
const unsigned int *__fastcall WriteUintKeyData(
    TSGrowableArray<char> &buffer,
    const unsigned int *entry,
    unsigned int elements
);
void __fastcall AddObjectErrors(TSet &errors);
void __fastcall ReadObjectName(Parser &parse, char *name);
int __fastcall ReadObjectBody(
    Parser &parse,
    unsigned int savedToken,
    NTempest::C3Vector *pivot,
    MDLGENOBJECT *object,
    CMDLStatus *status
);
void __fastcall ReadObjectEnd(
    TSet &errors,
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
);
void __fastcall ReadBinObjectEnd(
    MDLDATA &data,
    MDLGENOBJECT *object,
    unsigned long listIndex,
    unsigned long listMask
);
int __fastcall IExpectAnimation(
    Parser &parse,
    unsigned int *savedToken,
    const char **tokenText
);
void __fastcall WriteObjectTrailer(
    const MDLGENOBJECT &object,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteObjectHeader(
    const MDLDATA &data,
    const MDLGENOBJECT &object,
    unsigned int title,
    int writeIndex,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteOptionalVertex(
    unsigned int title,
    const char *indent,
    const NTempest::C3Vector &vertex,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteOptionalFloat(
    unsigned int title,
    const char *indent,
    float value,
    TSGrowableArray<char> &buffer
);
void __fastcall WriteBounds(
    const CMdlBounds &bounds,
    const char *indent,
    TSGrowableArray<char> &buffer
);
void __fastcall SkipUnknown(CMsgBuffer &buffer, unsigned int &totalRead);
unsigned int __fastcall GetBinGenObjectSize(const MDLGENOBJECT &object);
int __fastcall WriteBinGenObject(
    const MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *status
);
int __fastcall ReadBinGenObject(
    MDLGENOBJECT &object,
    CMsgBuffer &buffer,
    CMDLStatus *status,
    unsigned int &totalRead
);
void __fastcall WriteBinQuatKeyFrames(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int __fastcall ReadBinQuatKeyFrames(
    MDLKEYTRACK<NTempest::C4Quaternion> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
void __fastcall WriteBinFloatKeyFrames(
    const MDLKEYTRACK<float> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int __fastcall ReadBinFloatKeyFrames(
    MDLKEYTRACK<float> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
int __fastcall ReadBinUintKeyFrames(
    MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
void __fastcall WriteBinUintKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &track,
    unsigned long magic,
    CMsgBuffer &buffer
);
int __fastcall ReadBinFloatKeyFrames(
    MDLKEYTRACK<C3Color> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
int __fastcall ReadBinFloatKeyFrames(
    MDLKEYTRACK<NTempest::C3Vector> &track,
    CMsgBuffer &buffer,
    unsigned int &totalRead
);
unsigned int __fastcall GetBinQuatKeyFramesSize(
    const MDLKEYTRACK<NTempest::C4Quaternion> &track
);
