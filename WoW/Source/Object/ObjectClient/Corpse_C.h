#pragma once

#include "Object/ObjectClient/Object_C.h"

struct CORPSEANIMDATA;
struct HCHARGEOSET__;
struct HTEXCOMPONENT__;

struct CGCorpseData {
  DWORDLONG          m_owner;
  float              m_facing;
  NTempest::C3Vector m_position;
  UINT               m_displayID;
  UINT               m_items[19];
  BYTE               m_unused;
  BYTE               m_raceID;
  BYTE               m_sex;
  BYTE               m_skinID;
  BYTE               m_faceID;
  BYTE               m_hairStyleID;
  BYTE               m_hairColorID;
  BYTE               m_facialHairStyleID;
  UINT               m_guildID;
  UINT               m_level;
};

class CGCorpse {
 public:
  static __forceinline UINT GetDataSize() {
    return sizeof(CGCorpseData);
  }
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 36;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  BYTE *GetData(UINT index);
  void  SetStorage(DWORD *storage) {
    m_corpse = reinterpret_cast<CGCorpseData *>(storage);
  }

  DWORDLONG GetOwner() const {
    return m_corpse->m_owner;
  }
  UINT               GetDisplayID() const;
  UINT               GetItemDisplayID(UINT index) const;
  UINT               GetItemInventoryType(UINT index) const;
  BYTE               GetRaceID() const;
  BYTE               GetSex() const;
  BYTE               GetSkinID() const;
  BYTE               GetFaceID() const;
  BYTE               GetHairStyleID() const;
  BYTE               GetHairColorID() const;
  BYTE               GetFacialHairStyleID() const;
  void               GetCorpsePosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetCorpsePosition() const;
  float              GetCorpseFacing() const;

 protected:
  explicit CGCorpse(DWORD *storage) {
    SetStorage(storage);
  }

  __forceinline ~CGCorpse() {
  }

  CGCorpseData *m_corpse;
};

class CGCorpse_C : public CGObject_C, public CGCorpse {
 public:
  CGCorpse_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGCorpse_C();

  void SetStorage(DWORD *storage);
  void PostInit(const CClientObjCreate &init);
  void PostMovementUpdate() {
  }
  virtual void   Disable(int shutdown);
  virtual void   Reenable();
  BOOL           SetBlock(UINT i, DWORD data);
  void           SetData(LPCVOID data, UINT bytes);
  static UINT    OffsetOf(OBJECT_TYPE_ID type);
  virtual LPCSTR GetModelFileName() const;
  virtual BOOL   ShouldRender(DWORD worldStatus);
  virtual void   GetPosition(NTempest::C3Vector &vec) const {
    vec = m_corpse->m_position;
  }
  virtual NTempest::C3Vector GetPosition() const {
    return m_corpse->m_position;
  }
  virtual float GetFacing() const {
    return m_corpse->m_facing;
  }
  virtual void GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  virtual BOOL CanHighlight() const {
    return 1;
  }
  virtual void OnLeftClick();
  virtual void OnRightClick();
  void         AddComponents();
  void         AddComponent(int displayID, UINT inventoryType, int slot, int commit);
  bool         IsUnderWater() const;
  void         OnDeathAnimEnd();
  void         CommitTexture(int force);

 private:
  CGCorpse_C &operator=(const CGCorpse_C &);

  void InitComponents();
  void InitPreferredGeosets();

  HCHARGEOSET__   *m_geosetHandle;
  HTEXCOMPONENT__ *m_texComponent;
  UINT             m_preferredGeosets[15];
  CORPSEANIMDATA  *m_animData;
};
