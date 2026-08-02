#include <WowConst.h>
#include <MapDefs.h>

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
#include "Ui/GameUI.h"
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
  HMODEL           model;
  float            distance;
  float            scale;
  unsigned __int64 guid;

  CModelRecord(const CModelRecord &source);
  CModelRecord() : model(0), distance(FLT_MAX), scale(1.0f), guid(0) {
  }
  ~CModelRecord();
  CModelRecord &operator=(const CModelRecord &source);
  void          Copy(const CModelRecord &source);
};

CModelRecord::~CModelRecord() {
  if (model) {
    HandleClose(model);
  }
}

struct FADEOUTHASHOBJ : public TSHashObject<FADEOUTHASHOBJ, CHashKeyGUID> {
  HMODEL              model;
  HTEXCOMPONENT__    *texture;
  NTempest::C34Matrix matrix;
  float               renderScale;
  unsigned int        startTime;
  unsigned char       startAlpha;

  ~FADEOUTHASHOBJ() {
    if (model) {
      HandleClose(model);
    }
    if (texture) {
      HandleClose(texture);
    }
  }
};

int Player_C_SetPlayerRender(int enable);
bool Spell_C_IsTargeting();
void CursorResetCursor(int force);
bool Spell_C_CanTargetObjects();
bool Spell_C_CanTargetUnits();
bool Spell_C_CanTargetMe();
bool Spell_C_CanTargetParty();
bool Spell_C_CanTargetFriends();
bool Spell_C_CanTargetEnemies();
bool Spell_C_CanTargetDead();
bool Spell_C_CanTargetItems();
bool Spell_C_CanTargetTerrain();
bool Spell_C_WaitingForStringInput();
unsigned int Spell_C_WorldObjectCursor();
float Spell_C_WorldObjectFacing();
bool Spell_C_WorldObjectHousing();
float Spell_C_GetSpellRadius();
bool Spell_C_HandleSpriteRay(const CSpriteClickEvent &evt, bool checkRange);
bool Spell_C_HandleTerrainRay(const CTerrainClickEvent &evt, bool checkRange);
int Spell_C_GetTargettingSpell();
const unsigned __int64 &Spell_C_GetCurrentCaster();
void WorldTextInitialize();
void WorldTextShutdown();
void SmartScreenRectInitialize();
void SmartScreenRectShutdown();
void SmartScreenRectClearAllGrids();
void UnitEffectUpdate(CGCamera *camera);
void UnitFootprintRenderSplats(const NTempest::C3Vector &cameraPos);
void                               SpellVisualsRender();
void                               SpellVisualsTick(float elapsed);
void UpdatePortraits();
void ModelRenderSceneOpaque(CStatus *status);
void ModelRenderSceneTransparent(CStatus *status);
int ObjectEnumProc(void *param, unsigned long status, unsigned __int64 param64, unsigned long param32);
int ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data);

static const char *s_spellShadowName[2] = {
    "Interface\\SpellShadow\\Spell-Shadow-Acceptable.blp", "Interface\\SpellShadow\\Spell-Shadow-Unacceptable.blp"
};
static CVar *s_playerFadeCVar;
static CVar *s_playerFadeInRateCVar;
static CVar *s_playerFadeOutRateCVar;
static CVar *s_playerFadeOutAlphaCVar;
enum SPELLSHADOWSTYLE {
  SPELL_GOOD = 0,
  SPELL_BAD = 1,
  NUM_SPELL_SHADOWS = 2,
  SPELL_NONE = 3
};

static SPELLSHADOWSTYLE                          s_spellShadowStyle = SPELL_NONE;
static const unsigned long                       AUTO_SIT_IDLE_TIME = 300000;
static const unsigned long                       PLAYER_MOVE_TUTORIAL_TIME = 90000;
static const unsigned long                       CAMERA_MOVE_TUTORIAL_TIME = 120000;
static const unsigned long                       AUTO_LOGOUT_IDLE_TIME = 1800000;
static NTempest::C3Vector                        s_spellShadowPos;
static float                                     s_spellShadowSize;
static HTEXTURE                                  s_spellShadowTexture[2];
static TSHashTable<FADEOUTHASHOBJ, CHashKeyGUID> s_fadeOutModelTable;

static int CheckFadeOutModels(const char *command, const char *arguments) {
  int          count = 0;
  unsigned int currentTime = OsGetAsyncTimeMs();
  ITERATELIST(FADEOUTHASHOBJ, s_fadeOutModelTable, fade) {
    ConsolePrintf("Model %02d: %d ms elapsed\n", ++count, currentTime - fade->startTime);
  }
  ConsolePrintf("Found %d models, nuking. If this improves Anim let Jeff know", count);
  s_fadeOutModelTable.Clear();
  return 1;
}

void RenderFadeOutModels(const NTempest::C3Vector cameraPos, const NTempest::C3Vector cameraTarg) {
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

void DrawCursorShadow() {
  unsigned int cursor = Spell_C_WorldObjectCursor();
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

CGWorldFrame *CGWorldFrame::s_currentWorldFrame;

int CGWorldFrame::IsUnitLegalSelection(const CGUnit_C *unit, unsigned int hitFilter) {
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
    if (unit->GetUnitData()->health <= 0) {
      return (hitFilter & 0x200000) != 0;
    }
    return (hitFilter & 0x100000) != 0;
  }
  return 1;
}

CGWorldFrame::~CGWorldFrame() {
  ConsoleCommandUnregister("debugobjectpathing");
  ConsoleCommandUnregister("SeeIfWorldFrameSucks");
  EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
  s_currentWorldFrame = 0;

  for (unsigned int i = 0; i < 2; ++i) {
    if (s_spellShadowTexture[i]) {
      HandleClose(s_spellShadowTexture[i]);
    }
    s_spellShadowTexture[i] = 0;
  }
  WorldTextShutdown();
  SmartScreenRectShutdown();

  CModelRecord *record;
  while ((record = m_models.Head()) != 0) {
    DEL(record);
  }
  while ((record = m_filteredModels.Head()) != 0) {
    DEL(record);
  }
  while ((record = m_freeModels.Head()) != 0) {
    DEL(record);
  }

  s_fadeOutModelTable.Clear();

  if (m_camera) {
    DEL(m_camera);
  }
  m_camera = 0;

  GxMasterEnableSet(GxMasterEnable_ClearOnPresent, 1);
}

int CGWorldFrame::IsLegalSelection(CModelRecord *record, unsigned int hitFilter) {
  CGObject_C *object = ClntObjMgrObjectPtr(record->guid, __FILE__, __LINE__);
  FATALASSERT(object);

  switch (object->GetType()) {
    case 9:
      return (hitFilter & 4) && IsUnitLegalSelection(static_cast<CGUnit_C *>(object), hitFilter);
    case 25:
      if (!(hitFilter & 8) || (!(hitFilter & 0x10) && object->GetGUID() == ClntObjMgrGetActivePlayer())) {
        return 0;
      }
      return IsUnitLegalSelection(static_cast<CGUnit_C *>(object), hitFilter);
    case 33:
      if (!(hitFilter & 2)) {
        return 0;
      }
      return !Spell_C_IsTargeting() ||
             static_cast<CGGameObject_C *>(object)->IsValidTargetForSpell(Spell_C_GetCurrentCaster(), Spell_C_GetTargettingSpell());
    case 129:
      return (hitFilter & 4) && !Spell_C_IsTargeting();
  }
  return 0;
}

unsigned int CGWorldFrame::SphereTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector, unsigned int hitFilter) {
  NTempest::C34Matrix camRelativeMatrix;
  NTempest::C34Matrix worldMatrix;
  NTempest::C3Vector &cameraPos = m_camera->Position();
  unsigned int        numHit = 0;

  ModelSceneCalcFrustumPlanes();
  for (CModelRecord *record = m_models.Head(); record;) {
    CModelRecord *next = record->Next();
    CGObject_C   *object = ClntObjMgrObjectPtr(record->guid, __FILE__, __LINE__);
    if (!object || !object->IsSolidSelectable()) {
      MoveToFreeList(record);
      record = next;
      continue;
    }

    object->GetWorldMatrix(&worldMatrix);
    camRelativeMatrix = worldMatrix;
    camRelativeMatrix.d0 -= cameraPos.x;
    camRelativeMatrix.d1 -= cameraPos.y;
    camRelativeMatrix.d2 -= cameraPos.z;
    if (!ModelTestSphere(record->model, camRelativeMatrix, record->scale, 1) ||
        !ModelHitTestSphere(record->model, record->scale, aVector, bVector, 1, &record->distance))
    {
      MoveToFreeList(record);
    } else if (IsLegalSelection(record, hitFilter)) {
      ++numHit;
    } else {
      m_models.UnlinkNode(record);
      m_filteredModels.LinkNode(record, LIST_HEAD, 0);
    }
    record = next;
  }
  return numHit;
}

unsigned int CGWorldFrame::VolumeTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector) {
  unsigned int numHit = 0;
  for (CModelRecord *record = m_models.Head(); record;) {
    CModelRecord *next = record->Next();
    if (!ModelHasHitTestVolumes(record->model) || ModelHitTestVolumes(record->model, record->scale, aVector, bVector, 1, &record->distance)) {
      ++numHit;
    } else {
      MoveToFreeList(record);
    }
    record = next;
  }
  return numHit;
}

unsigned int CGWorldFrame::GeometryTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector) {
  unsigned int numHit = 0;
  for (CModelRecord *record = m_models.Head(); record;) {
    CModelRecord *next = record->Next();
    if (ModelHitTestGeometry(record->model, record->scale, aVector, bVector, 1, &record->distance)) {
      ++numHit;
    } else {
      MoveToFreeList(record);
    }
    record = next;
  }
  return numHit;
}

static int GetObjectSelectCategory(CGObject_C *object) {
  int type = object->GetType();
  if (type == 9 || type == 25) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(object);
    if (unit->GetUnitData()->health > 0) {
      return 2;
    }
    return unit->CanBeLooted(OsGetAsyncTimeMs()) != 0;
  }
  return type == 33 && static_cast<CGGameObject_C *>(object)->CanHighlight();
}

CModelRecord *CGWorldFrame::HigherPriorityModel(CModelRecord *a, CModelRecord *b) {
  CGObject_C *aObject = ClntObjMgrObjectPtr(a->guid, __FILE__, __LINE__);
  int         aCategory = GetObjectSelectCategory(aObject);
  CGObject_C *bObject = ClntObjMgrObjectPtr(b->guid, __FILE__, __LINE__);
  int         bCategory = GetObjectSelectCategory(bObject);
  if (aCategory > bCategory || (aCategory == bCategory && a->distance <= b->distance)) {
    MoveToFreeList(b);
    return a;
  }
  MoveToFreeList(a);
  return b;
}

void CGWorldFrame::ReduceToClosestModel() {
  CModelRecord *closest = m_models.Head();
  for (CModelRecord *record = closest ? closest->Next() : 0; record;) {
    CModelRecord *next = record->Next();
    closest = HigherPriorityModel(closest, record);
    record = next;
  }
}

void CGWorldFrame::HideObstructingModels(float maxDist) {
  unsigned __int64 fade = 0;
  ITERATELIST(CModelRecord, m_filteredModels, record) {
    if (record->guid == CGPlayer_C::GetActive()) {
      if (record->distance <= maxDist) {
        fade = record->guid;
      }
      break;
    }
  }
  SendUnitFadeEvent(fade);
}

unsigned __int64 CGWorldFrame::FindClosestModel(const NTempest::C3Vector &a, const NTempest::C3Vector &b, unsigned int hitFilter, float *hitDist) {
  NTempest::C3Vector cameraPos = m_camera->Position();
  NTempest::C3Vector aVector = a - cameraPos;
  NTempest::C3Vector bVector = b - cameraPos;

  if (!SphereTestModels(aVector, bVector, hitFilter)) {
    return 0;
  }

  unsigned int volumeResult = VolumeTestModels(aVector, bVector);
  if (!volumeResult) {
    return 0;
  }

  if (volumeResult != 1) {
    unsigned int geometryResult = GeometryTestModels(aVector, bVector);
    if (!geometryResult) {
      return 0;
    }

    if (geometryResult != 1) {
      ReduceToClosestModel();
    }
  }

  CModelRecord *picked = m_models.Head();
  FATALASSERT(picked);
  *hitDist = picked->distance;
  return picked->guid;
}

CGWorldFrame::HIT_TYPE CGWorldFrame::HitTest(const NTempest::C3Vector &a, const NTempest::C3Vector &b, unsigned int hitFilter, HitTestResult *hitTestResult) {
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

  float            objDist = 0.0f;
  unsigned __int64 object = 0;
  if (hitFilter & 0x1E) {
    object = FindClosestModel(a, b, hitFilter, &objDist);
  }

  if (!object && !terrainFound) {
    return HIT_NONE;
  }

  NTempest::C3Vector direction = b - a;
  direction.Normalize();

  if (!object || (terrainFound && terrDist < objDist)) {
    hitTestResult->object = 0;
    hitTestResult->point = a + direction * terrDist;
    hitTestResult->distance = terrDist;
    return HIT_GROUND;
  }

  hitTestResult->object = object;
  hitTestResult->point = a + direction * objDist;
  hitTestResult->distance = objDist;
  return HIT_OBJECT;
}

unsigned int CGWorldFrame::GetHitTestFilterFlags() const {
  if (!Spell_C_IsTargeting()) {
    return ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__) ? 0xE : 0;
  }

  unsigned int hitFilter = 0;
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

  FATALASSERT(Spell_C_CanTargetItems() || Spell_C_WaitingForStringInput() || hitFilter);
  return hitFilter;
}

CGWorldFrame::HIT_TYPE CGWorldFrame::HitTestPoint(float x, float y, HitTestResult *hitTestResult) {
  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  m_camera->SetupWorldProjection(m_rect);

  HIT_TYPE hitType = HIT_NONE;
  unsigned int hitFilter = GetHitTestFilterFlags();
  NTempest::C3Vector a;
  NTempest::C3Vector b;
  if (hitFilter && GetLineSegment(x, y, &a, &b)) {
    hitType = HitTest(a, b, hitFilter, hitTestResult);
    if (hitType < HIT_OBJECT) {
      MoveToFreeList(&m_filteredModels);
    }
  }

  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
  return hitType;
}

int CGWorldFrame::GetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b) {
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

int ObjectEnumProc(void *param, unsigned long status, unsigned __int64 param64, unsigned long param32) {
  CGWorldFrame *worldFrame = static_cast<CGWorldFrame *>(param);
  FATALASSERT(worldFrame);

  CGObject_C *object = ClntObjMgrObjectPtr(param64, __FILE__, __LINE__);
  FATALASSERT(object);

  if ((param64 != CGPlayer_C::GetRealActivePlayer() || CGPlayer_C::GetRealActivePlayer() == ClntObjMgrGetActivePlayer()) && object &&
      !object->IsDisabled())
  {
    worldFrame->UpdateObject(object, status);
  }

  return 1;
}

int ObjectCollisionProc(unsigned __int64 param64, unsigned long param32, WorldObjCollisionHandlerData *data) {
  FATALASSERT(data);

  CGObject_C *object = ClntObjMgrObjectPtr(param64, __FILE__, __LINE__);
  FATALASSERT(object);
  if (!object->IsSolidCollidable()) {
    return 0;
  }

  if (!(object->GetType() & TYPE_GAMEOBJECT)) {
    return 0;
  }

  CGGameObject_C *gameObject = static_cast<CGGameObject_C *>(object);
  data->model = object->GetObjectModel();
  data->collideExt = gameObject->m_collideExtents;
  data->scale = object->GetScale() * object->GetRenderScale();

  data->matrix = NTempest::C44Matrix(object->GetMatrix());
  return 1;
}

void CGWorldFrame::UpdateObject(CGObject_C *object, unsigned long status) {
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

        NTempest::C3Vector cameraPos = m_camera->Position();
        NTempest::C3Vector target = cameraPos + m_camera->Forward();
        object->ObjectPostAnimate(camRelativeMatrix, cameraPos, target);
      }
    }
  }
}

void CGWorldFrame::AddModelToScene(CGObject_C *object, HMODEL model) {
  CModelRecord *record = m_freeModels.Head();
  if (record) {
    m_freeModels.UnlinkNode(record);
  } else {
    record = NEW(CModelRecord);
  }

  m_models.LinkNode(record, LIST_TAIL, 0);
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

void CGWorldFrame::MoveToFreeList(CModelRecord *record) {
  m_models.UnlinkNode(record);
  m_filteredModels.UnlinkNode(record);
  m_freeModels.LinkNode(record, LIST_HEAD, 0);
  HandleClose(record->model);
  record->model = 0;
}

void CGWorldFrame::MoveToFreeList(LISTPTR(CModelRecord) objList) {
  ITERATELISTPTR(CModelRecord, objList, record) {
    HandleClose(record->model);
    record->model = 0;
  }
  m_freeModels.Combine(objList, LIST_HEAD, 0);
}

void CGWorldFrame::GetCameraFacing(NTempest::C3Vector *position) {
  FATALASSERT(position);
  FATALASSERT(s_currentWorldFrame);
  FATALASSERT(s_currentWorldFrame->m_camera);

  *position = s_currentWorldFrame->m_camera->Forward();
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
      m_camera(0),
      m_updateTimeStamp(0),
      m_playerFadeMode(PLAYERFADE_NONE),
      m_playerAlpha(255),
      m_cameraAlpha(255),
      m_cameraAlphaChanged(0) {
  FATALASSERT(!s_currentWorldFrame || !"Error, why is there more than one world frame being created?");
  s_currentWorldFrame = this;

  SetAllPoints(m_top, 1);
  EnableEvent(SIMPLE_EVENT_KEY, static_cast<unsigned int>(-1));
  EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
  EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, static_cast<unsigned int>(-1));
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
  for (unsigned int i = 0; i < 2; ++i) {
    FATALASSERT(!s_spellShadowTexture[i]);
    s_spellShadowTexture[i] = TextureCreate(
        s_spellShadowName[i], CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0
    );
    SysMsgAdd(status, 1);
  }

  SmartScreenRectInitialize();
  s_spellShadowStyle = SPELL_GOOD;
  s_fadeOutModelTable.Clear();
  ConsoleCommandRegister("SeeIfWorldFrameSucks", CheckFadeOutModels, DEBUG, 0);
  GxMasterEnableSet(GxMasterEnable_ClearOnPresent, 0);
}

int CGWorldFrame::OnLayerTrackUpdate(const CMouseEvent &evt) {
  int result = CSimpleFrame::OnLayerTrackUpdate(evt);
  if (result) {
    CGInputControl::GetActive();
    return 1;
  }
  return result;
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

int CGWorldFrame::OnLayerKeyDown(CKeyEvent &evt) {
  if (CSimpleFrame::OnLayerKeyDown(evt)) {
    return 1;
  }
  if (evt.key < KEY_LAST && CGUIBindings::KeyEventToString(evt, m_lastKey[evt.key], sizeof(m_lastKey[evt.key]))) {
    return CGUIBindings::GetActive()->ExecKey(m_lastKey[evt.key], evt.time, 1);
  }
  return 0;
}

int CGWorldFrame::OnLayerKeyUp(CKeyEvent &evt) {
  if (CSimpleFrame::OnLayerKeyUp(evt)) {
    return 1;
  }
  if (evt.key >= KEY_LAST) {
    return 0;
  }

  unsigned long processTime = evt.time;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && (player->m_flags & 0x200) && static_cast<long>(evt.time - player->m_animEndTime) < 0) {
    processTime = player->m_animEndTime;
  }

  char *key = m_lastKey[evt.key];
  int result = 0;
  if (*key || (CGUIBindings::KeyEventToString(evt, key, sizeof(m_lastKey[evt.key])), *key)) {
    result = CGUIBindings::GetActive()->ExecKey(key, processTime, 0);
    *key = 0;
  }
  return result;
}

int CGWorldFrame::OnLayerMouseDown(CMouseEvent &evt) {
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

int CGWorldFrame::OnLayerMouseUp(CMouseEvent &evt) {
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

int CGWorldFrame::OnLayerMouseWheel(CMouseEvent &evt) {
  if (CSimpleFrame::OnLayerMouseWheel(evt)) {
    return 1;
  }
  char keyName[32];
  if (CGUIBindings::MouseEventToString(evt, keyName, sizeof(keyName))) {
    return CGUIBindings::GetActive()->ExecKey(keyName, evt.time, 1);
  }
  return 0;
}

int CGWorldFrame::OnLayerMouseMoveRelative(CMouseEvent &evt) {
  CGInputControl::GetActive()->OnMouseMoveRel(evt);
  return 1;
}

unsigned __int64 CGWorldFrame::GetObjectUnderMouse() {
  CModelRecord *record = m_models.Head();
  return record ? record->guid : 0;
}

float CGWorldFrame::GetSkyProgress() {
  return (g_clientGameTime.GetHourAndMinutes() + 720) % 1440 * 0.00069444446f * m_skyAnimDuration;
}

int CGWorldFrame::TogglePlayerRender() {
  m_renderPlayer = !m_renderPlayer;
  return m_renderPlayer;
}

int CGWorldFrame::SetPlayerRender(int state) {
  int oldState = m_renderPlayer;
  m_renderPlayer = state != 0;
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

void CGWorldFrame::SetSpriteClickButtons(unsigned int buttons) {
  m_spriteButtons = buttons;
}

void CGWorldFrame::SetTerrainClickButtons(unsigned int buttons) {
  m_terrainButtons = buttons;
}

int CGWorldFrame::PerformDefaultAction(MOUSEBUTTON button, unsigned int timestamp) {
  NTempest::C2Vector mousePos;
  NDCToDDC(m_top->m_mousePosition.x, m_top->m_mousePosition.y, &mousePos.x, &mousePos.y);

  HitTestResult hitTestResult;
  HIT_TYPE      hitType = HitTestPoint(mousePos.x, mousePos.y, &hitTestResult);
  if (hitType == HIT_GROUND) {
    CTerrainClickEvent terrainClickEvent;
    terrainClickEvent.point = hitTestResult.point;
    terrainClickEvent.button = button;
    return CGGameUI::HandleTerrainClick(terrainClickEvent);
  }

  if (hitType == HIT_OBJECT) {
    CSpriteClickEvent spriteClickEvent;
    spriteClickEvent.objectGUID = hitTestResult.object;
    spriteClickEvent.button = button;
    spriteClickEvent.time = timestamp;
    spriteClickEvent.pos = mousePos;
    return CGGameUI::HandleSpriteClick(spriteClickEvent);
  }

  CWorldClickEvent worldClickEvent;
  worldClickEvent.button = button;
  return CGGameUI::HandleWorldClick(worldClickEvent);
}

int CGWorldFrame::SendUnitFadeEvent(unsigned __int64 guid) {
  if (guid == m_lastUnitFade) {
    return 0;
  }

  HandleUnitFade(0, guid != 0);
  m_lastUnitFade = guid;
  return 1;
}

int CGWorldFrame::SendObjectTrackEvent(unsigned __int64 guid, float x, float y) {
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
      s_spellShadowSize = Spell_C_GetSpellRadius();
      if (s_spellShadowSize > 20.0f) {
        s_spellShadowSize = 20.0f;
      }
      CursorModelSetSequence(CAST_CURSOR);
    } else {
      s_spellShadowStyle = SPELL_BAD;
      s_spellShadowSize = 0.0f;
      CursorModelSetSequence(CAST_ERROR_CURSOR);
    }

    unsigned int cursor = Spell_C_WorldObjectCursor();
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
  if (!player) {
    CursorResetCursor(0);
    return;
  }

  if (player->m_flags & 0x800) {
    CGUnit_C *possessed = player->GetPossessedUnit();
    if (unit->GetUnitData()->health > 0 && possessed && possessed->GetUnitData()->health > 0 && !(possessed->GetUnitData()->flags & 0x2000) &&
        possessed->CanAttack(unit))
    {
      CursorModelSetSequence(ATTACK_CURSOR);
    } else {
      CursorResetCursor(0);
    }
    return;
  }

  float sqMag = (player->GetPosition() - unit->GetPosition()).SquaredMag();
  if (player->CanInteract(unit) && unit->GetUnitData()->health > 0) {
    int          outOfRange = sqMag > 5.5555553436f * 5.5555553436f;
    unsigned int npcFlags = unit->GetUnitData()->npcFlags;
    if (npcFlags & 0x1) {
      CursorModelSetSequence(outOfRange ? PICKUP_ERROR_CURSOR : PICKUP_CURSOR);
    } else if ((npcFlags & 0x2) && unit->GetUnitData()->weaponMode != 0 && unit->GetUnitData()->weaponMode != 2) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else if (npcFlags & 0x4) {
      CursorModelSetSequence(outOfRange ? TAXI_ERROR_CURSOR : TAXI_CURSOR);
    } else if (npcFlags & 0x8) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else if (npcFlags & 0x10) {
      CursorModelSetSequence(outOfRange ? INTERACT_ERROR_CURSOR : INTERACT_CURSOR);
    } else if (npcFlags & 0x20) {
      CursorModelSetSequence(outOfRange ? BUY_ERROR_CURSOR : BUY_CURSOR);
    } else if (npcFlags & 0xC0) {
      CursorModelSetSequence(outOfRange ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
    } else {
      CursorResetCursor(0);
    }
    return;
  }

  if (unit->CanBeLooted(m_updateTimeStamp)) {
    CursorModelSetSequence(
        player->CanLoot(unit) || unit->GetGUID() == player->m_lootingUnit || unit->GetGUID() == player->GetUnitBeingLooted()
            ? PICKUP_CURSOR
            : PICKUP_ERROR_CURSOR
    );
  } else if (
      player->GetUnitData()->health > 0 && !(player->GetUnitData()->flags & 0x2000) && unit->GetUnitData()->health > 0 && player->CanAttack(unit)
  )
  {
    CursorModelSetSequence(sqMag <= 109.202499f ? ATTACK_CURSOR : ATTACK_ERROR_CURSOR);
  } else {
    CursorResetCursor(0);
  }
}

void CGWorldFrame::CursorTrackObject(CGGameObject_C *gameObject) {
  if (!gameObject->m_baseObj->CanChangeCursor()) {
    CursorResetCursor(0);
  } else if (gameObject->m_baseObj->CanUseNow(0)) {
    CursorModelSetSequence(INTERACT_CURSOR);
  } else {
    CursorModelSetSequence(INTERACT_ERROR_CURSOR);
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
    clickEvt.pos.x = 0.0f;
    clickEvt.pos.y = 0.0f;
    CursorModelSetSequence(Spell_C_HandleSpriteRay(clickEvt, true) ? CAST_CURSOR : CAST_ERROR_CURSOR);
    SendObjectTrackEvent(hitTestResult.object, x, y);
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGObject_C *object = ClntObjMgrObjectPtr(hitTestResult.object, __FILE__, __LINE__);
  if (!player || !object || !object->CanHighlight()) {
    CursorResetCursor(0);
    SendObjectTrackEvent(0, 0.0f, 0.0f);
    return;
  }

  switch (object->GetType()) {
    case HIER_TYPE_ITEM:
      CursorModelSetSequence(PICKUP_CURSOR);
      break;

    case HIER_TYPE_UNIT:
    case HIER_TYPE_PLAYER:
      CursorTrackUnit(static_cast<CGUnit_C *>(object));
      break;

    case HIER_TYPE_GAMEOBJECT:
      CursorTrackObject(static_cast<CGGameObject_C *>(object));
      break;

    default:
      CursorResetCursor(0);
      break;
  }

  SendObjectTrackEvent(hitTestResult.object, x, y);
}

void CGWorldFrame::UpdateDayNightInfo(float elapsedSec) {
  DNInfo *dnInfo = DayNightGetInfo();
  dnInfo->farClip = m_camera->FarZ();
  dnInfo->elapsedSec = elapsedSec;
  dnInfo->nearClipScaled = m_camera->NearZ() * 3.0f;
  dnInfo->cameraPos = m_camera->Position();
  dnInfo->cameraDir = m_camera->Forward();
  dnInfo->cameraDir.Normalize();

  unsigned __int64 guid = ClntObjMgrGetActivePlayer();
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

void CGWorldFrame::SetPlayerFadeCameraValue(unsigned char value) {
  if (value != m_cameraAlpha) {
    if (m_camera->m_target == ClntObjMgrGetActivePlayer() && ((!value && m_cameraAlpha) || (value && !m_cameraAlpha))) {
      Player_C_SetPlayerRender(value != 0);
    }

    m_cameraAlpha = value;
    m_cameraAlphaChanged = 1;
  }
}

void CGWorldFrame::RefreshPlayerAlpha() {
  CGObject_C *object = ClntObjMgrObjectPtr(m_camera->m_target, __FILE__, __LINE__);
  if (object && (object->GetType() & TYPE_PLAYER)) {
    unsigned int alpha = m_cameraAlpha;
    if (alpha >= static_cast<unsigned int>(m_playerAlpha)) {
      alpha = m_playerAlpha;
    }
    object->SetMaxAlpha(alpha);
  }
}

void CGWorldFrame::UpdatePlayerAlpha(float elapsedSeconds) {
  if (m_cameraAlpha && (m_playerFadeMode || m_cameraAlphaChanged)) {
    if (m_playerFadeMode) {
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

        default:
          FATALASSERT(!"Error, unrecognized player fade mode!");
          break;
      }
    } else {
      m_cameraAlphaChanged = 0;
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

int UnitUpdateProc(unsigned __int64 guid, void *param) {
  CGWorldFrame *pWorldFrame = static_cast<CGWorldFrame *>(param);
  FATALASSERT(pWorldFrame);

  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object->GetType() & TYPE_UNIT) {
    static_cast<CGUnit_C *>(object)->UpdateDisplay(pWorldFrame->m_updateTimeStamp);
  }
  return 1;
}

void CGWorldFrame::UnitUpdate() {
  MoveToFreeList(&m_models);
  MoveToFreeList(&m_filteredModels);
  ClntObjMgrEnumVisibleObjects(UnitUpdateProc, this);
}

void CGWorldFrame::OnFrameRender(CRenderBatch *batch, unsigned int layer) {
  CSimpleFrame::OnFrameRender(batch, layer);
  if (!layer) {
    batch->QueueCallback(RenderWorld, this);
  }
}

void CGWorldFrame::RenderWorld(void *param) {
  CGWorldFrame       *worldFrame = static_cast<CGWorldFrame *>(param);
  NTempest::C44Matrix saved_view;
  NTempest::C44Matrix saved_proj;

  GxXformView(saved_view);
  GxXformProjection(saved_proj);
  worldFrame->OnWorldUpdate();
  worldFrame->OnWorldRender();
  PlayerNameRenderWorldText();
  GxXformSetView(saved_view);
  GxXformSetProjection(saved_proj);
}

NTempest::C2Vector CGWorldFrame::GetScreenCoordinates(const NTempest::C3Vector &point) {
  NTempest::C3Vector cameraPos = m_camera->m_position;
  NTempest::C4Vector position(point.x - cameraPos.x,
                             point.y - cameraPos.y,
                             point.z - cameraPos.z,
                             0.0f);
  position = position * m_worldMatrix;

  float inverseW = 1.0f / position.w;
  position.x = (position.x * inverseW + 1.0f) * 0.5f;
  position.y = (position.y * inverseW + 1.0f) * 0.5f;
  position.z = (position.z * inverseW + 1.0f) * 0.5f;
  position.w = (position.w * inverseW + 1.0f) * 0.5f;

  float screenx;
  float screeny;
  NDCToDDC(position.x, position.y, &screenx, &screeny);
  screenx = screenx > 0.0f ? (screenx < 0.8f ? screenx : 0.8f) : 0.0f;
  screeny = screeny > 0.0f ? (screeny < 0.6f ? screeny : 0.6f) : 0.0f;
  return NTempest::C2Vector(screenx, screeny);
}

NTempest::C2Vector CGWorldFrame::GetScreenCoordinates(
    const NTempest::C3Vector &point,
    const NTempest::C44Matrix &matrix,
    int clip,
    int worldPositionSpecified
) {
  NTempest::C4Vector position(point.x, point.y, point.z, 1.0f);
  if (worldPositionSpecified) {
    FATALASSERT(m_camera);
    position.x -= m_camera->m_position.x;
    position.y -= m_camera->m_position.y;
    position.z -= m_camera->m_position.z;
    position.w -= 1.0f;
  }

  position = position * matrix;
  float inverseW = 1.0f / position.w;
  position.x = (position.x * inverseW + 1.0f) * 0.5f;
  position.y = (position.y * inverseW + 1.0f) * 0.5f;
  position.z = (position.z * inverseW + 1.0f) * 0.5f;
  position.w = (position.w * inverseW + 1.0f) * 0.5f;

  if (!clip) {
    NDCToDDC(position.x, position.y, &position.x, &position.y);
    position.x = position.x > 0.0f ? (position.x < 0.8f ? position.x : 0.8f) : 0.0f;
    position.y = position.y > 0.0f ? (position.y < 0.6f ? position.y : 0.6f) : 0.0f;
  }
  return NTempest::C2Vector(position.x, position.y);
}

void CGWorldFrame::OnWorldUpdate() {
  NTempest::C3Vector  facing;
  NTempest::C44Matrix newMatrix;
  NTempest::C3Vector  target;
  NTempest::CRect     projection;
  float               elapsedSec = m_elapsedSec;
  NTempest::C3Vector  cameraPos;

  unsigned int idleTime = OsGetAsyncTimeMs() - m_top->m_eventTime;
  if (static_cast<int>(idleTime - AUTO_SIT_IDLE_TIME) >= 0) {
    if (static_cast<int>(idleTime - AUTO_LOGOUT_IDLE_TIME) < 0) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(CGPlayer_C::GetActive(), __FILE__, __LINE__));
      if (player && !player->GetUnitData()->standState) {
        player->ChangeStandState(1);
      }
    } else if (!ClientServices_CharacterLoggingOut()) {
      CGChat::AddChatMessage(FrameScript_GetText("IDLE_MESSAGE", -1, GENDER_NOT_APPLICABLE), static_cast<SLASH_COMMAND_ID>(9), 0, 0, 0, 0, 0);
      ClientServices_CharacterLogout(false);
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
  GetRect(&projection);

  m_camera->SetupWorldProjection(projection);

  cameraPos = m_camera->Position();
  target = cameraPos + m_camera->Forward();
  facing = m_camera->Forward();
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
    FATALASSERT(m_camera->m_target != CGPlayer_C::GetActive());
    object = ClntObjMgrObjectPtr(CGPlayer_C::GetActive(), __FILE__, __LINE__);
    FATALASSERT(object);
    CGPlayer_C *player = static_cast<CGPlayer_C *>(object);
    if (player->IsInFarSight()) {
      player->ClearFarSight();
    }
    m_camera->SetTarget(object);
  }

  CWorld::SetCameraTarget(object->GetWorldObject());
  CWorld::PrepareUpdate(cameraPos, target);
  CWorld::Update();
  ParticleSystemManager::GetInstance()->UpdateEmitters(elapsedSec, cameraPos, target);
  RibbonManager::GetInstance()->UpdateEmitters(elapsedSec, cameraPos, target);
  SpellVisualsTick(elapsedSec);

  GxXformViewProj(newMatrix);
  if (newMatrix != m_worldMatrix) {
    m_flags |= 3;
    m_worldMatrix = newMatrix;
  }
}

void CGWorldFrame::OnWorldRender() {
  unsigned int rsStackOffset = GxRsStackOffset();

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

void CGWorldFrame::SetNamePlateUpdate() {
  m_flags |= 1;
}

void CGWorldFrame::RegisterObjectFadeoutModel(CGObject_C *object, HTEXCOMPONENT texture, unsigned char startAlpha) {
  FATALASSERT(object);

  HMODEL model = object->GetObjectModel();
  if (!model) {
    return;
  }

  CHashKeyGUID key(object->GetGUID());
  unsigned int hash = static_cast<unsigned int>(object->GetGUID());
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

int CLayoutFrame::IsAttachmentOrigin() {
  return 0;
}

void CGWorldFrame::SetCameraTarget(CGObject_C *target) {
  FATALASSERT(m_camera);
  m_camera->SetTarget(target);
}
