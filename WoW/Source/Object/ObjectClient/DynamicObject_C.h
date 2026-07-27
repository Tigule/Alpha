#pragma once

#include "Object/ObjectClient/Object_C.h"

struct BlizzardObject;
struct Sound;
class SpellVisualEffectNameRec;

struct CGDynamicObjectData {
  unsigned __int64   m_caster;
  unsigned int       m_type;
  int                m_spellID;
  float              m_radius;
  NTempest::C3Vector m_position;
  float              m_facing;
  unsigned int       m_padding;
};

class CGDynamicObject {
 public:
  CGDynamicObject(unsigned long *storage) : m_dynamicObj(reinterpret_cast<CGDynamicObjectData *>(storage + 6)) {
  }

 protected:
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
  SpellVisualEffectNameRec      *GetVisualEffectNameRec();
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
  unsigned int    m_haveStandSequence : 1;
  unsigned int    m_haveHoldSequence : 1;
  float           m_dynamicScale;
  BlizzardObject *m_blizzardObject;
  Sound          *m_sound;
};
