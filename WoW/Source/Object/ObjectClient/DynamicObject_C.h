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
  unsigned __int64   m_caster;
  unsigned char      m_type;
  unsigned char      m_typeFlags;
  unsigned char      m_padding[2];
  int                m_spellID;
  float              m_radius;
  NTempest::C3Vector m_position;
  float              m_facing;
  int                m_morePadding;

  CGDynamicObjectData();
};

class CGDynamicObject {
 public:
  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static unsigned int TotalFields();
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();

  unsigned char *GetData(unsigned int index);
  DYNAMIC_OBJECT_TYPE GetDynamicType();
  void SetStorage(unsigned long *storage) {
    m_dynamicObj = reinterpret_cast<CGDynamicObjectData *>(storage);
  }

  int GetSpellID() const;
  float GetRadius() const;
  void GetObjectPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetObjectPosition() const;
  float GetObjectFacing() const;
  unsigned __int64 GetCaster() const;

 protected:
  explicit CGDynamicObject(unsigned long *storage) {
    SetStorage(storage);
  }

  ~CGDynamicObject() {
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
  CGDynamicObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGDynamicObject_C();

  void         SetStorage(unsigned long *storage);
  void         PostInit(const CClientObjCreate &init);
  void PostMovementUpdate() {
  }
  virtual void                   Disable(int shutdown);
  virtual void                   Reenable();
  int                            SetBlock(unsigned int i, unsigned long data);
  void                           SetData(const void *data, unsigned int bytes);
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);
  const SpellVisualEffectNameRec *GetVisualEffectNameRec() const;
  virtual const char            *GetModelFileName() const;
  void                           HandleAnimEvent(const char *eventName, const NTempest::C3Vector &position);
  void                           AnimFinished();
  virtual void                   GetPosition(NTempest::C3Vector &vec) const {
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
  virtual int UpdateModelLoadStatus();
  void        UpdateDisplay(unsigned long displayID);
  void        ObjectVisKitProc();
  void        ClearSound();

 private:
  CGDynamicObject_C &operator=(const CGDynamicObject_C &);

  int             m_haveStandSequence : 1;
  int             m_haveHoldSequence : 1;
  float           m_dynamicScale;
  BlizzardObject *m_blizzardObject;
  Sound          *m_sound;
};
