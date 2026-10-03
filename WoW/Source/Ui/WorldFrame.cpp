#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "WorldFrame.h"
#include <Os/OsTime.h>

#include "Frame/CLayoutFrame.h"
#include "Frame/CSimpleTop.h"
#include "Component/Component.h"
#include "DayNight.h"
#include "Game/GameTime.h"
#include "Game/GameClient/PlayerName.h"
#include "Game/GameClient/WorldText.h"
#include "Object/MovementData.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Cursor.h"
#include "UIUtil/Tooltip.h"
#include "Ui/GameUI.h"
#include "Ui/LootFrame.h"
#include "Ui/PartyFrame.h"
#include "Ui/ChatFrame.h"
#include "Ui/Tutorial.h"
#include "Ui/UIBindings.h"
#include "WorldClient/World.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/Coordinate.h>
#include <Base/Status.h>
#include <Console/ConsoleClient.h>
#include <Console/ConsoleVar.h>
#include <FrameScript/FrameScript.h>
#include <Event/CMouseEvent.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Os/W32/OsSound.h>
#include <Services/IParticleMisc.h>
#include <Services/Camera.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Tempest/cmath.h>
#include <storm.h>

#include <string.h>
#include <float.h>

NODEDECL(CModelRecord) {
  HMODEL    model;
  float     distance;
  float     scale;
  DWORDLONG guid;

  CModelRecord() : model(0), distance(INFINITY), scale(1.0f), guid(0) {
  }
  CModelRecord(const CModelRecord &source);
  ~CModelRecord();
  CModelRecord &operator=(const CModelRecord &source);
  void          Copy(const CModelRecord &source);
};

struct FADEOUTHASHOBJ : public TSHashObject<FADEOUTHASHOBJ, CHashKeyGUID> {
  HMODEL              model;
  HTEXCOMPONENT__    *texture;
  NTempest::C34Matrix matrix;
  float               renderScale;
  UINT                startTime;
  BYTE                startAlpha;

  ~FADEOUTHASHOBJ() {
    if (model) {
      HandleClose(model);
    }
    if (texture) {
      HandleClose(texture);
    }
  }
};

int              Player_C_SetPlayerRender(int enable);
bool             Spell_C_IsTargeting();
void             CursorResetCursor(int force);
bool             Spell_C_CanTargetObjects();
bool             Spell_C_CanTargetObject(const CGObject_C *objectPtr);
bool             Spell_C_CanTargetUnits();
bool             Spell_C_CanTargetMe();
bool             Spell_C_CanTargetParty();
bool             Spell_C_CanTargetFriends();
bool             Spell_C_CanTargetEnemies();
bool             Spell_C_CanTargetDead();
bool             Spell_C_CanTargetItems();
bool             Spell_C_CanTargetTerrain();
bool             Spell_C_WaitingForStringInput();
UINT             Spell_C_WorldObjectCursor();
float            Spell_C_WorldObjectFacing();
bool             Spell_C_WorldObjectHousing();
float            Spell_C_GetSpellRadius();
bool             Spell_C_HandleSpriteRay(const CSpriteClickEvent &evt, bool checkRange);
bool             Spell_C_HandleTerrainRay(const CTerrainClickEvent &evt, bool checkRange);
int              Spell_C_GetTargettingSpell();
const DWORDLONG &Spell_C_GetCurrentCaster();
void             WorldTextInitialize();
void             WorldTextShutdown();
void             SmartScreenRectInitialize();
void             SmartScreenRectShutdown();
void             SmartScreenRectClearAllGrids();
void             UnitEffectUpdate(CGCamera *camera);
void             UnitFootprintRenderSplats(const NTempest::C3Vector &cameraPos);
void             SpellVisualsRender();
void             SpellVisualsTick(float elapsed);
void             UpdatePortraits();
void             ModelRenderSceneOpaque(CStatus *status);
void             ModelRenderSceneTransparent(CStatus *status);
static BOOL             ObjectEnumProc(LPVOID param, DWORD status, DWORDLONG param64, DWORD param32);
static BOOL             ObjectCollisionProc(DWORDLONG param64, DWORD param32, WorldObjCollisionHandlerData *data);

static NTempest::C44Matrix IDENTITY;
static LPCSTR s_spellShadowName[2] = {"Interface\\SpellShadow\\Spell-Shadow-Acceptable.blp", "Interface\\SpellShadow\\Spell-Shadow-Unacceptable.blp"};
static CVar  *s_playerFadeCVar;
static CVar  *s_playerFadeInRateCVar;
static CVar  *s_playerFadeOutRateCVar;
static CVar  *s_playerFadeOutAlphaCVar;
enum SPELLSHADOWSTYLE {
  SPELL_GOOD = 0,
  SPELL_BAD = 1,
  NUM_SPELL_SHADOWS = 2,
  SPELL_NONE = 3
};

static SPELLSHADOWSTYLE                          s_spellShadowStyle = SPELL_NONE;
static const DWORD                               AUTO_SIT_IDLE_TIME = 300000;
static const DWORD                               PLAYER_MOVE_TUTORIAL_TIME = 90000;
static const DWORD                               CAMERA_MOVE_TUTORIAL_TIME = 120000;
static const DWORD                               AUTO_LOGOUT_IDLE_TIME = 1800000;
static NTempest::C3Vector                        s_spellShadowPos;
static float                                     s_spellShadowSize;
static HTEXTURE                                  s_spellShadowTexture[2];
static TSHashTable<FADEOUTHASHOBJ, CHashKeyGUID> s_fadeOutModelTable;

inline QUEST_GIVER_STATUS CGUnit_C::GetQuestGiverStatus() {
  return m_questGiverStatus;
}

CGWorldFrame *CGWorldFrame::s_currentWorldFrame;

BOOL CGWorldFrame::IsUnitLegalSelection(const CGUnit_C *unit, UINT hitFilter) {
  if (hitFilter & 0x70000) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if ((hitFilter & 0x10000) && player->IsUnitInGroup(unit)) {
      return 1;
    }
    if ((hitFilter & 0x20000) && player->CanAssist(unit)) {
      return 1;
    }
    if (!(hitFilter & 0x40000) || !player->CanAttack(unit)) {
      return 0;
    }
  }

  if (hitFilter & 0x300000) {
    if (unit->GetHealth() <= 0) {
      return (hitFilter & 0x200000) != 0;
    }
    return (hitFilter & 0x100000) != 0;
  }
  return 1;
}

BOOL CGWorldFrame::IsLegalSelection(CModelRecord *record, UINT hitFilter) {
  CGObject_C *object = ClntObjMgrObjectPtr(record->guid, __FILE__, __LINE__);
  FATALASSERT(object);

  switch (object->GetType()) {
    case HIER_TYPE_PLAYER:
      if (!(hitFilter & 8) || (!(hitFilter & 0x10) && object->GetGUID() == ClntObjMgrGetActivePlayer())) {
        return 0;
      }
      return IsUnitLegalSelection(static_cast<CGUnit_C *>(object), hitFilter);
    case HIER_TYPE_UNIT:
      if (!(hitFilter & 4)) {
        return 0;
      }
      return IsUnitLegalSelection(static_cast<CGUnit_C *>(object), hitFilter);
    case HIER_TYPE_GAMEOBJECT:
      if (!(hitFilter & 2)) {
        return 0;
      }
      if (Spell_C_IsTargeting() && !Spell_C_CanTargetObject(object)) {
        return 0;
      }
      return 1;
    case HIER_TYPE_CORPSE:
      if (!(hitFilter & 4)) {
        return 0;
      }
      return !Spell_C_IsTargeting();
  }
  return 0;
}

void CGWorldFrame::MoveToFreeList(CModelRecord *record) {
  m_models.UnlinkNode(record);
  m_freeModels.LinkNode(record, LIST_TAIL, 0);
  HandleClose(record->model);
  record->model = 0;
}

void CGWorldFrame::MoveToFreeList(LISTPTR(CModelRecord) objList) {
  ITERATELISTPTR(CModelRecord, objList, record) {
    HandleClose(record->model);
    record->model = 0;
  }
  m_freeModels.Combine(objList, LIST_TAIL, 0);
}

UINT CGWorldFrame::SphereTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector, UINT hitFilter) {
  const NTempest::C3Vector &cameraPos = m_camera->Position();
  UINT                      numHit = 0;

  ModelSceneCalcFrustumPlanes();
  for (CModelRecord *record = m_models.Head(), *next; (int)record > 0 ? ((next = m_models.RawNext(record)), 1) : 0; record = next) {
    CGObject_C *object = ClntObjMgrObjectPtr(record->guid, __FILE__, __LINE__);
    if (object && object->IsSolidSelectable()) {
      NTempest::C34Matrix worldMatrix;
      object->GetWorldMatrix(&worldMatrix);
      NTempest::C34Matrix camRelativeMatrix = worldMatrix;
      camRelativeMatrix.d0 -= cameraPos.x;
      camRelativeMatrix.d1 -= cameraPos.y;
      camRelativeMatrix.d2 -= cameraPos.z;
      if (ModelTestSphere(record->model, camRelativeMatrix, record->scale, 1) &&
          ModelHitTestSphere(record->model, record->scale, aVector, bVector, 1, &record->distance))
      {
        if (IsLegalSelection(record, hitFilter)) {
          ++numHit;
        } else {
          m_models.UnlinkNode(record);
          m_filteredModels.LinkNode(record, LIST_TAIL, 0);
        }
        continue;
      }
    }
    MoveToFreeList(record);
  }
  return numHit;
}

UINT CGWorldFrame::VolumeTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector) {
  UINT numHit = 0;
  for (CModelRecord *record = m_models.Head(), *next; (int)record > 0 ? ((next = m_models.RawNext(record)), 1) : 0; record = next) {
    if (!ModelHasHitTestVolumes(record->model)) {
      ++numHit;
    } else if (ModelHitTestVolumes(record->model, record->scale, aVector, bVector, 1, &record->distance)) {
      ++numHit;
    } else {
      MoveToFreeList(record);
    }
  }
  return numHit;
}

UINT CGWorldFrame::GeometryTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector) {
  UINT numHit = 0;
  for (CModelRecord *record = m_models.Head(), *next; (int)record > 0 ? ((next = m_models.RawNext(record)), 1) : 0; record = next) {
    if (ModelHitTestGeometry(record->model, record->scale, aVector, bVector, 1, &record->distance)) {
      ++numHit;
    } else {
      MoveToFreeList(record);
    }
  }
  return numHit;
}

static int GetObjectSelectCategory(CGObject_C *object) {
  enum {
    PRIORITY_NON_INTERACTABLE = 0,
    PRIORITY_LIVING = 2,
    PRIORITY_INTERACTABLE = 1
  };

  switch (object->GetType()) {
    case HIER_TYPE_UNIT:
    case HIER_TYPE_PLAYER:
      if (static_cast<CGUnit_C *>(object)->GetHealth() > 0) {
        return PRIORITY_LIVING;
      }
      return static_cast<CGUnit_C *>(object)->CanBeLooted(OsGetAsyncTimeMs()) ? PRIORITY_INTERACTABLE : PRIORITY_NON_INTERACTABLE;
    case HIER_TYPE_GAMEOBJECT:
      return static_cast<CGGameObject_C *>(object)->CanUse() ? PRIORITY_INTERACTABLE : PRIORITY_NON_INTERACTABLE;
  }
  return PRIORITY_NON_INTERACTABLE;
}

CModelRecord *CGWorldFrame::HigherPriorityModel(CModelRecord *a, CModelRecord *b) {
  CGObject_C *aObject = ClntObjMgrObjectPtr(a->guid, __FILE__, __LINE__);
  int         aCategory = GetObjectSelectCategory(aObject);
  CGObject_C *bObject = ClntObjMgrObjectPtr(b->guid, __FILE__, __LINE__);
  int         bCategory = GetObjectSelectCategory(bObject);
  if (aCategory > bCategory) {
    MoveToFreeList(b);
    return a;
  }
  if (aCategory < bCategory || b->distance < a->distance) {
    MoveToFreeList(a);
    return b;
  }
  MoveToFreeList(b);
  return a;
}

void CGWorldFrame::ReduceToClosestModel() {
  CModelRecord *closest = m_models.Head();
  for (CModelRecord *record = m_models.RawNext(closest), *next; (int)record > 0 ? ((next = m_models.RawNext(record)), 1) : 0; record = next) {
    closest = HigherPriorityModel(closest, record);
  }
}

void CGWorldFrame::HideObstructingModels(float maxDist) {
  DWORDLONG fade = 0;
  for (CModelRecord *record = m_filteredModels.Head(), *next; (int)record > 0 ? ((next = m_filteredModels.RawNext(record)), 1) : 0; record = next) {
    if (record->guid == CGPlayer_C::GetActive()) {
      if (record->distance <= maxDist) {
        fade = record->guid;
      }
      break;
    }
  }
  SendUnitFadeEvent(fade);
}

DWORDLONG CGWorldFrame::FindClosestModel(const NTempest::C3Vector &a, const NTempest::C3Vector &b, UINT hitFilter, float *hitDist) {
  NTempest::C3Vector cameraPos = m_camera->Position();
  NTempest::C3Vector aVector = a - cameraPos;
  NTempest::C3Vector bVector = b - cameraPos;

  if (!SphereTestModels(aVector, bVector, hitFilter)) {
    return 0;
  }

  switch (VolumeTestModels(aVector, bVector)) {
    case 0:
      return 0;
    case 1:
      break;
    default:
      switch (GeometryTestModels(aVector, bVector)) {
        case 0:
          return 0;
        case 1:
          break;
        default:
          ReduceToClosestModel();
          break;
      }
      break;
  }

  CModelRecord *picked = m_models.Head();
  FATALASSERT(picked);
  *hitDist = picked->distance;
  return picked->guid;
}

CGWorldFrame::HIT_TYPE CGWorldFrame::HitTest(const NTempest::C3Vector &a, const NTempest::C3Vector &b, UINT hitFilter, HitTestResult *hitTestResult) {
  FATALASSERT(hitTestResult);

  NTempest::C3Vector ip(0.0f);
  float              terrDist = 1.0f;
  int                terrainFound = 0;
  if (hitFilter & 0x1) {
    terrainFound = CWorld::Intersect(&a, &b, 0.0f, &ip, &terrDist, 273);
    if (terrainFound) {
      terrDist *= (a - b).Mag();
    }
  }

  float     objDist = 0.0f;
  DWORDLONG object = 0;
  if (hitFilter & 0x1E) {
    object = FindClosestModel(a, b, hitFilter, &objDist);
  }

  if (!object && !terrainFound) {
    return HIT_NONE;
  }

  if (!object || (terrainFound && terrDist < objDist)) {
    NTempest::C3Vector direction = b - a;
    float              mag = direction.Mag();
    hitTestResult->point = a;
    if (NTempest::CMath::fnotequal_(mag, 0.0f)) {
      hitTestResult->point += direction * (terrDist / mag);
    }
    hitTestResult->distance = terrDist;
    return HIT_GROUND;
  }

  hitTestResult->object = object;
  NTempest::C3Vector direction = b - a;
  float              mag = direction.Mag();
  hitTestResult->point = a;
  if (NTempest::CMath::fnotequal_(mag, 0.0f)) {
    hitTestResult->point += direction * (objDist / mag);
  }
  hitTestResult->distance = objDist;
  return HIT_OBJECT;
}

UINT CGWorldFrame::GetHitTestFilterFlags() const {
  if (!Spell_C_IsTargeting()) {
    return ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__) ? 0xE : 0;
  }

  UINT hitFilter = 0;
  if (Spell_C_CanTargetTerrain()) {
    hitFilter |= 0x1;
  }
  if (Spell_C_CanTargetObjects()) {
    hitFilter |= 0x2;
  }
  if (Spell_C_CanTargetUnits()) {
    hitFilter |= 0xC;
    if (Spell_C_CanTargetMe()) {
      hitFilter |= 0x10;
    }
    if (Spell_C_CanTargetParty()) {
      hitFilter |= 0x10000;
    }
    if (Spell_C_CanTargetFriends()) {
      hitFilter |= 0x20000;
    }
    if (Spell_C_CanTargetEnemies()) {
      hitFilter |= 0x40000;
    }
    hitFilter |= Spell_C_CanTargetDead() ? 0x200000 : 0x100000;
  }

  FATALASSERT(Spell_C_CanTargetItems() || Spell_C_WaitingForStringInput() || (hitFilter != HIT_TEST_NOTHING));
  return hitFilter;
}

CGWorldFrame::HIT_TYPE CGWorldFrame::HitTestPoint(float x, float y, HitTestResult *hitTestResult) {
  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  m_camera->SetupWorldProjection(m_rect);

  HIT_TYPE hitType = HIT_NONE;
  UINT     hitFilter = GetHitTestFilterFlags();
  if (hitFilter) {
    NTempest::C3Vector a;
    NTempest::C3Vector b;
    if (GetLineSegment(x, y, &a, &b)) {
      hitType = HitTest(a, b, hitFilter, hitTestResult);
      if (hitType < HIT_OBJECT) {
        MoveToFreeList(&m_filteredModels);
      }
    }
  }

  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
  return hitType;
}

BOOL CGWorldFrame::GetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b) {
  if (!PtInFrameRect(NTempest::C2Vector(x, y))) {
    return 0;
  }

  float lineX = (x - m_rect.l) / (m_rect.r - m_rect.l);
  float lineY = (y - m_rect.t) / (m_rect.b - m_rect.t);
  CameraGetLineSegment(lineX, lineY, a, b);

  NTempest::C3Vector cameraPos = m_camera->Position();
  *a += cameraPos;
  *b += cameraPos;
  return 1;
}

void CGWorldFrame::UpdateObject(CGObject_C *object, DWORD status) {
  FATALASSERT(object);

  HMODEL model = object->GetObjectModel();
  if (model) {
    object->UpdateModelLoadStatus();
    object->UpdateAttachmentLoadStatus();
    object->UpdateTexComponentLoadStatus();

    if (ModelAdvanceTime(model)) {
      object->PreRender(m_updateTimeStamp, m_elapsedSec);
      if (object->ShouldRender(status)) {
        object->PreAnimate(this);

        NTempest::C34Matrix worldMatrix;
        object->GetWorldMatrix(&worldMatrix);
        NTempest::C34Matrix camRelativeMatrix = worldMatrix;
        camRelativeMatrix.d0 -= m_camera->Position().x;
        camRelativeMatrix.d1 -= m_camera->Position().y;
        camRelativeMatrix.d2 -= m_camera->Position().z;

        object->Animate(camRelativeMatrix);
        ModelProcessEvents(model, worldMatrix);
        object->PostAnimate(this);
        AddModelToScene(object, model);
        object->ObjectPostAnimate(camRelativeMatrix, m_camera->Position(), m_camera->Position() + m_camera->Forward());
      }
    }
  }
}

void CGWorldFrame::AddModelToScene(CGObject_C *object, HMODEL model) {
  CModelRecord *record;
  if (m_freeModels.IsEmpty()) {
    record = m_models.NewNode(LIST_TAIL, 0, 0);
  } else {
    record = m_freeModels.Head();
    m_freeModels.UnlinkNode(record);
    m_models.LinkNode(record, LIST_TAIL, 0);
  }

  record->model = reinterpret_cast<HMODEL>(HandleDuplicate(reinterpret_cast<HOBJECT>(model)));
  record->guid = object->GetGUID();
  record->scale = object->GetScale();
  ModelAddToScene(model, 0);
}

void CGWorldFrame::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_top->m_mouseFocus == this) {
    NTempest::C2Vector mousePos(0.0f);
    NDCToDDC(m_top->m_mousePosition.x, m_top->m_mousePosition.y, &mousePos.x, &mousePos.y);

    HitTestResult hitTestResult;
    s_spellShadowStyle = SPELL_NONE;
    switch (HitTestPoint(mousePos.x, mousePos.y, &hitTestResult)) {
      case HIT_GROUND:
        OnLayerTrackTerrain(hitTestResult);
        break;

      case HIT_OBJECT:
        OnLayerTrackObject(hitTestResult, mousePos.x, mousePos.y);
        break;

      default:
        if (Spell_C_IsTargeting()) {
          CursorModelSetSequence(CAST_ERROR_CURSOR);
        } else {
          CursorResetCursor(0);
        }
        SendObjectTrackEvent(0, 0.0f, 0.0f);
        break;
    }

    HideObstructingModels(hitTestResult.distance);
  }

  CGInputControl::GetActive()->OnUpdate(elapsedSec);
  m_elapsedSec = elapsedSec;
  CGGameUI::UpdateInteractTarget();
}

CGCamera *CGWorldFrame::GetActiveCamera() {
  FATALASSERT(s_currentWorldFrame);
  FATALASSERT(s_currentWorldFrame->m_camera);
  return s_currentWorldFrame->m_camera;
}

void CGWorldFrame::GetCameraPosition(NTempest::C3Vector *position) {
  FATALASSERT(position);
  FATALASSERT(s_currentWorldFrame);
  FATALASSERT(s_currentWorldFrame->m_camera);

  *position = s_currentWorldFrame->m_camera->Position();
}

void CGWorldFrame::GetCameraFacing(NTempest::C3Vector *position) {
  FATALASSERT(position);
  FATALASSERT(s_currentWorldFrame);
  FATALASSERT(s_currentWorldFrame->m_camera);

  *position = s_currentWorldFrame->m_camera->Forward();
}

static BOOL CheckFadeOutModels(LPCSTR command, LPCSTR arguments) {
  int  count = 0;
  UINT currentTime = OsGetAsyncTimeMs();
  ITERATELIST(FADEOUTHASHOBJ, s_fadeOutModelTable, fade) {
    ConsolePrintf("Model %02d: %d ms elapsed\n", ++count, currentTime - fade->startTime);
  }
  ConsolePrintf("Found %d models, nuking. If this improves Anim let Jeff know", count);
  s_fadeOutModelTable.Clear();
  return 1;
}

CGWorldFrame::CGWorldFrame(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_spriteButtons(1),
      m_terrainButtons(1),
      m_lastUnitFade(0),
      m_lastObjectTrack(0),
      m_lastUpdateElapsedSec(0.0f),
      m_renderPlayer(1),
      m_freeLookMode(0),
      m_flags(0),
      m_playerFadeMode(PLAYERFADE_NONE),
      m_playerAlpha(255),
      m_cameraAlpha(255),
      m_cameraAlphaChanged(0) {
  FATALASSERT(!s_currentWorldFrame || !"Error, why is there more than one world frame being created?");
  s_currentWorldFrame = this;

  SetAllPoints(m_top, 1);
  EnableEvent(SIMPLE_EVENT_KEY, static_cast<UINT>(-1));
  EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
  EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, static_cast<UINT>(-1));
  memset(m_lastKey, 0, sizeof(m_lastKey));

  m_camera = NEW(CGCamera);
  FATALASSERT(m_camera);

  s_playerFadeCVar = CVar::Register("PlayerFadeMouseOver", "Controls fading of the local player when mousing over", 0, "0", 0, DEFAULT, false, 0);
  s_playerFadeInRateCVar = CVar::Register("PlayerFadeInRate", "fade in rate for player mouseover", 0, "4096", 0, DEFAULT, false, 0);
  s_playerFadeOutRateCVar = CVar::Register("PlayerFadeOutRate", "fade out rate for player mouseover", 0, "4096", 0, DEFAULT, false, 0);
  s_playerFadeOutAlphaCVar = CVar::Register("PlayerFadeOutAlpha", "min fade out alpha for player mouseover", 0, "128", 0, DEFAULT, false, 0);

  SetSpriteClickButtons(5);
  WorldTextInitialize();
  CGUnit_C::NamePlateShow(0);

  CStatus status;
  for (UINT i = 0; i < 2; ++i) {
    FATALASSERT(s_spellShadowTexture[i] == 0);
    s_spellShadowTexture[i] = TextureCreate(s_spellShadowName[i], CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
    SysMsgAdd(status, 1);
  }

  SmartScreenRectInitialize();
  s_spellShadowStyle = SPELL_GOOD;
  s_fadeOutModelTable.Clear();
  ConsoleCommandRegister("SeeIfWorldFrameSucks", CheckFadeOutModels, DEBUG, 0);
  GxMasterEnableSet(GxMasterEnable_ClearOnPresent, 0);
}

CGWorldFrame::~CGWorldFrame() {
  ConsoleCommandUnregister("debugobjectpathing");
  ConsoleCommandUnregister("SeeIfWorldFrameSucks");
  EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
  s_currentWorldFrame = 0;

  for (UINT i = 0; i < 2; ++i) {
    if (s_spellShadowTexture[i]) {
      HandleClose(s_spellShadowTexture[i]);
    }
    s_spellShadowTexture[i] = 0;
  }
  WorldTextShutdown();
  SmartScreenRectShutdown();

  m_models.Clear();
  m_filteredModels.Clear();
  m_freeModels.Clear();

  s_fadeOutModelTable.Clear();

  if (m_camera) {
    DEL(m_camera);
  }
  m_camera = 0;

  GxMasterEnableSet(GxMasterEnable_ClearOnPresent, 1);
}

void CGWorldFrame::SetSpriteClickButtons(UINT buttons) {
  m_spriteButtons = buttons;
}

void CGWorldFrame::SetTerrainClickButtons(UINT buttons) {
  m_terrainButtons = buttons;
}

BOOL CGWorldFrame::PerformDefaultAction(MOUSEBUTTON button, UINT timestamp) {
  NTempest::C2Vector mousePos(0.0f);
  NDCToDDC(m_top->m_mousePosition.x, m_top->m_mousePosition.y, &mousePos.x, &mousePos.y);

  if (!CGGameUI::HasPlayerControl()) {
    CWorldClickEvent worldClickEvent;
    worldClickEvent.button = button;
    return CGGameUI::HandleWorldClick(worldClickEvent);
  }

  HitTestResult hitTestResult;
  switch (HitTestPoint(mousePos.x, mousePos.y, &hitTestResult)) {
    case HIT_OBJECT: {
      CSpriteClickEvent spriteClickEvent;
      spriteClickEvent.pos = mousePos;
      spriteClickEvent.objectGUID = hitTestResult.object;
      spriteClickEvent.button = button;
      spriteClickEvent.time = timestamp;
      return CGGameUI::HandleSpriteClick(spriteClickEvent);
    }

    case HIT_GROUND: {
      CTerrainClickEvent terrainClickEvent;
      terrainClickEvent.point = hitTestResult.point;
      terrainClickEvent.button = button;
      return CGGameUI::HandleTerrainClick(terrainClickEvent);
    }

    case HIT_NONE: {
      CWorldClickEvent worldClickEvent;
      worldClickEvent.button = button;
      return CGGameUI::HandleWorldClick(worldClickEvent);
    }
  }
  return 0;
}

BOOL CGWorldFrame::SendUnitFadeEvent(DWORDLONG guid) {
  if (guid == m_lastUnitFade) {
    return 0;
  }

  HandleUnitFade(0, guid != 0);
  m_lastUnitFade = guid;
  return 1;
}

BOOL CGWorldFrame::SendObjectTrackEvent(DWORDLONG guid, float x, float y) {
  if (guid == m_lastObjectTrack) {
    return 0;
  }

  CObjectTrackEvent spriteTrackEvent;
  spriteTrackEvent.object = guid;
  spriteTrackEvent.oldGUID = m_lastObjectTrack;
  spriteTrackEvent.x = x;
  spriteTrackEvent.y = y;
  CGGameUI::HandleSpriteTrack(spriteTrackEvent);
  m_lastObjectTrack = guid;
  return 1;
}

void CGWorldFrame::OnLayerTrackTerrain(const HitTestResult &hitTestResult) {
  if (Spell_C_IsTargeting() && Spell_C_CanTargetTerrain()) {
    CTerrainClickEvent evt;
    evt.point = hitTestResult.point;
    s_spellShadowPos = hitTestResult.point;
    if (Spell_C_HandleTerrainRay(evt, true)) {
      s_spellShadowStyle = SPELL_GOOD;
      s_spellShadowSize = __min(Spell_C_GetSpellRadius(), 20.0f);
      CursorModelSetSequence(CAST_CURSOR);
    } else {
      s_spellShadowStyle = SPELL_BAD;
      s_spellShadowSize = 0.0f;
      CursorModelSetSequence(CAST_ERROR_CURSOR);
    }

    UINT cursor = Spell_C_WorldObjectCursor();
    if (cursor) {
      CWorld::ObjectUpdate(cursor, s_spellShadowPos, Spell_C_WorldObjectFacing(), Spell_C_WorldObjectHousing());
    }
  } else {
    CursorResetCursor(0);
    SendObjectTrackEvent(0, 0.0f, 0.0f);
  }
}

void CGWorldFrame::CursorTrackUnit(CGUnit_C *unit) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player->m_flags & 0x800) {
    CGUnit_C *possessed = player->GetPossessedUnit();
    if (unit->GetHealth() > 0 && possessed && possessed->GetHealth() > 0 && !possessed->IsMounted() &&
        possessed->CanAttack(unit))
    {
      CursorModelSetSequence(ATTACK_CURSOR);
    } else {
      CursorResetCursor(0);
    }
    return;
  }

  float sqMag = (player->m_move.GetPosition() - unit->GetPosition()).SquaredMag();
  if (player->CanInteract(unit) && unit->GetHealth() > 0) {
    int  outOfRange = sqMag > MAX_SHOP_DISTANCE_SQUARED;
    UINT npcFlags = unit->GetUnitNPCFlags();
    if (npcFlags & 0x1) {
      CursorModelSetSequence(outOfRange ? PICKUP_ERROR_CURSOR : PICKUP_CURSOR);
    } else if ((npcFlags & 0x2) && unit->GetQuestGiverStatus() != QUEST_GIVER_NONE && unit->GetQuestGiverStatus() != QUEST_GIVER_FUTURE) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else if (npcFlags & 0x4) {
      CursorModelSetSequence(outOfRange ? TAXI_ERROR_CURSOR : TAXI_CURSOR);
    } else if (npcFlags & 0x8) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else if (npcFlags & 0x10) {
      CursorModelSetSequence(outOfRange ? INTERACT_ERROR_CURSOR : INTERACT_CURSOR);
    } else if (npcFlags & 0x20) {
      CursorModelSetSequence(outOfRange ? BUY_ERROR_CURSOR : BUY_CURSOR);
    } else if ((npcFlags & 0xC0) || unit->IsGuildRegistrar()) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else {
      CursorResetCursor(0);
    }
    return;
  }

  if (unit->CanBeLooted(m_updateTimeStamp)) {
    CursorModelSetSequence(
        player->CanLoot(unit) || unit->GetGUID() == player->m_lootingUnitSent || unit->GetGUID() == player->GetUnitBeingLooted() ? PICKUP_CURSOR
                                                                                                                             : PICKUP_ERROR_CURSOR
    );
  } else if (
      player->GetHealth() > 0 && !player->IsMounted() && unit->GetHealth() > 0 && player->CanAttack(unit)
  )
  {
    CursorModelSetSequence(sqMag <= 109.202499f ? ATTACK_CURSOR : ATTACK_ERROR_CURSOR);
  } else {
    CursorResetCursor(0);
  }
}

void CGWorldFrame::CursorTrackObject(CGGameObject_C *gameObject) {
  if (gameObject->CanChangeCursor()) {
    if (gameObject->CanUseNow()) {
      CursorModelSetSequence(INTERACT_CURSOR);
    } else {
      CursorModelSetSequence(INTERACT_ERROR_CURSOR);
    }
  } else {
    CursorResetCursor(0);
  }
}

void CGWorldFrame::OnLayerTrackObject(const HitTestResult &hitTestResult, float x, float y) {
  if (m_freeLookMode) {
    SendObjectTrackEvent(0, 0.0f, 0.0f);
    return;
  }

  if (Spell_C_IsTargeting()) {
    CSpriteClickEvent clickEvt;
    clickEvt.objectGUID = hitTestResult.object;
    if (Spell_C_HandleSpriteRay(clickEvt, true)) {
      CursorModelSetSequence(CAST_CURSOR);
    } else {
      CursorModelSetSequence(CAST_ERROR_CURSOR);
    }
  } else {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    CGObject_C *object = ClntObjMgrObjectPtr(hitTestResult.object, __FILE__, __LINE__);
    if (!player || !object || !object->CanHighlight()) {
      SendObjectTrackEvent(0, 0.0f, 0.0f);
      CursorResetCursor(0);
      return;
    }

    switch (object->GetType()) {
      case HIER_TYPE_UNIT:
      case HIER_TYPE_PLAYER:
        CursorTrackUnit(static_cast<CGUnit_C *>(object));
        break;

      case HIER_TYPE_GAMEOBJECT:
        CursorTrackObject(static_cast<CGGameObject_C *>(object));
        break;

      case HIER_TYPE_ITEM:
        CursorModelSetSequence(PICKUP_CURSOR);
        break;

      default:
        CursorResetCursor(0);
        break;
    }
  }

  SendObjectTrackEvent(hitTestResult.object, x, y);
}

BOOL CGWorldFrame::OnLayerTrackUpdate(const CMouseEvent &evt) {
  int result = CSimpleFrame::OnLayerTrackUpdate(evt);
  if (!result) {
    return result;
  }
  CGInputControl::GetActive();
  return 1;
}

void CGWorldFrame::UpdateDayNightInfo(float elapsedSec) {
  DNInfo *dnInfo = DayNightGetInfo();
  dnInfo->farClip = m_camera->FarZ();
  dnInfo->nearClipScaled = m_camera->NearZ() * 3.0f;
  dnInfo->elapsedSec = elapsedSec;
  dnInfo->cameraPos = m_camera->Position();
  dnInfo->cameraDir = m_camera->Forward();
  dnInfo->cameraDir.Normalize();

  DWORDLONG guid = ClntObjMgrGetActivePlayer();
  if (guid) {
    CGObject_C *player = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
    if (player) {
      dnInfo->playerPos = player->GetPosition();
    }
  }

  dnInfo->time = g_clientGameTime.GetHourAndMinutes();
  dnInfo->dayProgression = g_clientGameTime.GameTimeGetDayProgression();
  dnInfo->day = static_cast<float>(g_clientGameTime.GetDaysSinceEpoch());
}

void CGWorldFrame::SetPlayerFadeCameraValue(BYTE value) {
  if (value != m_cameraAlpha) {
    if (m_camera->m_target == ClntObjMgrGetActivePlayer() && ((value && !m_cameraAlpha) || (!value && m_cameraAlpha))) {
      Player_C_SetPlayerRender(value != 0);
    }

    m_cameraAlpha = value;
    m_cameraAlphaChanged = 1;
  }
}

void CGWorldFrame::RefreshPlayerAlpha() {
  CGObject_C *object = ClntObjMgrObjectPtr(m_camera->m_target, __FILE__, __LINE__);
  if (object && static_cast<bool>((static_cast<UINT>(object->GetType()) >> ID_UNIT) & 1)) {
    object->SetMaxAlpha(__min(m_cameraAlpha, m_playerAlpha));
  }
}

void CGWorldFrame::UpdatePlayerAlpha(float elapsedSeconds) {
  if (m_cameraAlpha && (m_playerFadeMode || m_cameraAlphaChanged)) {
    switch (m_playerFadeMode) {
      case PLAYERFADE_IN:
        FATALASSERT(s_playerFadeInRateCVar);
        m_playerAlpha += static_cast<int>(s_playerFadeInRateCVar->GetFloat() * elapsedSeconds);
        if (m_playerAlpha > 255) {
          m_playerAlpha = 255;
          m_playerFadeMode = PLAYERFADE_NONE;
        }
        break;

      case PLAYERFADE_OUT: {
        FATALASSERT(s_playerFadeOutRateCVar);
        FATALASSERT(s_playerFadeOutAlphaCVar);
        m_playerAlpha -= static_cast<int>(s_playerFadeOutRateCVar->GetFloat() * elapsedSeconds);
        int minAlpha = s_playerFadeOutAlphaCVar->GetInt();
        if (minAlpha <= 1) {
          minAlpha = 1;
        }
        if (m_playerAlpha < minAlpha) {
          m_playerAlpha = minAlpha;
          m_playerFadeMode = PLAYERFADE_NONE;
        }
        break;
      }

      case PLAYERFADE_NONE:
        m_cameraAlphaChanged = 0;
        break;

      default:
        FATALASSERT(!"Error, unrecognized player fade mode!");
        break;
    }
    RefreshPlayerAlpha();
  }
}

void CGWorldFrame::HandleUnitFade(int nowTracking, int immediateFade) {
  if (!s_playerFadeCVar || s_playerFadeCVar->GetInt()) {
    if (immediateFade) {
      m_playerAlpha = 255;
      m_playerFadeMode = PLAYERFADE_IN;
    } else {
      m_playerFadeMode = nowTracking ? PLAYERFADE_OUT : PLAYERFADE_IN;
    }
  }
}

static BOOL UnitUpdateProc(DWORDLONG guid, LPVOID param) {
  CGWorldFrame *pWorldFrame = static_cast<CGWorldFrame *>(param);
  FATALASSERT(pWorldFrame);

  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (static_cast<bool>((static_cast<UINT>(object->GetType()) >> ID_UNIT) & 1)) {
    static_cast<CGUnit_C *>(object)->UpdateDisplay(pWorldFrame->m_updateTimeStamp);
  }
  return 1;
}

void CGWorldFrame::UnitUpdate() {
  MoveToFreeList(&m_models);
  MoveToFreeList(&m_filteredModels);
  ClntObjMgrEnumVisibleObjects(UnitUpdateProc, this);
}

void CGWorldFrame::OnFrameRender(CRenderBatch *batch, UINT layer) {
  CSimpleFrame::OnFrameRender(batch, layer);
  if (!layer) {
    batch->QueueCallback(RenderWorld, this);
  }
}

void CGWorldFrame::RenderWorld(LPVOID param) {
  CGWorldFrame       *worldFrame = static_cast<CGWorldFrame *>(param);
  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;

  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  worldFrame->OnWorldUpdate();
  worldFrame->OnWorldRender();
  PlayerNameRenderWorldText();
  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
}

void CGWorldFrame::OnWorldUpdate() {
  float elapsedSec = m_elapsedSec;

  UINT idleTime = OsGetAsyncTimeMs() - m_top->m_eventTime;
  if (static_cast<int>(idleTime - AUTO_SIT_IDLE_TIME) >= 0) {
    if (static_cast<int>(idleTime - AUTO_LOGOUT_IDLE_TIME) >= 0) {
      if (!ClientServices_CharacterLoggingOut()) {
        CGChat::AddChatMessage(FrameScript_GetText("IDLE_MESSAGE", -1, GENDER_NOT_APPLICABLE), static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
        ClientServices_CharacterLogout(false);
      }
    } else {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(CGPlayer_C::GetActive(), __FILE__, __LINE__));
      if (player && !player->GetStandState()) {
        player->ChangeStandState(1);
      }
    }
  }

  CGInputControl *inputControl = CGInputControl::GetActive();
  FATALASSERT(inputControl);
  if (!inputControl->HasPlayerMoved() && static_cast<int>(OsGetAsyncTimeMs() - inputControl->GetInitializeTime() - PLAYER_MOVE_TUTORIAL_TIME) >= 0) {
    CGTutorial::TriggerTutorial(TUTORIAL_MOVEMENT);
  }
  if (!inputControl->HasCameraMoved() && static_cast<int>(OsGetAsyncTimeMs() - inputControl->GetInitializeTime() - CAMERA_MOVE_TUTORIAL_TIME) >= 0) {
    CGTutorial::TriggerTutorial(TUTORIAL_CAMERA);
  }

  m_flags &= ~3u;
  CGCamera::UpdateCallback(0, m_camera);
  NTempest::CRect projection;
  GetRect(&projection);

  m_camera->SetupWorldProjection(projection);

  NTempest::C3Vector cameraPos = m_camera->Position();
  NTempest::C3Vector target = m_camera->Position() + m_camera->Forward();
  NTempest::C3Vector facing = m_camera->Forward();
  ModelScenePlaceCamera(cameraPos, facing);
  ModelSceneCalcFrustumPlanes();
  Sound::SetListenerAttributes(cameraPos, 0, m_camera->Forward(), m_camera->Up());

  UpdateDayNightInfo(elapsedSec);
  UpdatePlayerAlpha(elapsedSec);
  m_updateTimeStamp = OsGetAsyncTimeMs();
  UpdatePortraits();
  UnitUpdate();

  CWorld::SetUpdateTime(elapsedSec, OsGetAsyncTimeMs());
  CWorld::SetObjectHandler(ObjectEnumProc, this);
  CWorld::SetObjectCollisionHandler(ObjectCollisionProc);

  CGObject_C *object = ClntObjMgrObjectPtr(m_camera->m_target, __FILE__, __LINE__);
  if (!object && CGPlayer_C::GetActive()) {
    FATALASSERT(m_camera->GetTarget() != CGPlayer_C::GetActive());
    object = ClntObjMgrObjectPtr(CGPlayer_C::GetActive(), __FILE__, __LINE__);
    FATALASSERT(object);
    CGPlayer_C *player = static_cast<CGPlayer_C *>(object);
    if (player->IsInFarSight()) {
      player->ToggleFarSight();
    }
    m_camera->SetTarget(object);
  }

  CWorld::SetCameraTarget(object->GetWorldObject());
  CWorld::PrepareUpdate(cameraPos, target);
  CWorld::Update();
  ParticleSystemManager::GetInstance()->UpdateEmitters(elapsedSec, cameraPos, target);
  RibbonManager::GetInstance()->UpdateEmitters(elapsedSec, cameraPos, target);
  SpellVisualsTick(elapsedSec);

  NTempest::C44Matrix newMatrix;
  GxXformViewProj(newMatrix);
  if (newMatrix != m_worldMatrix) {
    m_flags |= 3;
    m_worldMatrix = newMatrix;
  }
}

static BOOL ObjectEnumProc(LPVOID param, DWORD status, DWORDLONG param64, DWORD param32) {
  CGWorldFrame *pWorldFrame = static_cast<CGWorldFrame *>(param);
  FATALASSERT(pWorldFrame);

  CGObject_C *object = ClntObjMgrObjectPtr(param64, __FILE__, __LINE__);
  FATALASSERT(object);

  if ((param64 != CGPlayer_C::GetRealActivePlayer() || ClntObjMgrGetActivePlayer() == CGPlayer_C::GetRealActivePlayer()) && object &&
      !object->IsDisabled())
  {
    pWorldFrame->UpdateObject(object, status);
  }

  return 1;
}

static BOOL ObjectCollisionProc(DWORDLONG param64, DWORD param32, WorldObjCollisionHandlerData *data) {
  FATALASSERT(data);

  CGObject_C *object = ClntObjMgrObjectPtr(param64, __FILE__, __LINE__);
  FATALASSERT(object);
  if (!object->IsSolidCollidable()) {
    return 0;
  }

  if (!static_cast<bool>((static_cast<UINT>(object->GetType()) >> ID_GAMEOBJECT) & 1)) {
    return 0;
  }

  CGGameObject_C *gameObject = static_cast<CGGameObject_C *>(object);
  data->model = object->GetObjectModel();
  data->collideExt = gameObject->m_collideExtents;
  data->scale = object->GetScale() * object->GetRenderScale();

  data->matrix = NTempest::C44Matrix(object->GetMatrix());
  return 1;
}

static void RenderFadeOutModels(const NTempest::C3Vector cameraPos, const NTempest::C3Vector cameraTarg) {
  int currentTime = OsGetAsyncTimeMs();
  for (FADEOUTHASHOBJ *curr = s_fadeOutModelTable.Head(); curr;) {
    FATALASSERT(curr->model);
    int elapsed = currentTime - curr->startTime;
    if (elapsed > 2000) {
      FADEOUTHASHOBJ *next = s_fadeOutModelTable.Next(curr);
      s_fadeOutModelTable.Delete(curr);
      curr = next;
    } else {
      float alpha = (1.0f - static_cast<float>(elapsed > 0 ? elapsed : 0) * 0.0005f) * static_cast<float>(curr->startAlpha) * (1.0f / 255.0f);
      alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
      ModelSetVertexAlpha(curr->model, NTempest::CMath::ftol_0_256_(alpha * 255.0f), 1);

      NTempest::C34Matrix camRelativeMatrix = curr->matrix;
      camRelativeMatrix.d0 -= cameraPos.x;
      camRelativeMatrix.d1 -= cameraPos.y;
      camRelativeMatrix.d2 -= cameraPos.z;
      ModelAnimate(curr->model, camRelativeMatrix, curr->renderScale, cameraPos, cameraTarg - cameraPos);
      ModelAddToScene(curr->model, 0);
      curr = s_fadeOutModelTable.Next(curr);
    }
  }
}

static void DrawCursorShadow() {
  UINT cursor = Spell_C_WorldObjectCursor();
  if (cursor) {
    NTempest::C3Vector position = s_spellShadowPos - CGWorldFrame::GetActiveCamera()->Position();
    NTempest::CAaBox   extents;
    CWorld::ObjectGetExtents(cursor, extents);

    CStatus  status;
    HTEXTURE texture = TextureCreateSolid(NTempest::CImVector(s_spellShadowStyle ? 0x40FF0000 : 0x4000FF00), &status);
    FATALASSERT(texture);
    HMODEL model = ModelCreateBox(extents, texture, GxBlend_Alpha);
    if (model) {
      ModelAnimate(
          model, position, Spell_C_WorldObjectFacing(), NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1.0f, NTempest::C3Vector(0.0f), NTempest::C3Vector(0.0f)
      );
      ModelAddToScene(model, 0);
      HandleClose(model);
    }
    HandleClose(texture);
  } else {
    float            z = CWorld::CalcAltitude(s_spellShadowPos.x, s_spellShadowPos.y, 1.0f);
    float            r = s_spellShadowSize == 0.0f ? 1.3888888f : s_spellShadowSize;
    NTempest::CAaBox box;
    box.t = NTempest::C3Vector(s_spellShadowPos.x + r, s_spellShadowPos.y + r, z + 2.0f);
    box.b = NTempest::C3Vector(s_spellShadowPos.x - r, s_spellShadowPos.y - r, z - 2.0f);

    GxRsPush();
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Blend, GxBlend_Alpha);
    if (s_spellShadowTexture[s_spellShadowStyle]) {
      GxRsSet(GxRs_Texture0, TextureGetGxTex(s_spellShadowTexture[s_spellShadowStyle], 1, 0));
    }
    ProjectTex2d(box, NTempest::CImVector(0xFFFFFFFF), 0, 0.5f);
    GxRsPop();
  }
}

void CGWorldFrame::OnWorldRender() {
  UINT rsStackOffset = GxRsStackOffset();

  GxRsPush();
  PlayerNameUpdateEarly();
  CWorld::SetEnvironment();
  CWorld::Render();

  if (m_flags & 3) {
    CGUnit_C::ResortAllUnitNameplates(this);
  }
  if (m_flags & 1) {
    CGUnit_C::UpdateUnitNameplates(this);
  }

  UnitFootprintRenderSplats(m_camera->Position());
  if (s_spellShadowStyle < 2 && s_spellShadowTexture[s_spellShadowStyle]) {
    DrawCursorShadow();
  }

  UnitEffectUpdate(m_camera);
  ParticleSystemManager::GetInstance()->RenderEmitters();
  RibbonManager::GetInstance()->RenderEmitters();

  NTempest::C3Vector cameraPos = m_camera->Position();
  NTempest::C3Vector target = cameraPos + m_camera->Forward();
  RenderFadeOutModels(cameraPos, target);
  CGUnit_C_RenderBowStrings(cameraPos);

  GxRsSet(GxRs_TexLodBias0, -1.0f);
  ModelRenderSceneOpaque(0);
  CWorld::RenderAlpha();
  SpellVisualsRender();
  ModelRenderSceneTransparent(0);
  RenderCollisionInfo();
  DayNightRenderGlares();
  PlayerNameUpdateLate();
  GxRsPop();

  ASSERT(rsStackOffset == GxRsStackOffset());
  Player_C_ClearGuildIDs();
  SmartScreenRectClearAllGrids();
}

void CGWorldFrame::OnLayerCursorExit() {
  CSimpleFrame::OnLayerCursorExit();
  if (m_lastUnitFade) {
    HandleUnitFade(0, 1);
    m_lastUnitFade = 0;
  }
  if (m_lastObjectTrack) {
    CObjectTrackEvent spriteTrackEvent;
    spriteTrackEvent.object = 0;
    spriteTrackEvent.oldGUID = m_lastObjectTrack;
    CGGameUI::HandleSpriteTrack(spriteTrackEvent);
    m_lastObjectTrack = 0;
  }
  CursorResetCursor(0);
  s_spellShadowStyle = SPELL_NONE;
}

BOOL CGWorldFrame::OnLayerKeyDown(CKeyEvent &evt) {
  if (CSimpleFrame::OnLayerKeyDown(evt)) {
    return 1;
  }
  if (evt.key < KEY_LAST) {
    char *string = m_lastKey[evt.key];
    if (CGUIBindings::KeyEventToString(evt, string, sizeof(m_lastKey[0]))) {
      return CGUIBindings::GetActive()->ExecKey(string, evt.time, 1);
    }
  }
  return 0;
}

BOOL CGWorldFrame::OnLayerKeyUp(CKeyEvent &evt) {
  if (CSimpleFrame::OnLayerKeyUp(evt)) {
    return 1;
  }
  if (evt.key >= KEY_LAST) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  DWORD       processTime = evt.time;
  if (player && (player->m_move.GetMoveFlags() & 0x200) && static_cast<long>(processTime - player->m_move.GetMoveStartTime()) < 0) {
    processTime = player->m_move.GetMoveStartTime();
  }

  char *key = m_lastKey[evt.key];
  int   result = 0;
  if (*key || (CGUIBindings::KeyEventToString(evt, key, sizeof(m_lastKey[evt.key])), *key)) {
    result = CGUIBindings::GetActive()->ExecKey(key, processTime, 0);
    *key = 0;
  }
  return result;
}

BOOL CGWorldFrame::OnLayerMouseDown(CMouseEvent &evt) {
  if (CSimpleFrame::OnLayerMouseDown(evt)) {
    return 1;
  }
  if (CGGameUI::HandleMouseDown(evt)) {
    return 1;
  }
  char keyName[32];
  if (CGUIBindings::MouseEventToString(evt, keyName, sizeof(keyName))) {
    return CGUIBindings::GetActive()->ExecKey(keyName, evt.time, 1);
  }
  return 0;
}

BOOL CGWorldFrame::OnLayerMouseUp(CMouseEvent &evt) {
  if (CSimpleFrame::OnLayerMouseUp(evt)) {
    return 1;
  }
  if (CGGameUI::HandleMouseUp(evt)) {
    return 1;
  }
  char keyName[32];
  if (CGUIBindings::MouseEventToString(evt, keyName, sizeof(keyName))) {
    return CGUIBindings::GetActive()->ExecKey(keyName, evt.time, 0);
  }
  return 0;
}

BOOL CGWorldFrame::OnLayerMouseWheel(CMouseEvent &evt) {
  if (CSimpleFrame::OnLayerMouseWheel(evt)) {
    return 1;
  }
  char keyName[32];
  if (CGUIBindings::MouseEventToString(evt, keyName, sizeof(keyName))) {
    return CGUIBindings::GetActive()->ExecKey(keyName, evt.time, 1);
  }
  return 0;
}

BOOL CGWorldFrame::OnLayerMouseMoveRelative(CMouseEvent &evt) {
  CGInputControl::GetActive()->OnMouseMoveRel(evt);
  return 1;
}

DWORDLONG CGWorldFrame::GetObjectUnderMouse() {
  if (!m_models.Head()) {
    return 0;
  }
  return m_models.Head()->guid;
}

float CGWorldFrame::GetSkyProgress() {
  return static_cast<int>((g_clientGameTime.GetHourAndMinutes() + 720) % 1440u) * 0.00069444446f * m_skyAnimDuration;
}

BOOL CGWorldFrame::TogglePlayerRender() {
  m_renderPlayer = !m_renderPlayer;
  return m_renderPlayer;
}

int CGWorldFrame::SetPlayerRender(int state) {
  int oldState = m_renderPlayer;
  m_renderPlayer = state;
  return oldState;
}

void CGWorldFrame::OnMouseModeNormal() {
  m_freeLookMode = 0;
}

void CGWorldFrame::OnMouseModeRelative() {
  m_freeLookMode = 1;
  if (m_lastUnitFade) {
    HandleUnitFade(0, 1);
    m_lastUnitFade = 0;
  }
  if (m_lastObjectTrack) {
    CObjectTrackEvent spriteTrackEvent;
    spriteTrackEvent.object = 0;
    spriteTrackEvent.oldGUID = m_lastObjectTrack;
    CGGameUI::HandleSpriteTrack(spriteTrackEvent);
    m_lastObjectTrack = 0;
  }
}

NTempest::C2Vector CGWorldFrame::GetScreenCoordinates(const NTempest::C3Vector &worldPosition) {
  NTempest::C4Vector position(worldPosition);
  NTempest::C3Vector cameraPos = m_camera->Position();
  position -= NTempest::C4Vector(cameraPos);
  position = position * m_worldMatrix;
  float inverseW = 1.0f / position.w;
  position.x = position.x * inverseW;
  position.y = position.y * inverseW;
  position.z = position.z * inverseW;
  position.w = position.w * inverseW;
  position += 1.0f;
  position *= 0.5f;

  float screenx;
  float screeny;
  NDCToDDC(position.x, position.y, &screenx, &screeny);
  screenx = __min(__max(screenx, 0.0f), 0.8f);
  screeny = __min(__max(screeny, 0.0f), 0.6f);
  return NTempest::C2Vector(screenx, screeny);
}

NTempest::C2Vector CGWorldFrame::GetScreenCoordinates(
    const NTempest::C3Vector &worldPosition, const NTempest::C44Matrix &worldMatrix, int doNotNormalize, int worldSpaceSpecified
) {
  NTempest::C4Vector position(worldPosition);
  if (worldSpaceSpecified) {
    FATALASSERT(m_camera);
    position -= NTempest::C4Vector(m_camera->Position());
  }

  position = position * worldMatrix;
  float inverseW = 1.0f / position.w;
  position.x = position.x * inverseW;
  position.y = position.y * inverseW;
  position.z = position.z * inverseW;
  position.w = position.w * inverseW;
  position += 1.0f;
  position *= 0.5f;

  float screenx = position.x;
  float screeny = position.y;
  if (!doNotNormalize) {
    NDCToDDC(position.x, position.y, &screenx, &screeny);
    screenx = __min(__max(screenx, 0.0f), 0.8f);
    screeny = __min(__max(screeny, 0.0f), 0.6f);
  }
  return NTempest::C2Vector(screenx, screeny);
}

void CGWorldFrame::SetNamePlateUpdate() {
  m_flags |= 1;
}

CModelRecord::~CModelRecord() {
  if (model) {
    HandleClose(model);
  }
}

CModelRecord &CModelRecord::operator=(const CModelRecord &source) {
  if (model) {
    HandleClose(model);
  }
  Copy(source);
  return *this;
}

void CModelRecord::Copy(const CModelRecord &source) {
  model = static_cast<HMODEL>(HandleDuplicate(source.model));
  distance = source.distance;
  scale = source.scale;
  guid = source.guid;
}

void CGWorldFrame::RegisterObjectFadeoutModel(CGObject_C *object, HTEXCOMPONENT texture, BYTE startAlpha) {
  FATALASSERT(object);

  HMODEL model = object->GetObjectModel();
  if (!model) {
    return;
  }

  CHashKeyGUID key(object->GetGUID());
  UINT         hash = static_cast<UINT>(object->GetGUID());
  if (s_fadeOutModelTable.Ptr(hash, key)) {
    return;
  }

  NTempest::C34Matrix worldMatrix;
  object->GetWorldMatrix(&worldMatrix);
  FADEOUTHASHOBJ *fade = s_fadeOutModelTable.New(hash, key, 0, 0);
  fade->model = static_cast<HMODEL>(HandleDuplicate(model));
  fade->texture = texture ? static_cast<HTEXCOMPONENT>(HandleDuplicate(texture)) : 0;
  fade->matrix = worldMatrix;
  fade->renderScale = object->GetScale() * object->GetRenderScale();
  fade->startTime = OsGetAsyncTimeMs();
  fade->startAlpha = startAlpha;
}

void CGWorldFrame::SetCameraTarget(CGObject_C *target) {
  FATALASSERT(m_camera);
  m_camera->SetTarget(target);
}
