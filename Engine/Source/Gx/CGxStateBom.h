#pragma once

#include "Gx.h"

class CGxDevice;

class CGxStateBom {
 public:
  int                 operator!=(const CGxStateBom &value);
  CGxStateBom         operator~();
  int                 GetAsInt();
  float               GetAsFloat();
  NTempest::CImVector GetAsCArgb();
  NTempest::C3Vector  GetAsC3Vector();
  void               *GetAsPointer();

  const CGxStateBom &operator=(int value) {
    mData[0] = value;
    mData[1] = value;
    mData[2] = value;
    return *this;
  }

  const CGxStateBom &operator=(float value) {
    *reinterpret_cast<float *>(&mData[0]) = value;
    *reinterpret_cast<float *>(&mData[1]) = value;
    *reinterpret_cast<float *>(&mData[2]) = value;
    return *this;
  }

 private:
  friend class CGxDevice;

  int mData[3];
  int filler;
};

struct CGxPushedRenderState {
  EGxRenderState mWhich;
  CGxStateBom    mValue;
  unsigned int   mStackDepth;
};

struct CGxAppRenderState {
  CGxAppRenderState() {
    mValue = 0;
    mStackDepth = 0;
    mDirty = 0;
  }

  CGxStateBom  mValue;
  unsigned int mStackDepth;
  int          mDirty;
};
