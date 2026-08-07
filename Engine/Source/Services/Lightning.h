#pragma once

#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

typedef DWORD ulong;
typedef UINT  BoltID;

const BoltID BADBOLT = static_cast<BoltID>(-1);
const ulong  NOTUSEDFLAG = 0x80000000;

struct LightningCoordUpdateData {
  void (*callback)(LPVOID context, UINT time, NTempest::C3Vector *source, NTempest::C3Vector *destination);
  LPVOID context;
};

class CLightning {
 public:
  CLightning();
  ~CLightning();
  void Update(float elapsed);
  void Render(UINT boltId, const NTempest::C3Vector &cameraPos);
  void SetTexture(HTEXTURE texture);
  void SetSrcPos(const NTempest::C3Vector &position) {
    mSrcPos = position;
    mRebuildPoints = 1;
  }
  void SetDstPos(const NTempest::C3Vector &position) {
    mDstPos = position;
    mRebuildPoints = 1;
  }
  void SetAvgSegLen(float length) {
    mAvgSegLen = length;
    mRebuildPoints = 1;
  }
  void SetWidth(float width) {
    mWidth = width;
  }
  void SetColor(NTempest::CImVector color) {
    mColor = color;
  }
  void SetNoiseScale(float scale) {
    mNoiseScale = scale;
  }
  void SetTexCoordScale(float scale) {
    mTexCoordScale = scale;
  }
  void SetCoordUpdateData(LightningCoordUpdateData &data) {
    mCoordUpdateData = data;
  }
  void SetDuration(float duration) {
    mDuration = duration;
  }
  void GetSrcPos(NTempest::C3Vector &position) {
    position = mSrcPos;
  }
  void GetDstPos(NTempest::C3Vector &position) {
    position = mDstPos;
  }
  void GetAvgSegLen(float &length) {
    length = mAvgSegLen;
  }
  void GetWidth(float &width) {
    width = mWidth;
  }
  void GetColor(NTempest::CImVector &color) {
    color = mColor;
  }
  void GetNoiseScale(float &scale) {
    scale = mNoiseScale;
  }
  void GetTexCoordScale(float &scale) {
    scale = mTexCoordScale;
  }
  void GetTexture(HTEXTURE &texture) {
    texture = mTexture;
  }
  void GetCoordUpdateData(LightningCoordUpdateData &data) {
    data = mCoordUpdateData;
  }
  void GetDuration(float &duration) {
    duration = mDuration;
  }

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
  BOOL                                          mRebuildPoints;
  TSFixedArray_<NTempest::C3Vector, 'Ligh', __LINE__> mPoints;
  TSFixedArray_<NTempest::C3Vector, 'Ligh', __LINE__> mPos;
  TSFixedArray_<NTempest::C2Vector, 'Ligh', __LINE__> mTexCoords;
  TSFixedArray_<WORD, 'Ligh', __LINE__>               mIndices;
  float                                         mAccTime;
  HTEXTURE                                      mTexture;
  LightningCoordUpdateData                      mCoordUpdateData;
};

class CLightningManager {
 public:
  ~CLightningManager();

  BoltID Add(
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
  );
  void  Move(BoltID boltId, NTempest::C3Vector *src, NTempest::C3Vector *dst);
  void  SetCoordUpdate(BoltID boltId, void (*updateproc)(LPVOID, UINT, NTempest::C3Vector *, NTempest::C3Vector *), LPVOID context);
  void  SetColor(BoltID boltId, NTempest::CImVector color);
  void  GetColor(BoltID boltId, NTempest::CImVector &color);
  float GetDuration(BoltID boltId);
  void  Update(float elapsed);
  void  Render(const NTempest::C3Vector &cameraPos);
  void  Remove(BoltID boltId);

 private:
  friend void SpellVisualsInitialize();

  CLightningManager();
  CLightningManager(const CLightningManager &);
  CLightningManager &operator=(const CLightningManager &);

  TSGrowableArray<CLightning *> mLiveBolts;
  TSGrowableArray<int>          mDeadBolts;
};
