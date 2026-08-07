#pragma once

#include "Object/ObjectClient/Object_C.h"

struct BlizzardObject;
struct Sound;
class SpellVisualEffectNameRec;

enum DYNAMIC_OBJECT_TYPE {
  DYNAMIC_OBJECT_PORTAL = 0,
  DYNAMIC_OBJECT_AREA_SPELL = 1,
  DYNAMIC_OBJECT_FARSIGHT_FOCUS = 2
};

struct CGDynamicObjectData {
  DWORDLONG          m_caster;
  BYTE               m_type;
  BYTE               m_typeFlags;
  BYTE               m_padding[2];
  int                m_spellID;
  float              m_radius;
  NTempest::C3Vector m_position;
  float              m_facing;
  int                m_morePadding;

  CGDynamicObjectData();
};

class CGDynamicObject {
 public:
  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 16;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  BYTE               *GetData(UINT index);
  DYNAMIC_OBJECT_TYPE GetDynamicType();
  void                SetStorage(DWORD *storage) {
    m_dynamicObj = reinterpret_cast<CGDynamicObjectData *>(storage);
  }

  int                GetSpellID() const;
  float              GetRadius() const;
  void               GetObjectPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetObjectPosition() const;
  float              GetObjectFacing() const;
  DWORDLONG          GetCaster() const;

 protected:
  explicit CGDynamicObject(DWORD *storage) {
    SetStorage(storage);
  }

  __forceinline ~CGDynamicObject() {
  }

  CGDynamicObjectData *DynamicObject() {
    return m_dynamicObj;
  }

  const CGDynamicObjectData *DynamicObject() const {
    return m_dynamicObj;
  }

  CGDynamicObjectData *m_dynamicObj;
};

class CGDynamicObject_C : public CGObject_C, public CGDynamicObject {
 public:
  CGDynamicObject_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGDynamicObject_C();

  void SetStorage(DWORD *storage);
  void PostInit(const CClientObjCreate &init);
  void PostMovementUpdate() {
  }
  virtual void                    Disable(int shutdown);
  virtual void                    Reenable();
  BOOL                            SetBlock(UINT i, DWORD data);
  void                            SetData(LPCVOID data, UINT bytes);
  static UINT                     OffsetOf(OBJECT_TYPE_ID type);
  const SpellVisualEffectNameRec *GetVisualEffectNameRec() const;
  virtual LPCSTR                  GetModelFileName() const;
  void                            HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &position);
  void                            AnimFinished();
  virtual void                    GetPosition(NTempest::C3Vector &vec) const {
    vec = m_dynamicObj->m_position;
  }
  virtual NTempest::C3Vector GetPosition() const {
    return m_dynamicObj->m_position;
  }
  virtual float GetFacing() const {
    return m_dynamicObj->m_facing;
  }
  virtual float GetScale() const {
    return m_dynamicScale;
  }
  virtual BOOL UpdateModelLoadStatus();
  void         UpdateDisplay(DWORD displayID);
  void         ObjectVisKitProc();
  void         ClearSound();

 private:
  CGDynamicObject_C &operator=(const CGDynamicObject_C &);

  int             m_haveStandSequence : 1;
  int             m_haveHoldSequence : 1;
  float           m_dynamicScale;
  BlizzardObject *m_blizzardObject;
  Sound          *m_sound;
};
