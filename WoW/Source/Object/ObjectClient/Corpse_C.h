#pragma once

#include "Object/ObjectClient/Object_C.h"

struct CORPSEANIMDATA;
struct HCHARGEOSET__;
struct HTEXCOMPONENT__;

struct CGCorpseData {
  unsigned __int64   m_owner;
  float              m_facing;
  NTempest::C3Vector m_position;
  unsigned int       m_displayID;
  unsigned int       m_items[19];
  unsigned char      m_unused;
  unsigned char      m_raceID;
  unsigned char      m_sex;
  unsigned char      m_skinID;
  unsigned char      m_faceID;
  unsigned char      m_hairStyleID;
  unsigned char      m_hairColorID;
  unsigned char      m_facialHairStyleID;
  unsigned int       m_guildID;
  unsigned int       m_level;
};

class CGCorpse {
 public:
  CGCorpse(unsigned long *storage) : m_corpse(reinterpret_cast<CGCorpseData *>(storage + 6)) {
  }

 protected:
  CGCorpseData *m_corpse;
};

class CGCorpse_C : public CGObject_C, public CGCorpse {
 public:
  CGCorpse_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  virtual ~CGCorpse_C();

  void         SetStorage(unsigned long *storage);
  void         PostInit(const CClientObjCreate &init);
  virtual void PostMovementUpdate() {
  }
  virtual void                   Disable(int shutdown);
  virtual void                   Reenable();
  int                            SetBlock(unsigned int i, unsigned long data);
  void                           SetData(const void *data, unsigned int bytes);
  static unsigned int __fastcall OffsetOf(OBJECT_TYPE_ID type);
  virtual const char            *GetModelFileName() const;
  virtual int                    ShouldRender(unsigned long worldStatus);
  virtual void                   GetPosition(NTempest::C3Vector &vec) const {
    vec = m_corpse->m_position;
  }
  virtual NTempest::C3Vector GetPosition() const {
    return m_corpse->m_position;
  }
  virtual float GetFacing() const {
    return m_corpse->m_facing;
  }
  virtual void GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  virtual int  CanHighlight() const {
    return 1;
  }
  virtual void OnLeftClick();
  virtual void OnRightClick();
  void         AddComponents();
  void         AddComponent(int displayID, unsigned int inventoryType, int slot, int commit);
  unsigned int IsUnderWater();
  void         OnDeathAnimEnd();
  void         CommitTexture(int force);

 private:
  void InitComponents();
  void InitPreferredGeosets();

  HCHARGEOSET__   *m_geosetHandle;
  HTEXCOMPONENT__ *m_texComponent;
  unsigned int     m_preferredGeosets[15];
  CORPSEANIMDATA  *m_animData;
};
