#pragma once

#include "Gx.h"

class CGxDevice;

class CGxStateBom {
 private:
  friend class CGxDevice;

  int mData[3];
  int filler;

 public:
  int                 operator!=(const CGxStateBom &value);

  CGxStateBom operator~() {
    CGxStateBom tmp_;

    tmp_.mData[0] = ~mData[0];
    tmp_.mData[1] = ~mData[1];
    tmp_.mData[2] = ~mData[2];
    return tmp_;
  }


  int                 GetAsInt();

  float GetAsFloat() {
    return *reinterpret_cast<float *>(&mData[0]);
  }

  NTempest::CImVector GetAsCArgb();

  NTempest::C3Vector GetAsC3Vector() {
    return *reinterpret_cast<NTempest::C3Vector *>(&mData[0]);
  }

  LPVOID GetAsPointer();

  const CGxStateBom &operator=(int value) {
    mData[0] = value;
    mData[1] = value;
    mData[2] = value;
    return *this;
  }

  const CGxStateBom &operator=(NTempest::CImVector value) {
    mData[0] = *value.IV_();
    mData[1] = *value.IV_();
    mData[2] = *value.IV_();
    return *this;
  }

  const CGxStateBom &operator=(float value) {
    mData[0] = *reinterpret_cast<int *>(&value);
    mData[1] = *reinterpret_cast<int *>(&value);
    mData[2] = *reinterpret_cast<int *>(&value);
    return *this;
  }
};

struct CGxPushedRenderState {
  EGxRenderState mWhich;
  CGxStateBom    mValue;
  DWORD          mStackDepth;
};

struct CGxAppRenderState {
  CGxStateBom mValue;
  DWORD       mStackDepth;
  int         mDirty;

  CGxAppRenderState() {
    mValue = 0;
    mStackDepth = 0;
    mDirty = 0;
  }
};
