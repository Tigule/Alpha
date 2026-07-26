#include "Anim/AnimInternal.h"

static float EvaluateCubicPolynomial(float t, const float* coefficients) {
  float result = coefficients[0];
  for (unsigned int i = 1; i < 4; ++i) {
    result = result * t + coefficients[i];
  }
  return result;
}

void CKeyFrameTrackBase::SetNumKeys(unsigned int numKeys, unsigned int keySize) {
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

  unsigned int numSequences = seq.Count();
  if (!numSequences) {
    m_indices.ReserveSpace(1);
    m_indices.m_count = 1;
    m_indices[0].start = 0;
    m_indices[0].count = m_numKeyFrames;
    return;
  }

  m_indices.ReserveSpace(numSequences);
  m_indices.m_count = numSequences;
  if (!m_numKeyFrames) {
    memset(m_indices.m_data, 0, numSequences * sizeof(CKeySeq));
    return;
  }

  unsigned int currKeyId = 0;
  CKeyFrame   *key = m_keyFrames;
  int          priorEnd = 0;
  for (unsigned int sequence = 0; sequence < numSequences; ++sequence) {
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

unsigned int CKeyFrameTrackBase::SetAnimTime(const CBaseStatus &sequence, CKeyTrackStatus *keyStat, const InterpInfo &interpData) {
  if (SequenceNeverChanges()) {
    if (TotalKeys() == 0) {
      return 0;
    }

    ISetAnimTimeConstSeq(interpData.unique->globalSeqElapsed[m_globalSeqId], interpData.shared->globalSeqLength[m_globalSeqId], keyStat);
    return TotalKeys();
  }

  unsigned char seqId = sequence.currSeq;
  unsigned int  numKeys = NumKeysThisSeq(seqId);
  if (numKeys == 0) {
    return 0;
  }

  if (interpData.shared->seq.Count()) {
    ISetAnimTime(seqId, sequence.flags & 0x10, interpData.unique->seq[seqId].elapsed, interpData.shared->seq[seqId].time.h, keyStat);
  }

  return numKeys;
}

int CKeyFrameTrackBase::JustPastKeyForward(
    int elapsedTime,
    const CAnimSequence &seqShared,
    int seqElapsed,
    int seqIsNew,
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
    int elapsedTime,
    const CAnimSequence &seqShared,
    int seqElapsed,
    int seqIsNew,
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
    int elapsedTime,
    const CAnimSequence &seqShared,
    int seqElapsed,
    unsigned char sequenceId,
    int seqIsNew,
    const CKeyTrackStatus &prev,
    const CKeyTrackStatus &curr
) const {
  unsigned int numKeys = SequenceNeverChanges() ? TotalKeys() : NumKeysThisSeq(sequenceId);
  if (!numKeys) {
    return 0;
  }
  if (elapsedTime < 0) {
    return JustPastKeyBackward(elapsedTime, seqShared, seqElapsed, seqIsNew, prev, curr);
  }
  return JustPastKeyForward(elapsedTime, seqShared, seqElapsed, seqIsNew, prev, curr);
}

const CKeyFrame *CKeyFrameTrackBase::NextKey(const CKeyFrame *key) const {
  return reinterpret_cast<const CKeyFrame *>(reinterpret_cast<const unsigned char *>(key) + m_keyFrameSize);
}

CKeyFrame *CKeyFrameTrackBase::NextKey(CKeyFrame *key) {
  return reinterpret_cast<CKeyFrame *>(reinterpret_cast<unsigned char *>(key) + m_keyFrameSize);
}

const CKeyFrame *CKeyFrameTrackBase::GetKeyFrame(unsigned int keyId) const {
  return reinterpret_cast<const CKeyFrame *>(reinterpret_cast<const unsigned char *>(m_keyFrames) + m_keyFrameSize * keyId);
}

CKeyFrame *CKeyFrameTrackBase::GetKeyFrame(unsigned int keyId) {
  return reinterpret_cast<CKeyFrame *>(reinterpret_cast<unsigned char *>(m_keyFrames) + m_keyFrameSize * keyId);
}

unsigned int CKeyFrameTrackBase::TimeDiff(const CKeyFrame &curr, const CKeyFrame &next, unsigned int seqTime) {
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

void CKeyFrameTrackBase::ISetAnimTime(unsigned char sequenceId, int seqIsNew, int milliseconds, int endtime, CKeyTrackStatus *keyStat) {
  ASSERT(keyStat);
  ASSERT(SequenceChanges());

  if (seqIsNew) {
    keyStat->currKey = FirstKeyId(sequenceId);
    keyStat->nextKey = keyStat->currKey == LastKeyId(sequenceId) ? FirstKeyId(sequenceId) : keyStat->currKey + 1;
  }

  keyStat->currKey = FindKeyForTime(sequenceId, keyStat->currKey, milliseconds);
  unsigned int numKeys = NumKeysThisSeq(sequenceId);
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

unsigned int CKeyFrameTrackBase::FindKeyForTime(unsigned int currSeq, unsigned int currKeyId, int targettime) {
  unsigned int numKeys = NumKeysThisSeq(currSeq);
  ASSERT(numKeys > 0);

  if (numKeys == 1) {
    return FirstKeyId(currSeq);
  }

  const CKeyFrame *key = GetKeyFrame(currKeyId);
  if (targettime < key->time) {
    currKeyId = FirstKeyId(currSeq);
  } else {
    unsigned int lastKeyId = LastKeyId(currSeq);
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

unsigned int CKeyFrameTrackBase::FindKeyForTimeConstSeq(unsigned int currKeyId, int targettime) {
  ASSERT(TotalKeys() > 0);

  if (TotalKeys() == 1) {
    return 0;
  }

  unsigned int     keyId = currKeyId;
  const CKeyFrame *key = GetKeyFrame(currKeyId);
  if (targettime < key->time) {
    keyId = 0;
  } else if (currKeyId >= TotalKeys() - 1) {
    return TotalKeys() - 1;
  }

  key = GetKeyFrame(keyId + 1);
  if (targettime >= key->time) {
    unsigned int lastKeyId = TotalKeys() - 1;
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

void __fastcall Blend(const NTempest::C3Vector &previous, NTempest::C3Vector *current, int timeLeft, unsigned int blendTime) {
  ASSERT(current);
  float ratio = static_cast<float>(blendTime - timeLeft) / static_cast<float>(blendTime);
  *current = previous * (1.0f - ratio) + *current * ratio;
}

void __fastcall Blend(const NTempest::C4Quaternion &previous, NTempest::C4Quaternion *current, int timeLeft, unsigned int blendTime) {
  ASSERT(current);
  float ratio = static_cast<float>(blendTime - timeLeft) / static_cast<float>(blendTime);
  *current = NTempest::C4Quaternion::Slerp(ratio, previous, *current);
}
