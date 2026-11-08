#pragma once

#include "Object/ObjectClient/Unit_C.h"

class NODEBASE {
 public:
  NODEBASE() : model(0), flags(0), deathHoldTimer(0) {
  }
  ~NODEBASE();
  virtual void ReleaseDeathHolds() = 0;

  void ClearDeathHoldTimer();
  void SetDeathHoldTimer(UINT duration);
  bool CheckModelLoadStatus();

  LINKDECLEX(NODEBASE, node);
  HMODEL model;
  UINT      flags;
  UINT      deathHoldTimer;
};

class ONESHOTEFFECTNODE : public NODEBASE {
 public:
  ONESHOTEFFECTNODE() : objectModel(0) {
  }
  ~ONESHOTEFFECTNODE();
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();

  HMODEL objectModel;
  UINT      objectModelAttachmentPoint;
  DWORDLONG objectGUID;
  int       spellID;
  BYTE      isCastEffect;
};

class ONESHOTSTANDALONEEFFECTNODE : public NODEBASE {
 public:
  ONESHOTSTANDALONEEFFECTNODE() : facing(0.0f), scale(1.0f), worldObject(0) {
  }
  ~ONESHOTSTANDALONEEFFECTNODE();
  virtual void ReleaseDeathHolds();
  void         CheckModelLoadStatus();

  NTempest::C3Vector      position;
  TSFixedArray<DWORDLONG> objects;
  float                   facing;
  float                   scale;
  DWORD                   worldObject;
  int                     expireTime;
};

NODEDECL(MISSILENODE) {
  static const float HEIGHT_SCAN_RANGE;
  static const float MIN_HEIGHT;

  MISSILENODE() : model(0), caster(0), target(0), flags(0), sound(0) {
  }
  ~MISSILENODE() {
    if (model) {
      HandleClose(model);
    }
    if (sound) {
      sound->Stop(2.0f);
    }
    if (target) {
      CGObject_C *object = ClntObjMgrObjectPtr(target, __FILE__, __LINE__);
      if (object && object->IsA(TYPE_UNIT)) {
        ((CGUnit_C *)object)->DDDELLOG(caster, "MISSILENODE immaturely freed", __FILE__, __LINE__);
      }
    }
  }

  HMODEL             model;
  DWORDLONG          caster;
  NTempest::C3Vector startPosition;
  NTempest::C3Vector position;
  NTempest::C3Vector endPosition;
  NTempest::C3Vector normal;
  DWORDLONG          target;
  UINT               startTime;
  UINT               travelTime;
  NTempest::C3Vector facing;
  UINT               spellID;
  UINT               victimEffect;
  UINT               pathType;
  bool               miss;
  MISS_REASON        missReason;
  int                flags;
  void CheckModelLoadStatus();
  Sound             *sound;
};
