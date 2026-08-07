#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H

#include "Object_C.h"
#include "Unit_C.h"

#include <stpl.h>

#include "Tempest/c34matrix.h"
#include "Tempest/c3spline.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/caabox.h"
#include "Ui/GameUI.h"
#include "Object/MovementData.h"

class CGGameObject_C;
class CGItem_C;
class CGUnit_C;
class LockRec;
class Sound;
class TransportAnimationRec;
class GameObjectStats;
struct HCOLLISIONDATA__;
struct HMODEL__;
struct WorldObjCollisionHandlerData;

BOOL ObjectCollisionProc(DWORDLONG param64, DWORD param32, WorldObjCollisionHandlerData *data);

struct CGGameObjectData {
  int                    m_displayID;
  UINT                   m_flags;
  NTempest::C4Quaternion m_rotation;
  int                    m_state;
  UINT                   m_timestamp;
  NTempest::C3Vector     m_position;
  float                  m_facing;
  UINT                   m_dynamicFlags;
  int                    m_factionTemplate;
};

class CGGameObject {
  friend class CGGameObject_C_TypeBase;

 public:
  enum {
    STATE_OPENING,
    STATE_CLOSING,
    STATE_DESTROYED,
    NUM_STATES
  };

  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 20;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  BYTE *GetData(UINT index);
  void  SetStorage(DWORD *storage) {
    m_gameObj = reinterpret_cast<CGGameObjectData *>(storage);
  }

  CGGameObjectData *GameObject() {
    return m_gameObj;
  }

  const CGGameObjectData *GameObject() const {
    return m_gameObj;
  }

  int                           GetDisplayID() const;
  const NTempest::C4Quaternion &GetRotation() const;
  int                           GetState() const {
    return m_gameObj->m_state;
  }
  UINT               GetTimeStamp() const;
  UINT               GetGameObjectFlags() const;
  void               GetObjectPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetObjectPosition() const;
  float              GetObjectFacing() const;
  int                GetFactionTemplate() const;
  bool               GetDisabled() const;
  bool               GetLocked() const;
  bool               GetQuestOnly() const;

 protected:
  explicit CGGameObject(DWORD *storage) {
    SetStorage(storage);
  }

  __forceinline ~CGGameObject() {
  }

  CGGameObjectData *m_gameObj;
};

class CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_TypeBase() : m_owner(0), m_interactDistance(5.0f) {
  }
  CGGameObject_C_TypeBase(CGGameObject_C *owner);

  CGGameObject_C *m_owner;

  virtual __forceinline ~CGGameObject_C_TypeBase() {
  }
  virtual bool               CanHighlight() const;
  virtual bool               CanChangeCursor() const;
  virtual bool               CanUse() const;
  virtual bool               CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual bool               Use(const DWORDLONG &activator);
  virtual void               UpdateState(int oldState, int newState);
  virtual void               HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &position);
  virtual void               HandleAnimFinished();
  virtual LPCSTR             DebugStatus();
  virtual void               ActivateCustomAnim(UINT anim);
  virtual NTempest::C3Vector GetPosition() const;
  virtual float              GetFacing() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual NTempest::C3Vector GetCurrentMoveVector() const;
  virtual BOOL               IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               PostInit();
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               PostReenable();
  virtual void               UpdateMovement(DWORD eventTime, float elapsed);
  virtual void               ModelJustLoaded();
  virtual void               StartInteraction();
  virtual void               CloseInteraction();

 protected:
  float m_interactDistance;
};

class CGGameObject_C_Type_Null : public CGGameObject_C_TypeBase {
 public:
  virtual bool   CanUse() const;
  virtual bool   CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual LPCSTR DebugStatus();
};

class CGGameObject_C_TypeAnimated : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_TypeAnimated(CGGameObject_C *owner);
  virtual ~CGGameObject_C_TypeAnimated();
  virtual void   UpdateState(int oldState, int newState);
  virtual void   HandleAnimEvent(LPCSTR eventName, const NTempest::C3Vector &position);
  virtual void   HandleAnimFinished();
  virtual LPCSTR DebugStatus();
  virtual void   ActivateCustomAnim(UINT anim);
  virtual void   UpdateAnimState(UINT newState);
  virtual void   PostInit();
  virtual void   Disable(int shutdown);
  virtual void   ModelJustLoaded();

 protected:
  void PlayAnimatedSound(int index, const NTempest::C3Vector &position);
  void CloseLoopingSound();
  void SetSequence();

  UINT   m_animState;
  BYTE   m_useFallbackAnim[11];
  Sound *m_loopingSound;
  UINT   m_animPresent;
};

class CGGameObject_C_Type_Door : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Door(CGGameObject_C *owner);
  virtual bool CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual void UpdateAnimState(UINT newState);
  bool         IsAtRest() const;
  UINT         GetStartOpen() const;
  UINT         GetAutoClose() const;
};

class CGGameObject_C_Type_Button : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Button(CGGameObject_C *owner);
};

class CGGameObject_C_Type_Chest : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Chest(CGGameObject_C *owner);
};

class CGGameObject_C_Type_Trap : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Trap(CGGameObject_C *owner);
};

class CGGameObject_C_Type_AreaDamage : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_AreaDamage(CGGameObject_C *owner);
  virtual void ModelJustLoaded();
};

class CGGameObject_C_Type_QuestGiver : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_QuestGiver(CGGameObject_C *owner);
  virtual void StartInteraction();
  virtual void CloseInteraction();
};

class CGGameObject_C_Type_Binder : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_Binder(CGGameObject_C *owner);
};

class CGGameObject_C_Type_Generic : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_Generic(CGGameObject_C *owner);
  virtual bool CanHighlight() const;
  virtual bool CanUse() const;
};

class CGGameObject_C_Type_MapObj : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_MapObj(CGGameObject_C *owner);
  virtual ~CGGameObject_C_Type_MapObj();
  virtual bool CanHighlight() const;
  virtual bool CanUse() const;
  virtual void PostInit();

 protected:
  UINT m_objectId;
};

class CGGameObject_C_Type_MapObjTransport : public CGGameObject_C_Type_MapObj {
 public:
  CGGameObject_C_Type_MapObjTransport(CGGameObject_C *owner);
  virtual ~CGGameObject_C_Type_MapObjTransport();
  virtual NTempest::C3Vector GetPosition() const;
  virtual float              GetFacing() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual BOOL               IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               UpdateMovement(DWORD eventTime, float elapsed);

 protected:
  LISTDECLEX(CMovementData, transportLink, m_passengers);
  NTempest::C3Spline_CatmullRom m_path[2];
  UINT                          m_tripTime[2];
  NTempest::C3Vector            m_position;
  float                         m_facing;
};

class CGGameObject_C_Type_Chair : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_Chair(CGGameObject_C *owner);
  virtual bool CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual void PostInit();
  UINT         GetNumSlots() const;
  UINT         GetHeight() const;

 protected:
  NTempest::C3Vector m_slotPositions[5];
};

class CGGameObject_C_Type_SpellFocus : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_SpellFocus(CGGameObject_C *owner);
  virtual bool CanHighlight() const;
  virtual bool CanUse() const;
};

class CGGameObject_C_Type_Text : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Text(CGGameObject_C *owner);
  virtual bool Use(const DWORDLONG &activator);
  virtual void PostInit();
  virtual void StartInteraction();
  virtual void CloseInteraction();
};

class CGGameObject_C_Type_Goober : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Goober(CGGameObject_C *owner);
};

class CGGameObject_C_Type_Transport : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Transport(CGGameObject_C *owner);
  virtual NTempest::C3Vector GetPosition() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual NTempest::C3Vector GetCurrentMoveVector() const;
  virtual bool               CanUse() const;
  virtual BOOL               IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               UpdateMovement(DWORD eventTime, float elapsed);
  virtual void               ModelJustLoaded();

 protected:
  NTempest::C3Vector GetMovement(UINT time);
  int                FindAnimData(CGGameObject_C *owner);
  UINT               NextKeyID() const;

  LISTDECLEX(CMovementData, transportLink, m_passengers);
  const TransportAnimationRec       *m_keys;
  UINT                               m_numKeys;
  UINT                               m_currKey;
  NTempest::C3Vector                 m_position;
  float                              m_currSpeed;
  NTempest::C3Vector                 m_currDirection;
  TSGrowableArray<NTempest::C4Plane> m_interior;
};

class CGGameObject_C_Type_Camera : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_Camera(CGGameObject_C *owner);
};

class CGGameObject_C_Type_DuelArbiter : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_DuelArbiter(CGGameObject_C *owner);
  virtual bool CanHighlight() const;
  virtual bool CanUse() const;
};

class CGGameObject_C_Type_FishingNode : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_FishingNode(CGGameObject_C *owner);
  virtual bool CanUse() const;
};

class CGGameObject_C_Type_Ritual : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Ritual(CGGameObject_C *owner);
  virtual bool CanUseNow(GAME_ERROR_TYPE *reason) const;
};

class CGGameObject_C : public CGObject_C, public CGGameObject {
  friend BOOL ObjectCollisionProc(DWORDLONG param64, DWORD param32, WorldObjCollisionHandlerData *data);
  friend class CGGameObject_C_TypeAnimated;
  friend class CGGameObject_C_Type_AreaDamage;
  friend class CGGameObject_C_Type_Door;
  friend class CGGameObject_C_Type_MapObj;
  friend class CGGameObject_C_Type_MapObjTransport;
  friend class CGGameObject_C_Type_Transport;

 public:
  CGGameObject_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGGameObject_C();

  void         SetStorage(DWORD *storage);
  void         PostInit(const CClientObjCreate &init);
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  virtual BOOL UpdateModelLoadStatus();
  BOOL         SetBlock(UINT i, DWORD data);
  void         SetData(LPCVOID data, UINT bytes);

  virtual NTempest::C3Vector  GetPosition() const;
  virtual void                GetPosition(NTempest::C3Vector &vec) const;
  virtual float               GetFacing() const;
  virtual NTempest::C3Vector  GetCurrentMoveVector() const;
  virtual LPCSTR              GetModelFileName() const;
  virtual BOOL                CanHighlight() const;
  virtual BOOL                IsSolidSelectable() const;
  virtual BOOL                IsSolidCollidable() const;
  virtual BOOL                FloatingTooltip() const;
  virtual void                OnRightClick();
  virtual BOOL                IsPointInside(const NTempest::C3Vector &point) const;
  virtual NTempest::C34Matrix GetMatrix() const;
  virtual LPCSTR              GetObjectName() const;
  virtual int                 GetPageTextID(void (*func)(int, const DWORDLONG &, LPVOID, bool)) const;
  virtual void                GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  virtual void ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);

  BOOL               IsTransport() const;
  int                GetPageTextLanguage() const;
  int                GetPageTextMaterial() const;
  void               LoadBaseObject(const GameObjectStats *stats);
  void               PostMovementUpdate();
  void               UpdateMovement(DWORD eventTime, float elapsed);
  void               AddPassenger(CMovementData *passenger);
  UINT               CreateWorldObject(DWORDLONG guid);
  void               SetMirrorHandlers();
  void               UnsetMirrorHandlers();
  void               PostPostInit();
  void               UpdateMatrix();
  void               ActivateCustomAnim(UINT anim);
  LPCSTR             GetName() const;
  LPCSTR             GetTypeName() const;
  LPCSTR             GetDebugStatus() const;
  bool               CanChangeCursor() const;
  bool               CanUse() const;
  bool               CanUseNow() const;
  int                GetType() const;
  UINT               GetPropertyValue(UINT index) const;
  const LockRec     *GetLockRec() const;
  bool               IsFriend(const CGUnit_C *unit) const;
  bool               IsPeaceful(const CGUnit_C *unit) const;
  bool               IsEnemy(const CGUnit_C *unit) const;
  UINT               GetServerTimeOffset();
  void               SetSolid(bool solid);
  bool               IsLocked(int *spellID, int *spellSkill, int *lockSkill, CGItem_C **itemPtr, int *openIndex) const;
  bool               IsValidOpenAction(int action) const;
  void               StartInteraction();
  void               CloseInteraction();
  UNIT_REACTION      ObjectReaction(const CGUnit_C *unit) const;
  bool               IsValidTargetForSpell(const DWORDLONG &caster, int spellID) const;
  bool               IsQuestObjectForMe();
  HCOLLISIONDATA__  *GetCollideData() const;
  NTempest::C3Vector GetCollideMin() const;
  NTempest::C3Vector GetCollideMax() const;
  NTempest::CAaBox   GetCollideExtents() const;

  static void Initialize();
  static void Shutdown();
  static UINT OffsetOf(OBJECT_TYPE_ID type);

  LINKDECLEX(CGGameObject_C, moveLink);
  CGGameObject_C_TypeBase *m_baseObj;

 private:
  CGGameObject_C        &operator=(const CGGameObject_C &);
  LPCSTR                 GetModelFileNameInternal() const;
  const GameObjectStats *m_stats;
  NTempest::C34Matrix    m_matrix;
  HMODEL__              *m_collideModel;
  HCOLLISIONDATA__      *m_collideData;
  NTempest::CAaBox       m_collideExtents;
  UINT                   m_serverTimeOffset;
  int                    m_isSolid : 1;
  int                    m_isQuestChestForMe : 1;
};

#endif
