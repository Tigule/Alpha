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

int ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data);

struct CGGameObjectData {
  int                    m_displayID;
  unsigned int           m_flags;
  NTempest::C4Quaternion m_rotation;
  int                    m_state;
  unsigned int           m_timestamp;
  NTempest::C3Vector     m_position;
  float                  m_facing;
  unsigned int           m_dynamicFlags;
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

  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static unsigned int TotalFields();
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();

  unsigned char *GetData(unsigned int index);
  void SetStorage(unsigned long *storage) {
    m_gameObj = reinterpret_cast<CGGameObjectData *>(storage);
  }

  CGGameObjectData *GameObject() {
    return m_gameObj;
  }

  const CGGameObjectData *GameObject() const {
    return m_gameObj;
  }

  int GetDisplayID() const;
  const NTempest::C4Quaternion &GetRotation() const;
  int GetState() const;
  unsigned int GetTimeStamp() const;
  unsigned int GetGameObjectFlags() const;
  void GetObjectPosition(NTempest::C3Vector &position) const;
  NTempest::C3Vector GetObjectPosition() const;
  float GetObjectFacing() const;
  int GetFactionTemplate() const;
  bool GetDisabled() const;
  bool GetLocked() const;
  bool GetQuestOnly() const;

 protected:
  explicit CGGameObject(unsigned long *storage) {
    SetStorage(storage);
  }

  ~CGGameObject() {
  }

  CGGameObjectData *m_gameObj;
};

class CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_TypeBase()
      : m_owner(0), m_interactDistance(5.0f) {
  }
  CGGameObject_C_TypeBase(CGGameObject_C *owner);

  CGGameObject_C *m_owner;

  virtual ~CGGameObject_C_TypeBase();
  virtual bool               CanHighlight() const;
  virtual bool               CanChangeCursor() const;
  virtual bool               CanUse() const;
  virtual bool               CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual bool               Use(const unsigned __int64 &activator);
  virtual void               UpdateState(int oldState, int newState);
  virtual void               HandleAnimEvent(const char *eventName, const NTempest::C3Vector &position);
  virtual void               HandleAnimFinished();
  virtual const char        *DebugStatus();
  virtual void               ActivateCustomAnim(unsigned int anim);
  virtual NTempest::C3Vector GetPosition() const;
  virtual float              GetFacing() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual NTempest::C3Vector GetCurrentMoveVector() const;
  virtual int                IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               PostInit();
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               PostReenable();
  virtual void               UpdateMovement(unsigned long eventTime, float elapsed);
  virtual void               ModelJustLoaded();
  virtual void               StartInteraction();
  virtual void               CloseInteraction();

 protected:
  float m_interactDistance;
};

class CGGameObject_C_Type_Null : public CGGameObject_C_TypeBase {
 public:
  virtual bool        CanUse() const;
  virtual bool        CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual const char  *DebugStatus();
};

class CGGameObject_C_TypeAnimated : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_TypeAnimated(CGGameObject_C *owner);
  virtual ~CGGameObject_C_TypeAnimated();
  virtual void        UpdateState(int oldState, int newState);
  virtual void        HandleAnimEvent(const char *eventName, const NTempest::C3Vector &position);
  virtual void        HandleAnimFinished();
  virtual const char *DebugStatus();
  virtual void        ActivateCustomAnim(unsigned int anim);
  virtual void        UpdateAnimState(unsigned int newState);
  virtual void        PostInit();
  virtual void        Disable(int shutdown);
  virtual void        ModelJustLoaded();

 protected:
  void PlayAnimatedSound(int index, const NTempest::C3Vector &position);
  void CloseLoopingSound();
  void SetSequence();

  unsigned int m_animState;
  unsigned char m_useFallbackAnim[11];
  Sound        *m_loopingSound;
  unsigned int  m_animPresent;
};

class CGGameObject_C_Type_Door : public CGGameObject_C_TypeAnimated {
 public:
  CGGameObject_C_Type_Door(CGGameObject_C *owner);
  virtual bool CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual void UpdateAnimState(unsigned int newState);
  bool         IsAtRest() const;
  unsigned int GetStartOpen() const;
  unsigned int GetAutoClose() const;
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
  virtual void         PostInit();

 protected:
  unsigned int m_objectId;
};

class CGGameObject_C_Type_MapObjTransport : public CGGameObject_C_Type_MapObj {
 public:
  CGGameObject_C_Type_MapObjTransport(CGGameObject_C *owner);
  virtual ~CGGameObject_C_Type_MapObjTransport();
  virtual NTempest::C3Vector GetPosition() const;
  virtual float              GetFacing() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual int                IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               UpdateMovement(unsigned long eventTime, float elapsed);

 protected:
  LISTDECLEX(CMovementData, transportLink, m_passengers);
  NTempest::C3Spline_CatmullRom    m_path[2];
  unsigned int                     m_tripTime[2];
  NTempest::C3Vector               m_position;
  float                            m_facing;
};

class CGGameObject_C_Type_Chair : public CGGameObject_C_TypeBase {
 public:
  CGGameObject_C_Type_Chair(CGGameObject_C *owner);
  virtual bool CanUseNow(GAME_ERROR_TYPE *reason) const;
  virtual void PostInit();
  unsigned int GetNumSlots() const;
  unsigned int GetHeight() const;

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
  virtual bool Use(const unsigned __int64 &activator);
  virtual void         PostInit();
  virtual void         StartInteraction();
  virtual void         CloseInteraction();
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
  virtual int                IsPointInside(const NTempest::C3Vector &point) const;
  virtual void               Reenable();
  virtual void               Disable(int shutdown);
  virtual void               UpdateMovement(unsigned long eventTime, float elapsed);
  virtual void               ModelJustLoaded();

 protected:
  NTempest::C3Vector GetMovement(unsigned int time);
  int                FindAnimData(CGGameObject_C *owner);
  unsigned int       NextKeyID() const;

  LISTDECLEX(CMovementData, transportLink, m_passengers);
  const TransportAnimationRec      *m_keys;
  unsigned int                      m_numKeys;
  unsigned int                      m_currKey;
  NTempest::C3Vector                m_position;
  float                             m_currSpeed;
  NTempest::C3Vector                m_currDirection;
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
  friend int ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data);
  friend class CGGameObject_C_TypeAnimated;
  friend class CGGameObject_C_Type_AreaDamage;
  friend class CGGameObject_C_Type_Door;
  friend class CGGameObject_C_Type_MapObj;
  friend class CGGameObject_C_Type_MapObjTransport;
  friend class CGGameObject_C_Type_Transport;

 public:
  CGGameObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGGameObject_C();

  void SetStorage(unsigned long *storage);
  void PostInit(const CClientObjCreate &init);
  virtual void Disable(int shutdown);
  virtual void Reenable();
  virtual void PostReenable();
  virtual int  UpdateModelLoadStatus();
  int          SetBlock(unsigned int i, unsigned long data);
  void         SetData(const void *data, unsigned int bytes);

  virtual NTempest::C3Vector GetPosition() const;
  virtual void               GetPosition(NTempest::C3Vector &vec) const;
  virtual float              GetFacing() const;
  virtual NTempest::C3Vector GetCurrentMoveVector() const;
  virtual const char        *GetModelFileName() const;
  virtual int                CanHighlight() const;
  virtual int                IsSolidSelectable() const;
  virtual int                IsSolidCollidable() const;
  virtual int                FloatingTooltip() const;
  virtual void               OnRightClick();
  virtual int                IsPointInside(const NTempest::C3Vector &point) const;
  virtual NTempest::C34Matrix GetMatrix() const;
  virtual const char        *GetObjectName() const;
  virtual int                GetPageTextID(void(*func)(int, const unsigned __int64 &, void *, bool)) const;
  virtual void               GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const;
  virtual void               ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);

  int           IsTransport() const;
  int           GetPageTextLanguage() const;
  int           GetPageTextMaterial() const;
  void          LoadBaseObject(const GameObjectStats *stats);
  void          PostMovementUpdate();
  void          UpdateMovement(unsigned long eventTime, float elapsed);
  void          AddPassenger(CMovementData *passenger);
  unsigned int  CreateWorldObject(unsigned __int64 guid);
  void          SetMirrorHandlers();
  void          UnsetMirrorHandlers();
  void          PostPostInit();
  void          UpdateMatrix();
  void          ActivateCustomAnim(unsigned int anim);
  const char   *GetName() const;
  const char   *GetTypeName() const;
  const char   *GetDebugStatus() const;
  bool          CanChangeCursor() const;
  bool          CanUse() const;
  bool          CanUseNow() const;
  int           GetType() const;
  unsigned int  GetPropertyValue(unsigned int index) const;
  const LockRec *GetLockRec() const;
  bool          IsFriend(const CGUnit_C *unit) const;
  bool          IsPeaceful(const CGUnit_C *unit) const;
  bool          IsEnemy(const CGUnit_C *unit) const;
  unsigned int  GetServerTimeOffset();
  void          SetSolid(bool solid);
  bool          IsLocked(int *spellID, int *spellSkill, int *lockSkill, CGItem_C **itemPtr, int *openIndex) const;
  bool          IsValidOpenAction(int action) const;
  void          StartInteraction();
  void          CloseInteraction();
  UNIT_REACTION ObjectReaction(const CGUnit_C *unit) const;
  bool          IsValidTargetForSpell(const unsigned __int64 &caster, int spellID) const;
  bool          IsQuestObjectForMe();
  HCOLLISIONDATA__ *GetCollideData() const;
  NTempest::C3Vector GetCollideMin() const;
  NTempest::C3Vector GetCollideMax() const;
  NTempest::CAaBox GetCollideExtents() const;

  static void Initialize();
  static void Shutdown();
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);

  LINKDECLEX(CGGameObject_C, moveLink);
  CGGameObject_C_TypeBase *m_baseObj;

 private:
  CGGameObject_C &operator=(const CGGameObject_C &);
  const char         *GetModelFileNameInternal() const;
  const GameObjectStats *m_stats;
  NTempest::C34Matrix m_matrix;
  HMODEL__           *m_collideModel;
  HCOLLISIONDATA__   *m_collideData;
  NTempest::CAaBox    m_collideExtents;
  unsigned int        m_serverTimeOffset;
  int                 m_isSolid : 1;
  int                 m_isQuestChestForMe : 1;
};

#endif
