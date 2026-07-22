#pragma once

#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

typedef unsigned long ulong;
typedef unsigned int  BoltID;

const BoltID BADBOLT = static_cast<BoltID>(-1);
const ulong  NOTUSEDFLAG = 0x80000000;

struct LightningCoordUpdateData {
  void(__fastcall *callback)(void *context, unsigned int time, NTempest::C3Vector *source, NTempest::C3Vector *destination);
  void *context;
};

class CLightning {
 public:
  CLightning();
  ~CLightning();
  void Update(float elapsed);
  void Render(unsigned int boltId, const NTempest::C3Vector &cameraPos);
  void SetTexture(HTEXTURE texture);

 private:
  void BuildStroke(TSFixedArray<NTempest::C3Vector> &points);

  friend class CLightningManager;

  NTempest::C3Vector                            mSrcPos;
  NTempest::C3Vector                            mDstPos;
  float                                         mAvgSegLen;
  float                                         mWidth;
  NTempest::CImVector                           mColor;
  float                                         mNoiseScale;
  float                                         mTexCoordScale;
  float                                         mDuration;
  int                                           mRebuildPoints;
  TSFixedArray_<NTempest::C3Vector, 'Ligh', 38> mPoints;
  TSFixedArray_<NTempest::C3Vector, 'Ligh', 39> mPos;
  TSFixedArray_<NTempest::C2Vector, 'Ligh', 40> mTexCoords;
  TSFixedArray_<unsigned short, 'Ligh', 41>     mIndices;
  float                                         mAccTime;
  HTEXTURE                                      mTexture;
  LightningCoordUpdateData                      mCoordUpdateData;
};

class CLightningManager {
 public:
  CLightningManager();
  ~CLightningManager();

  BoltID Add(
      NTempest::C3Vector &source,
      NTempest::C3Vector &dest,
      float               avgSegLen,
      float               width,
      NTempest::CImVector color,
      float               noiseScale,
      float               texCoordScale,
      float               duration,
      HTEXTURE            texture,
      void(__fastcall *updateproc)(void *, unsigned int, NTempest::C3Vector *, NTempest::C3Vector *),
      void *context
  );
  void Move(BoltID boltId, NTempest::C3Vector *src, NTempest::C3Vector *dst);
  void Update(float elapsed);
  void Render(const NTempest::C3Vector &cameraPos);
  void Remove(BoltID boltId);

 private:
  TSGrowableArray<CLightning *> mLiveBolts;
  TSGrowableArray<int>          mDeadBolts;
};
