#include <Base/Base.h>
#include <Gx/Gx.h>
#include <BLPFile/blp.h>

#include "Services/Lightning.h"

#include "Tempest/c44matrix.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

#include <math.h>

static NTempest::CRndSeed sRandSeed;

CLightning::CLightning() {
  mAvgSegLen = -2.0f;
  mWidth = 1.0f;
  mRebuildPoints = 1;
  mAccTime = 0.0f;
  mTexture = 0;
}

void CLightning::BuildStroke(TSFixedArray<NTempest::C3Vector> &points) {
  NTempest::C3Vector diff = mDstPos - mSrcPos;
  float              length = diff.Mag();
  UINT               numPoints = (UINT)(length / mAvgSegLen + 2.0f) + 1;
  float              ooNumPoints = 1.0f / (numPoints - 1);
  float              noiseScale = length * mNoiseScale;

  points.SetCount(numPoints);
  points[0] = mSrcPos;
  points[numPoints - 1] = mDstPos;

  for (UINT i = 1; i != numPoints - 1; ++i) {
    NTempest::C3Vector tmp;
    tmp.x = NTempest::CRandom::reals_(sRandSeed);
    tmp.y = NTempest::CRandom::reals_(sRandSeed);
    tmp.z = NTempest::CRandom::reals_(sRandSeed);
    points[i] = points[0] + diff * (float)i * ooNumPoints;
    points[i] += tmp * noiseScale;
  }
}

void CLightning::Update(float elapsed) {
  if (mTexCoordScale != 0.0f) {
    mAccTime = fmod(elapsed + mAccTime, mTexCoordScale);
  } else {
    mAccTime = 0.0f;
  }

  if (mRebuildPoints) {
    BuildStroke(mPoints);

    UINT  numPos = 2 * mPoints.Count();
    float ooNumPos = 1.0f / (numPos - 2);

    if (numPos != mTexCoords.Count()) {
      mPos.SetCount(numPos);
      mTexCoords.SetCount(numPos);
      mIndices.SetCount(numPos);

      for (UINT i = 0; i < numPos; i += 2) {
        float x = (float)i * ooNumPos;
        mTexCoords[i] = NTempest::C2Vector(x, 0.0f);
        mTexCoords[i + 1] = NTempest::C2Vector(x, 1.0f);
        mIndices[i] = i;
        mIndices[i + 1] = i + 1;
      }

      mTexCoords[0] = mTexCoords[1] = NTempest::C2Vector(0.0f, 0.5f);
      mTexCoords[numPos - 2] = mTexCoords[numPos - 1] = NTempest::C2Vector(1.0f, 0.5f);
    }
    mRebuildPoints = 0;
  }

  static TSFixedArray_<NTempest::C3Vector, 'Ligh', __LINE__> sPoints;
  UINT i = 1;
  UINT end = mPoints.Count() - 1;
  BuildStroke(sPoints);

  for (; i < end; ++i) {
    mPoints[i] = mPoints[i] * 0.75f + sPoints[i] * 0.25f;
  }
}

CLightning::~CLightning() {
  if (mTexture) {
    HandleClose(mTexture);
  }
}

void CLightning::Render(UINT boltId, const NTempest::C3Vector &cameraPos) {
  if (mCoordUpdateData.callback) {
    NTempest::C3Vector sourcePos = mSrcPos;
    NTempest::C3Vector destPos = mDstPos;
    mCoordUpdateData.callback(mCoordUpdateData.context, boltId, &sourcePos, &destPos);
    mSrcPos = sourcePos;
    mDstPos = destPos;
    mRebuildPoints = 1;
  }

  static NTempest::C44Matrix identity;
  static NTempest::C44Matrix worldToView;
  static NTempest::C44Matrix particleToView;
  GxXformView(worldToView);
  GxXformSetView(identity);
  GxXformPush(GxXform_World);
  GxXformIdentity(GxXform_World);
  GxXformPush(GxXform_Tex0);

  NTempest::C44Matrix viewRelative;
  *viewRelative.Row3AsVec3() = -cameraPos;
  particleToView = viewRelative * worldToView;

  UINT numPoints = mPoints.Count();
  mPos[0] *= 0.0f;
  mPos[1] *= 0.0f;

  NTempest::C3Vector p;
  p = mPoints[0] * particleToView;
  for (UINT i = 0; i < 2 * numPoints - 2; i += 2) {
    NTempest::C3Vector q = mPoints[(i + 2) / 2] * particleToView;
    NTempest::C3Vector d = q - p;
    NTempest::C3Vector perp(-d.y, d.x, 0.0f);
    float              mag = perp.Mag();
    if (mag > 0.001f) {
      perp *= 1.0f / mag;
    }

    mPos[i] += (p + perp * mWidth) * 0.5f;
    mPos[i + 1] += (p - perp * mWidth) * 0.5f;
    mPos[i + 2] = (q + perp * mWidth) * 0.5f;
    mPos[i + 3] = (q - perp * mWidth) * 0.5f;
    p = q;
  }

  mPos[0] = mPos[1] = mPoints[0] * particleToView;
  mPos[2 * numPoints - 2] = mPos[2 * numPoints - 1] = mPoints[numPoints - 1] * particleToView;

  NTempest::C3Vector texTranslate(-(mAccTime / (mTexCoordScale != 0.0f ? mTexCoordScale : 1.0f)), 0.0f, 0.0f);
  GxXformTranslate(GxXform_Tex0, texTranslate);

  GxRsPush();
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(mColor.a, 0, 0, 0));
  GxRsSet(GxRs_MatEmissive, mColor);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_Blend, GxBlend_Add);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(mTexture, 1, 0));
  static NTempest::C3Vector zup(0.0f, 0.0f, 1.0f);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxPrimLockVertexPtrs(mPos.Count(), &mPos[0], sizeof(NTempest::C3Vector), &zup, 0, 0, 0, 0, 0, &mTexCoords[0], sizeof(NTempest::C2Vector), 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, mIndices.Count(), mIndices.Ptr());
  GxPrimUnlockVertexPtrs();
  GxRsPop();
  GxXformSetView(worldToView);
  GxXformPop(GxXform_World);
  GxXformPop(GxXform_Tex0);
}


void CLightning::SetTexture(HTEXTURE texture) {
  if (mTexture) {
    HandleClose(mTexture);
  }
  mTexture = (HTEXTURE)HandleDuplicate(texture);
}

CLightningManager::~CLightningManager() {
  UINT count = mLiveBolts.Count();

  while (count) {
    CLightning *lightning = (CLightning *)((ulong)mLiveBolts[--count] & ~NOTUSEDFLAG);
    DELIFUSED(lightning);
  }
}

CLightningManager::CLightningManager() {
}

BoltID CLightningManager::Add(
    const NTempest::C3Vector &source,
    const NTempest::C3Vector &dest,
    float                     avgSegLen,
    float                     width,
    NTempest::CImVector       color,
    float                     noiseScale,
    float                     texCoordScale,
    float                     duration,
    HTEXTURE                  texture,
    void (*updateproc)(LPVOID, UINT, NTempest::C3Vector *, NTempest::C3Vector *),
    LPVOID context
) {
  BoltID boltId;

  if (!mDeadBolts.Count()) {
    boltId = mLiveBolts.Count();
    mLiveBolts.New();
    mLiveBolts[boltId] = NEW(CLightning);
  } else {
    boltId = *mDeadBolts.Top();
    mLiveBolts[boltId] = (CLightning *)((ulong)mLiveBolts[boltId] & ~NOTUSEDFLAG);
    mDeadBolts.SetCount(mDeadBolts.Count() - 1);
  }

  mLiveBolts[boltId]->SetSrcPos(source);
  mLiveBolts[boltId]->SetDstPos(dest);
  mLiveBolts[boltId]->SetAvgSegLen(avgSegLen);
  mLiveBolts[boltId]->SetWidth(width);
  mLiveBolts[boltId]->SetColor(color);
  mLiveBolts[boltId]->SetNoiseScale(noiseScale);
  mLiveBolts[boltId]->SetTexCoordScale(texCoordScale);
  mLiveBolts[boltId]->SetDuration(duration);
  mLiveBolts[boltId]->SetTexture(texture);
  SetCoordUpdate(boltId, updateproc, context);
  return boltId;
}


void CLightningManager::Update(float elapsed) {
  UINT count = mLiveBolts.Count();

  while (count) {
    --count;
    if (!((ulong)mLiveBolts[count] & NOTUSEDFLAG)) {
      mLiveBolts[count]->Update(elapsed);
    }
  }
}

void CLightningManager::Move(BoltID boltId, NTempest::C3Vector *src, NTempest::C3Vector *dst) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));
  VALIDATEBEGIN;
  VALIDATE(src || dst);
  VALIDATEENDVOID;

  if (src) {
    mLiveBolts[boltId]->SetSrcPos(*src);
  }
  if (dst) {
    mLiveBolts[boltId]->SetDstPos(*dst);
  }
}

void CLightningManager::SetCoordUpdate(BoltID boltId, void (*updateproc)(LPVOID, UINT, NTempest::C3Vector *, NTempest::C3Vector *), LPVOID context) {
  LightningCoordUpdateData updateData = {updateproc, context};

  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));
  mLiveBolts[boltId]->SetCoordUpdateData(updateData);
}

void CLightningManager::SetColor(BoltID boltId, NTempest::CImVector color) {
  ASSERT(BADBOLT != boltId);
  ASSERT(boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));

  mLiveBolts[boltId]->SetColor(color);
}

void CLightningManager::GetColor(BoltID boltId, NTempest::CImVector &color) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));

  mLiveBolts[boltId]->GetColor(color);
}

float CLightningManager::GetDuration(BoltID boltId) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));

  float duration;
  mLiveBolts[boltId]->GetDuration(duration);
  return duration;
}

void CLightningManager::Render(const NTempest::C3Vector &cameraPos) {
  UINT count = mLiveBolts.Count();

  while (count) {
    --count;
    if (!((ulong)mLiveBolts[count] & NOTUSEDFLAG)) {
      mLiveBolts[count]->Render(count, cameraPos);
    }
  }
}

void CLightningManager::Remove(BoltID boltId) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));

  mLiveBolts[boltId] = (CLightning *)((ulong)mLiveBolts[boltId] | NOTUSEDFLAG);
  *mDeadBolts.New() = boltId;
}
