#include <Base/Base.h>

#include "Services/Lightning.h"

#include "Gx/Gx.h"
#include "Tempest/c44matrix.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

#include <math.h>

static NTempest::CRndSeed                            sRandSeed;
static TSFixedArray_<NTempest::C3Vector, 'Ligh', __LINE__> sPoints;
static NTempest::C44Matrix                           identity;
static NTempest::C44Matrix                           worldToView;
static NTempest::C44Matrix                           particleToView;
static NTempest::C3Vector                            zup;

CLightning::CLightning() : mAvgSegLen(-2.0f), mWidth(1.0f), mRebuildPoints(1), mAccTime(0.0f), mTexture(0) {
}

void CLightning::BuildStroke(TSFixedArray<NTempest::C3Vector> &points) {
  NTempest::C3Vector diff = mDstPos - mSrcPos;
  float              length = diff.Mag();
  UINT               numPoints = static_cast<UINT>(length / mAvgSegLen + 2.0f);
  float              ooNumPoints = 1.0f / numPoints;
  float              noiseScale = length * mNoiseScale;

  points.SetCount(numPoints + 1);
  points[0] = mSrcPos;
  points[numPoints] = mDstPos;

  for (UINT i = 1; i != numPoints; ++i) {
    NTempest::C3Vector tmp = mSrcPos + diff * (static_cast<float>(i) * ooNumPoints);
    tmp += NTempest::C3Vector(NTempest::CRandom::reals_(sRandSeed), NTempest::CRandom::reals_(sRandSeed), NTempest::CRandom::reals_(sRandSeed)) *
           noiseScale;
    points[i] = tmp;
  }
}

void CLightning::Update(float elapsed) {
  if (mTexCoordScale == 0.0f) {
    mAccTime = 0.0f;
  } else {
    mAccTime = fmod(mAccTime + elapsed, mTexCoordScale);
  }

  if (mRebuildPoints) {
    BuildStroke(mPoints);

    UINT  numPos = 2 * mPoints.Count();
    UINT  end = numPos - 2;
    float ooNumPos = 1.0f / end;

    mPos.SetCount(numPos);
    mTexCoords.SetCount(numPos);
    mIndices.SetCount(numPos);

    for (UINT i = 0; i < numPos; i += 2) {
      float x = static_cast<float>(i) * ooNumPos;
      mTexCoords[i] = NTempest::C2Vector(x, 0.0f);
      mTexCoords[i + 1] = NTempest::C2Vector(x, 1.0f);
      mIndices[i] = static_cast<WORD>(i);
      mIndices[i + 1] = static_cast<WORD>(i + 1);
    }

    mTexCoords[1] = NTempest::C2Vector(0.0f, 0.5f);
    mTexCoords[0] = mTexCoords[1];
    mTexCoords[numPos - 1] = NTempest::C2Vector(1.0f, 0.5f);
    mTexCoords[numPos - 2] = mTexCoords[numPos - 1];
    mRebuildPoints = 0;
  }

  BuildStroke(sPoints);

  UINT end = mPoints.Count() - 1;
  for (UINT i = 1; i < end; ++i) {
    mPoints[i] = sPoints[i] * 0.25f + mPoints[i] * 0.75f;
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

  GxXformView(worldToView);
  GxXformSetView(identity);
  GxXformPush(GxXform_World);
  GxXformIdentity(GxXform_World);
  GxXformPush(GxXform_Tex0);

  NTempest::C44Matrix translate;
  translate.Translate(-cameraPos);
  particleToView = translate * worldToView;

  UINT numPoints = mPoints.Count();
  mPos[0] *= 0.0f;
  mPos[1] *= 0.0f;

  NTempest::C3Vector p = mPoints[0] * particleToView;
  UINT               end = 2 * numPoints - 2;
  for (UINT i = 2; i < end; i += 2) {
    NTempest::C3Vector q = mPoints[i / 2] * particleToView;
    NTempest::C3Vector d = q - p;
    NTempest::C3Vector perp(-d.y, d.x, 0.0f);
    float              mag = perp.Mag();
    if (mag > 0.001f) {
      perp *= 1.0f / mag;
    }
    perp *= mWidth;

    mPos[i - 2] += (p + perp) * 0.5f;
    mPos[i - 1] += (p - perp) * 0.5f;
    mPos[i] = (q + perp) * 0.5f;
    mPos[i + 1] = (q - perp) * 0.5f;
    p = q;
  }

  mPos[1] = mPoints[0] * particleToView;
  mPos[0] = mPos[1];
  mPos[2 * numPoints - 1] = mPoints[numPoints - 1] * particleToView;
  mPos[2 * numPoints - 2] = mPos[2 * numPoints - 1];

  NTempest::C3Vector texTranslate(-mAccTime / (mDuration == 0.0f ? 1.0f : mDuration), 0.0f, 0.0f);
  GxXformTranslate(GxXform_Tex0, texTranslate);

  GxRsPush();
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(mColor.a, 0, 0, 0));
  GxRsSet(GxRs_MatEmissive, mColor);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_Blend, GxBlend_Add);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(mTexture, 1, 0));
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxPrimLockVertexPtrs(mPos.Count(), &mPos[0], sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, &mTexCoords[0], sizeof(NTempest::C2Vector), 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, mIndices.Count(), &mIndices[0]);
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
  mTexture = static_cast<HTEXTURE>(HandleDuplicate(texture));
}

CLightningManager::~CLightningManager() {
  UINT count = mLiveBolts.Count();

  while (count) {
    CLightning *lightning = reinterpret_cast<CLightning *>(reinterpret_cast<ulong>(mLiveBolts[--count]) & ~NOTUSEDFLAG);
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
  BoltID      boltId;
  CLightning *lightning;

  if (mDeadBolts.Count()) {
    boltId = mDeadBolts[mDeadBolts.Count() - 1];
    mDeadBolts.SetCount(mDeadBolts.Count() - 1);
    lightning = reinterpret_cast<CLightning *>(reinterpret_cast<ulong>(mLiveBolts[boltId]) & ~NOTUSEDFLAG);
    mLiveBolts[boltId] = lightning;
  } else {
    boltId = mLiveBolts.Count();
    lightning = new CLightning;
    *mLiveBolts.New() = lightning;
  }

  lightning->SetSrcPos(source);
  lightning->SetDstPos(dest);
  lightning->SetAvgSegLen(avgSegLen);
  lightning->SetWidth(width);
  lightning->SetColor(color);
  lightning->SetNoiseScale(noiseScale);
  lightning->SetTexCoordScale(texCoordScale);
  lightning->SetDuration(duration);
  lightning->SetTexture(texture);
  SetCoordUpdate(boltId, updateproc, context);
  return boltId;
}

void CLightningManager::Update(float elapsed) {
  UINT count = mLiveBolts.Count();

  while (count) {
    --count;
    if (!(reinterpret_cast<ulong>(mLiveBolts[count]) & NOTUSEDFLAG)) {
      mLiveBolts[count]->Update(elapsed);
    }
  }
}

void CLightningManager::Move(BoltID boltId, NTempest::C3Vector *src, NTempest::C3Vector *dst) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));
  FATALASSERT(src || dst);

  if (src) {
    mLiveBolts[boltId]->SetSrcPos(*src);
  }
  if (dst) {
    mLiveBolts[boltId]->SetDstPos(*dst);
  }
}

void CLightningManager::SetCoordUpdate(BoltID boltId, void (*updateproc)(LPVOID, UINT, NTempest::C3Vector *, NTempest::C3Vector *), LPVOID context) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & reinterpret_cast<ulong>(mLiveBolts[boltId])));

  LightningCoordUpdateData updateData = {updateproc, context};
  mLiveBolts[boltId]->SetCoordUpdateData(updateData);
}

void CLightningManager::SetColor(BoltID boltId, NTempest::CImVector color) {
  ASSERT(BADBOLT != boltId);
  ASSERT(boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & reinterpret_cast<ulong>(mLiveBolts[boltId])));

  mLiveBolts[boltId]->SetColor(color);
}

void CLightningManager::GetColor(BoltID boltId, NTempest::CImVector &color) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & reinterpret_cast<ulong>(mLiveBolts[boltId])));

  mLiveBolts[boltId]->GetColor(color);
}

float CLightningManager::GetDuration(BoltID boltId) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & reinterpret_cast<ulong>(mLiveBolts[boltId])));

  float duration;
  mLiveBolts[boltId]->GetDuration(duration);
  return duration;
}

void CLightningManager::Render(const NTempest::C3Vector &cameraPos) {
  UINT count = mLiveBolts.Count();

  while (count) {
    --count;
    if (!(reinterpret_cast<ulong>(mLiveBolts[count]) & NOTUSEDFLAG)) {
      mLiveBolts[count]->Render(count, cameraPos);
    }
  }
}

void CLightningManager::Remove(BoltID boltId) {
  ASSERT(BADBOLT != boltId && boltId < mLiveBolts.Count());
  ASSERT(0 == (NOTUSEDFLAG & (ulong)mLiveBolts[boltId]));

  mLiveBolts[boltId] = reinterpret_cast<CLightning *>(reinterpret_cast<ulong>(mLiveBolts[boltId]) | NOTUSEDFLAG);
  *mDeadBolts.New() = boltId;
}
