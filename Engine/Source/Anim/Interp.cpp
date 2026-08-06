#include <Base/Base.h>

#include "Anim/AnimInternal.h"
#include "Tempest/c44matrix.h"

static NTempest::C44Matrix s_hermiteCoeffs(2.0f, -3.0f, 0.0f, 1.0f, 1.0f, -2.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -2.0f, 3.0f, 0.0f, 0.0f);

static NTempest::C44Matrix s_bezierCoeffs(-1.0f, 3.0f, -3.0f, 1.0f, 3.0f, -6.0f, 3.0f, 0.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

static float EvaluateCubicPolynomial(float t, const float *coefficients) {
  float result = coefficients[0];
  for (UINT i = 1; i < 4; ++i) {
    result = result * t + coefficients[i];
  }
  return result;
}

void CKeyFrameTrackBase::SetNumKeys(UINT numKeys, UINT keySize) {
  m_keyFrameSize = keySize;
  m_keyFrames = static_cast<CKeyFrame *>(SMemAlloc(numKeys * keySize, __FILE__, __LINE__, 0));
}

void CKeyFrameTrackBase::AddKey(int time) {
  GetKeyFrame(m_numKeyFrames++)->time = time;
}

void CKeyFrameTrackBase::SetSequenceIndices(const CArray<CAnimSequence> &seq) {
  if (!SequenceChanges()) {
    return;
  }

  UINT numSequences = seq.Count();
  if (!numSequences) {
    m_indices.ReserveSpace(1);
    m_indices.SetCount(1);
    m_indices[0].start = 0;
    m_indices[0].count = m_numKeyFrames;
    return;
  }

  m_indices.ReserveSpace(numSequences);
  m_indices.SetCount(numSequences);
  if (!m_numKeyFrames) {
    m_indices.Zero();
    return;
  }

  UINT       currKeyId = 0;
  CKeyFrame *key = m_keyFrames;
  int        priorEnd = 0;
  for (UINT sequence = 0; sequence < numSequences; ++sequence) {
    m_indices[sequence].count = 0;
    if (seq[sequence].time.l < priorEnd) {
      currKeyId = 0;
      key = m_keyFrames;
    }

    while (currKeyId < m_numKeyFrames) {
      if (key->time > seq[sequence].time.h || key->time >= seq[sequence].time.l) {
        break;
      }
      ++currKeyId;
      key = NextKey(key);
    }

    m_indices[sequence].start = currKeyId;
    while (currKeyId < m_numKeyFrames && key->time <= seq[sequence].time.h) {
      ++m_indices[sequence].count;
      ++currKeyId;
      key = NextKey(key);
    }
    priorEnd = seq[sequence].time.h;
  }
}

UINT CKeyFrameTrackBase::SetAnimTime(const CBaseStatus &sequence, CKeyTrackStatus *keyStat, const InterpInfo &interpData) {
  if (SequenceNeverChanges()) {
    if (TotalKeys() == 0) {
      return 0;
    }

    ISetAnimTimeConstSeq(interpData.unique->globalSeqElapsed[m_globalSeqId], interpData.shared->globalSeqLength[m_globalSeqId], keyStat);
    return TotalKeys();
  }

  BYTE seqId = sequence.currSeq;
  UINT numKeys = NumKeysThisSeq(seqId);
  if (numKeys == 0) {
    return 0;
  }

  if (interpData.shared->seq.Count()) {
    ISetAnimTime(seqId, sequence.flags & 0x10, interpData.unique->seq[seqId].elapsed, interpData.shared->seq[seqId].time.h, keyStat);
  }

  return numKeys;
}

int CKeyFrameTrackBase::JustPastKeyForward(
    int                    elapsedTime,
    const CAnimSequence   &seqShared,
    int                    seqElapsed,
    int                    seqIsNew,
    const CKeyTrackStatus &prev,
    const CKeyTrackStatus &curr
) const {
  const CKeyFrame *key = GetKeyFrame(curr.currKey);
  if (seqIsNew) {
    return seqElapsed >= key->time;
  }
  if (elapsedTime >= seqShared.time.h - seqShared.time.l) {
    return 1;
  }
  if (curr.currKey != prev.currKey) {
    return 1;
  }

  int timePastKey = prev.timepastkey + elapsedTime;
  if (prev.timepastkey < 0) {
    return timePastKey >= 0;
  }
  return timePastKey + seqShared.time.l - seqShared.time.h >= 0;
}

int CKeyFrameTrackBase::JustPastKeyBackward(
    int                    elapsedTime,
    const CAnimSequence   &seqShared,
    int                    seqElapsed,
    int                    seqIsNew,
    const CKeyTrackStatus &prev,
    const CKeyTrackStatus &curr
) const {
  const CKeyFrame *key = GetKeyFrame(curr.currKey);
  if (seqIsNew) {
    return seqElapsed <= key->time;
  }
  if (-elapsedTime >= seqShared.time.h - seqShared.time.l) {
    return 1;
  }
  if (curr.currKey != prev.currKey) {
    return 1;
  }

  int timePastKey = prev.timepastkey + elapsedTime;
  if (prev.timepastkey > 0) {
    return timePastKey <= 0;
  }
  return timePastKey + seqShared.time.h - seqShared.time.l <= 0;
}

int CKeyFrameTrackBase::JustPastKey(
    int                    elapsedTime,
    const CAnimSequence   &seqShared,
    int                    seqElapsed,
    BYTE                   sequenceId,
    int                    seqIsNew,
    const CKeyTrackStatus &prev,
    const CKeyTrackStatus &curr
) const {
  UINT numKeys = SequenceNeverChanges() ? TotalKeys() : NumKeysThisSeq(sequenceId);
  if (!numKeys) {
    return 0;
  }
  if (elapsedTime < 0) {
    return JustPastKeyBackward(elapsedTime, seqShared, seqElapsed, seqIsNew, prev, curr);
  }
  return JustPastKeyForward(elapsedTime, seqShared, seqElapsed, seqIsNew, prev, curr);
}

const CKeyFrame *CKeyFrameTrackBase::NextKey(const CKeyFrame *key) const {
  return reinterpret_cast<const CKeyFrame *>(reinterpret_cast<const BYTE *>(key) + m_keyFrameSize);
}

CKeyFrame *CKeyFrameTrackBase::NextKey(CKeyFrame *key) {
  return reinterpret_cast<CKeyFrame *>(reinterpret_cast<BYTE *>(key) + m_keyFrameSize);
}

const CKeyFrame *CKeyFrameTrackBase::GetKeyFrame(UINT keyId) const {
  return reinterpret_cast<const CKeyFrame *>(reinterpret_cast<const BYTE *>(m_keyFrames) + m_keyFrameSize * keyId);
}

CKeyFrame *CKeyFrameTrackBase::GetKeyFrame(UINT keyId) {
  return reinterpret_cast<CKeyFrame *>(reinterpret_cast<BYTE *>(m_keyFrames) + m_keyFrameSize * keyId);
}

UINT CKeyFrameTrackBase::TimeDiff(const CKeyFrame &curr, const CKeyFrame &next, UINT seqTime) {
  int timeDiff = next.time - curr.time;
  if (timeDiff < 0) {
    timeDiff += seqTime;
  }
  return timeDiff;
}

void CKeyFrameTrackBase::ISetAnimTimeConstSeq(int milliseconds, int endtime, CKeyTrackStatus *keyStat) {
  ASSERT(keyStat);
  ASSERT(TotalKeys() > 0);

  keyStat->currKey = FindKeyForTimeConstSeq(keyStat->currKey, milliseconds);
  if (TotalKeys() == 1) {
    keyStat->timepastkey = milliseconds - GetKeyFrame(keyStat->currKey)->time;
    keyStat->nextKey = keyStat->currKey;
    return;
  }

  const CKeyFrame *key = GetKeyFrame(keyStat->currKey);
  if (keyStat->currKey == 0 && key->time > milliseconds) {
    keyStat->currKey = FindKeyForTimeConstSeq(0, endtime);
  }

  keyStat->timepastkey = milliseconds - key->time;
  keyStat->nextKey = keyStat->currKey == TotalKeys() - 1 ? 0 : keyStat->currKey + 1;
}

void CKeyFrameTrackBase::ISetAnimTime(BYTE sequenceId, int seqIsNew, int milliseconds, int endtime, CKeyTrackStatus *keyStat) {
  ASSERT(keyStat);
  ASSERT(SequenceChanges());

  if (seqIsNew) {
    keyStat->currKey = FirstKeyId(sequenceId);
    keyStat->nextKey = keyStat->currKey == LastKeyId(sequenceId) ? FirstKeyId(sequenceId) : keyStat->currKey + 1;
  }

  keyStat->currKey = FindKeyForTime(sequenceId, keyStat->currKey, milliseconds);
  UINT numKeys = NumKeysThisSeq(sequenceId);
  ASSERT(numKeys > 0);

  if (numKeys == 1) {
    keyStat->timepastkey = milliseconds - GetKeyFrame(keyStat->currKey)->time;
    keyStat->nextKey = keyStat->currKey;
    return;
  }

  const CKeyFrame *key = GetKeyFrame(keyStat->currKey);
  if (keyStat->currKey == FirstKeyId(sequenceId) && key->time > milliseconds) {
    keyStat->currKey = FindKeyForTime(sequenceId, keyStat->currKey, endtime);
  }

  keyStat->timepastkey = milliseconds - key->time;
  keyStat->nextKey = keyStat->currKey == LastKeyId(sequenceId) ? FirstKeyId(sequenceId) : keyStat->currKey + 1;
}

UINT CKeyFrameTrackBase::FindKeyForTime(UINT currSeq, UINT currKeyId, int targettime) {
  UINT numKeys = NumKeysThisSeq(currSeq);
  ASSERT(numKeys > 0);

  if (numKeys == 1) {
    return FirstKeyId(currSeq);
  }

  const CKeyFrame *key = GetKeyFrame(currKeyId);
  if (targettime < key->time) {
    currKeyId = FirstKeyId(currSeq);
  } else {
    UINT lastKeyId = LastKeyId(currSeq);
    if (currKeyId >= lastKeyId) {
      return lastKeyId;
    }
  }

  const CKeyFrame *nextKey = GetKeyFrame(currKeyId + 1);
  while (targettime >= nextKey->time) {
    ++currKeyId;
    if (currKeyId == LastKeyId(currSeq)) {
      break;
    }
    nextKey = NextKey(nextKey);
  }

  return currKeyId;
}

UINT CKeyFrameTrackBase::FindKeyForTimeConstSeq(UINT currKeyId, int targettime) {
  ASSERT(TotalKeys() > 0);

  if (TotalKeys() == 1) {
    return 0;
  }

  UINT             keyId = currKeyId;
  const CKeyFrame *key = GetKeyFrame(currKeyId);
  if (targettime < key->time) {
    keyId = 0;
  } else if (currKeyId >= TotalKeys() - 1) {
    return TotalKeys() - 1;
  }

  key = GetKeyFrame(keyId + 1);
  if (targettime >= key->time) {
    UINT lastKeyId = TotalKeys() - 1;
    do {
      ++keyId;
      if (keyId == lastKeyId) {
        break;
      }
      key = NextKey(key);
    } while (targettime >= key->time);
  }

  return keyId;
}

template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateHermite(
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &currkey,
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &nextkey,
    float                                                    ratio,
    NTempest::C4Quaternion                                  *transform
) {
  ASSERT(transform);
  NTempest::C4Quaternion curr = currkey.transform;
  NTempest::C4Quaternion next = nextkey.transform;
  NTempest::C4Quaternion outTangent = currkey.outTan;
  NTempest::C4Quaternion inTangent = nextkey.inTan;
  *transform = NTempest::C4Quaternion::Squad(ratio, curr, next, outTangent, inTangent);
}

template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateBezier(
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &,
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &,
    float,
    NTempest::C4Quaternion *
) {
  FATALASSERT(0);
}

template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateLinear(
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> &currkey,
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> &nextkey,
    float                                                    ratio,
    NTempest::C4Quaternion                                  *transform
) {
  ASSERT(transform);
  NTempest::C4Quaternion curr = currkey.transform;
  NTempest::C4Quaternion next = nextkey.transform;
  *transform = NTempest::C4Quaternion::Slerp(ratio, curr, next);
}

template <>
void CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>::InterpolateHermite(
    const CSplineKeyFrame<NTempest::C3Vector> &currkey,
    const CSplineKeyFrame<NTempest::C3Vector> &nextkey,
    float                                      ratio,
    NTempest::C3Vector                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_hermiteCoeffs[i]);
  }
  *transform =
      currkey.transform * coefficients[0] + currkey.outTan * coefficients[1] + nextkey.inTan * coefficients[2] + nextkey.transform * coefficients[3];
}

template <>
void CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>::InterpolateBezier(
    const CSplineKeyFrame<NTempest::C3Vector> &currkey,
    const CSplineKeyFrame<NTempest::C3Vector> &nextkey,
    float                                      ratio,
    NTempest::C3Vector                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_bezierCoeffs[i]);
  }
  *transform =
      currkey.transform * coefficients[0] + currkey.outTan * coefficients[1] + nextkey.inTan * coefficients[2] + nextkey.transform * coefficients[3];
}

template <>
void CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>::InterpolateLinear(
    const CLinearKeyFrame<NTempest::C3Vector> &currkey,
    const CLinearKeyFrame<NTempest::C3Vector> &nextkey,
    float                                      ratio,
    NTempest::C3Vector                        *transform
) {
  ASSERT(transform);
  *transform = currkey.transform * (1.0f - ratio) + nextkey.transform * ratio;
}

template <>
void CKeyFrameTrack<C3Color, C3Color>::InterpolateHermite(
    const CSplineKeyFrame<C3Color> &currkey,
    const CSplineKeyFrame<C3Color> &nextkey,
    float                           ratio,
    C3Color                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_hermiteCoeffs[i]);
  }
  *transform = C3Color(
      currkey.transform.r * coefficients[0] + currkey.outTan.r * coefficients[1] + nextkey.inTan.r * coefficients[2] +
          nextkey.transform.r * coefficients[3],
      currkey.transform.g * coefficients[0] + currkey.outTan.g * coefficients[1] + nextkey.inTan.g * coefficients[2] +
          nextkey.transform.g * coefficients[3],
      currkey.transform.b * coefficients[0] + currkey.outTan.b * coefficients[1] + nextkey.inTan.b * coefficients[2] +
          nextkey.transform.b * coefficients[3]
  );
}

template <>
void CKeyFrameTrack<C3Color, C3Color>::InterpolateBezier(
    const CSplineKeyFrame<C3Color> &currkey,
    const CSplineKeyFrame<C3Color> &nextkey,
    float                           ratio,
    C3Color                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_bezierCoeffs[i]);
  }
  *transform = C3Color(
      currkey.transform.r * coefficients[0] + currkey.outTan.r * coefficients[1] + nextkey.inTan.r * coefficients[2] +
          nextkey.transform.r * coefficients[3],
      currkey.transform.g * coefficients[0] + currkey.outTan.g * coefficients[1] + nextkey.inTan.g * coefficients[2] +
          nextkey.transform.g * coefficients[3],
      currkey.transform.b * coefficients[0] + currkey.outTan.b * coefficients[1] + nextkey.inTan.b * coefficients[2] +
          nextkey.transform.b * coefficients[3]
  );
}

template <>
void CKeyFrameTrack<C3Color, C3Color>::InterpolateLinear(
    const CLinearKeyFrame<C3Color> &currkey,
    const CLinearKeyFrame<C3Color> &nextkey,
    float                           ratio,
    C3Color                        *transform
) {
  ASSERT(transform);
  float inverseRatio = 1.0f - ratio;
  *transform = C3Color(
      currkey.transform.r * inverseRatio + nextkey.transform.r * ratio, currkey.transform.g * inverseRatio + nextkey.transform.g * ratio,
      currkey.transform.b * inverseRatio + nextkey.transform.b * ratio
  );
}

template <>
void CKeyFrameTrack<float, float>::InterpolateHermite(
    const CSplineKeyFrame<float> &currkey,
    const CSplineKeyFrame<float> &nextkey,
    float                         ratio,
    float                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_hermiteCoeffs[i]);
  }
  *transform =
      currkey.transform * coefficients[0] + currkey.outTan * coefficients[1] + nextkey.inTan * coefficients[2] + nextkey.transform * coefficients[3];
}

template <>
void CKeyFrameTrack<float, float>::InterpolateBezier(
    const CSplineKeyFrame<float> &currkey,
    const CSplineKeyFrame<float> &nextkey,
    float                         ratio,
    float                        *transform
) {
  ASSERT(transform);
  float coefficients[4];
  for (UINT i = 0; i < 4; ++i) {
    coefficients[i] = EvaluateCubicPolynomial(ratio, s_bezierCoeffs[i]);
  }
  *transform =
      currkey.transform * coefficients[0] + currkey.outTan * coefficients[1] + nextkey.inTan * coefficients[2] + nextkey.transform * coefficients[3];
}

template <>
void CKeyFrameTrack<float, float>::InterpolateLinear(
    const CLinearKeyFrame<float> &currkey,
    const CLinearKeyFrame<float> &nextkey,
    float                         ratio,
    float                        *transform
) {
  ASSERT(transform);
  *transform = currkey.transform * (1.0f - ratio) + nextkey.transform * ratio;
}

template <>
void CKeyFrameTrack<UINT, UINT>::InterpolateHermite(const CSplineKeyFrame<UINT> &, const CSplineKeyFrame<UINT> &, float, UINT *) {
  FATALASSERT(0);
}

template <>
void CKeyFrameTrack<UINT, UINT>::InterpolateBezier(const CSplineKeyFrame<UINT> &, const CSplineKeyFrame<UINT> &, float, UINT *) {
  FATALASSERT(0);
}

template <>
void CKeyFrameTrack<UINT, UINT>::InterpolateLinear(const CLinearKeyFrame<UINT> &, const CLinearKeyFrame<UINT> &, float, UINT *) {
  FATALASSERT(0);
}

void Blend(const NTempest::C3Vector &previous, NTempest::C3Vector *current, int timeLeft, UINT blendTime) {
  ASSERT(current);
  float ratio = static_cast<float>(blendTime - timeLeft) / static_cast<float>(blendTime);
  *current = previous * (1.0f - ratio) + *current * ratio;
}

void Blend(const NTempest::C4Quaternion &previous, NTempest::C4Quaternion *current, int timeLeft, UINT blendTime) {
  ASSERT(current);
  float ratio = static_cast<float>(blendTime - timeLeft) / static_cast<float>(blendTime);
  *current = NTempest::C4Quaternion::Slerp(ratio, previous, *current);
}
