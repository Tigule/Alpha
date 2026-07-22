#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_GAMEOBJECT_C_H

#include "Object_C.h"
#include "Unit_C.h"

#include <stpl.h>

#include "Tempest/c34matrix.h"
#include "Tempest/caabox.h"
#include "Ui/GameUI.h"

class CMovementData;
class CGGameObject_C;
class CGUnit_C;
class LockRec;
struct GameObjectStats;
struct HCOLLISIONDATA__;
struct HMODEL__;
struct WorldObjCollisionHandlerData;

int __fastcall ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data);

struct CGGameObjectData {
  unsigned int       m_data[8];
  NTempest::C3Vector m_position;
  float              m_facing;
  unsigned int       m_dynamicFlags;
  int                m_factionTemplate;
};

class CGGameObject {
  friend class CGGameObject_C_TypeBase;

 protected:
  CGGameObjectData *m_gameObj;
};

class CGGameObject_C_TypeBase {
 public:
  CGGameObject_C *m_owner;

  virtual ~CGGameObject_C_TypeBase();
  virtual unsigned int       CanHighlight();
  virtual unsigned int       CanChangeCursor();
  virtual unsigned int       CanUse();
  virtual unsigned int       CanUseNow(GAME_ERROR_TYPE *reason);
  virtual unsigned int       Use(const unsigned __int64 &activator);
  virtual void               UpdateState(int oldState, int newState);
  virtual void               HandleAnimEvent(const char *eventName, const NTempest::C3Vector &position);
  virtual void               HandleAnimFinished();
  virtual const char        *DebugStatus();
  virtual void               ActivateCustomAnim(unsigned int anim);
  virtual NTempest::C3Vector GetPosition() const;
  virtual float              GetFacing() const;
  virtual void               AddPassenger(CMovementData *passenger);
  virtual NTempest::C3Vector GetCurrentMoveVector() const;
  virtual int                IsPointInside(NTempest::C3Vector &point);
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

class CGGameObject_C : public CGObject_C, public CGGameObject {
  friend int __fastcall ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data);

 public:
  CGGameObject_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  virtual ~CGGameObject_C();

  void SetStorage(unsigned long *storage);

  virtual NTempest::C3Vector GetPosition() const;
  virtual void               GetPosition(NTempest::C3Vector &vec) const;
  virtual float              GetFacing() const;
  virtual const char        *GetModelFileName() const;

  int           IsTransport() const;
  int           GetPageTextLanguage() const;
  int           GetPageTextMaterial() const;
  int           GetType();
  unsigned int  GetPropertyValue(unsigned int index);
  LockRec      *GetLockRec();
  unsigned int  IsValidOpenAction(int action);
  void          StartInteraction();
  void          CloseInteraction();
  UNIT_REACTION ObjectReaction(const CGUnit_C *unit) const;
  unsigned int  IsValidTargetForSpell(const unsigned __int64 &caster, int spellID);

  const CGGameObjectData *GetGameObjectData() const {
    return m_gameObj;
  }

  static void __fastcall Shutdown();

  TSLink<CGGameObject_C>   moveLink;
  CGGameObject_C_TypeBase *m_baseObj;

 private:
  const char         *GetModelFileNameInternal() const;
  GameObjectStats    *m_stats;
  NTempest::C34Matrix m_matrix;
  HMODEL__           *m_collideModel;
  HCOLLISIONDATA__   *m_collideData;
  NTempest::CAaBox    m_collideExtents;
  unsigned int        m_serverTimeOffset;
  int                 m_isSolid;
  int                 m_isQuestChestForMe;
};

#endif
