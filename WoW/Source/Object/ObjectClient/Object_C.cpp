#include "Object_C.h"

#include "AnimCompiles.h"
#include "Corpse_C.h"
#include "DynamicObject_C.h"
#include "GameObject_C.h"
#include "Item_C.h"
#include "Player_C.h"
#include "Unit_C.h"

#include <Base/Handle.h>
#include <Base/Status.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Model/CollisionData.h>
#include <Os/OsTime.h>
#include <Os/W32/OSSystem.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <Tempest/cimvector.h>
#include <stpl.h>
#include <storm.h>

#include "Console/ConsoleCommand.h"

static const char *s_boneNames[26] = {"ArmL",          "ArmR",         "ShoulderL",     "ShoulderR",    "SpineLow",    "Waist",  "Head",
                                      "Jaw",           "IndexFingerR", "MiddleFingerR", "PinkyFingerR", "RingFingerR", "ThumbR", "IndexFingerL",
                                      "MiddleFingerL", "PinkyFingerL", "RingFingerL",   "ThumbL",       "$BTH",        "$CSR",   "$CSL",
                                      "_Breath",       "_Name",        "_NameMount",    "$CHD",         "$CCH"};

static const char *s_cameraNames[2] = {"Portrait", "Paperdoll"};
#include "Console/ConsoleVar.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WorldClient/World.h"
#include "UIUtil/Camera.h"
#include "Ui/GameUI.h"
#include "Ui/WorldFrame.h"
#include "DayNight.h"

static CVar    *s_objectSelectionCircle;
static CVar    *s_debugTargetPath;
static CGxTex  *s_fadeTex;
static HTEXTURE s_selectionTexture;

static const float Gx_MaxTexAspect = 8.0f;

static unsigned int GenerateAnimFlags(unsigned int objectFlags) {
  unsigned int animFlags = 0;

  if (objectFlags & 2) {
    animFlags = 1;
  }
  if (objectFlags & 4) {
    animFlags |= 2;
  }
  if (objectFlags & 8) {
    animFlags |= 4;
  }

  return animFlags;
}

static void s_BlobFadeTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  static TSGrowableArray<NTempest::CImVector> s_texels;

  if (cmd == GxTex_Lock) {
    unsigned int count = w * h;
    if (count > s_texels.Count()) {
      s_texels.SetCount(count);
    }
  } else if (cmd == GxTex_Latch) {
    if (mipLevel) {
      return;
    }

    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    texels = s_texels.Ptr();

    for (unsigned int row = 0; row < h; ++row) {
      NTempest::CImVector *tex = s_texels.Ptr() + row * w;

      for (unsigned int column = 0; column < w; ++column) {
        float position = static_cast<float>(column) / static_cast<float>(w - 1) * 12.0f;
        float alpha;

        if (position < 2.0f) {
          alpha = position * 0.5f;
        } else if (position < 10.0f) {
          alpha = 1.0f;
        } else {
          alpha = (12.0f - position) * 0.5f;
          if (alpha <= 0.0f) {
            alpha = 0.0f;
          }
        }

        tex[column].Set(static_cast<unsigned char>(alpha * 255.0f), 255, 255, 255);
      }
    }
  } else if (cmd == GxTex_Unlock) {
    s_texels.Clear();
  }
}

static void s_BlobFadeTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
);

HMODEL ObjectModelCreate(const char *filename, OBJECT_TYPE objectType, unsigned int mdlCreateFlags) {
  FATALASSERT(filename);

  CModelCreate  createData;
  CStatus       status;
  unsigned long processorFeatures = OsGetProcessorFeatures();

  memset(&createData, 0, sizeof(createData));
  createData.flags = mdlCreateFlags | 0x202A;
  if (static_cast<long>(processorFeatures) < 0) {
    unsigned int processorClass = 0;
    while (processorFeatures >>= 1) {
      ++processorClass;
    }
    createData.flags |= (processorClass << 17) | 0x10000;
  } else if (processorFeatures & 0x800000) {
    createData.flags |= 0x1000;
  }

  if (objectType & TYPE_GAMEOBJECT) {
    createData.sequenceNames = &g_animationNames[FIRST_GAMEOBJECTANIMATION];
    createData.numSequences = NUM_GAMEOBJECTANIMATIONS;
  } else if (objectType & TYPE_DYNAMICOBJECT) {
    createData.sequenceNames = &g_animationNames[FIRST_EFFECTANIMATION];
    createData.numSequences = NUM_EFFECTANIMATIONS;
  } else {
    createData.flags |= 0x44;
    createData.sequenceNames = g_animationNames;
    createData.numSequences = NUM_OBJECTANIMATIONS;
    createData.boneNames = s_boneNames;
    createData.numBones = 26;
    createData.cameraNames = s_cameraNames;
    createData.numCameras = 2;
  }

  HMODEL model = ModelCreate(filename, &createData, &status);
  FATALASSERT(model);
  return model;
}

void CGObject_C::SetTypeID(OBJECT_TYPE_ID typeID) {
  switch (typeID) {
    case ID_OBJECT:
      m_obj->m_type = HIER_TYPE_OBJECT;
      break;
    case ID_ITEM:
      m_obj->m_type = HIER_TYPE_ITEM;
      break;
    case ID_CONTAINER:
      m_obj->m_type = HIER_TYPE_CONTAINER;
      break;
    case ID_UNIT:
      m_obj->m_type = HIER_TYPE_UNIT;
      break;
    case ID_PLAYER:
      m_obj->m_type = HIER_TYPE_PLAYER;
      break;
    case ID_GAMEOBJECT:
      m_obj->m_type = HIER_TYPE_GAMEOBJECT;
      break;
    case ID_DYNAMICOBJECT:
      m_obj->m_type = HIER_TYPE_DYNAMICOBJECT;
      break;
    case ID_CORPSE:
      m_obj->m_type = HIER_TYPE_CORPSE;
      break;
    default:
      FATALASSERT(0);
      break;
  }
}

int CGObject_C::ObjectModelSetSequence(HMODEL__ *model, unsigned int sequence, unsigned int flags, const char *modelName) {
  if (sequence > NUM_OBJECTANIMATIONS) {
    SysMsgPrintf(SYSMSG_ERROR, 8, "BADOBJECTANIMMODEL|%d|%s", sequence, modelName);
    return 0;
  }

  if (!(m_flags & 1)) {
    flags |= 8;
  }

  unsigned int animFlags = GenerateAnimFlags(flags);
  int          result = flags & 1 ? ModelSetSequence(model, sequence, animFlags) : ModelSetRandomSequenceFidget(model, sequence, animFlags);
  if (result) {
    return 1;
  }

  switch (SErrGetLastError()) {
    case 1:
      ReportMissingAnimation(sequence, modelName);
      break;
    case 3:
      ReportNoAnimation(modelName);
      break;
  }
  return 0;
}

int CGObject_C::ObjectModelSetBoneSequence(HMODEL__ *model, unsigned int sequence, unsigned int objectID, unsigned int flags) {
  if (sequence > 135) {
    SysMsgPrintf(SYSMSG_ERROR, 8, "BADOBJECTANIM|%d", sequence);
    return 0;
  }

  if (!(m_flags & 1)) {
    flags |= 8;
  }

  unsigned int animFlags = GenerateAnimFlags(flags);
  int          result;
  if (flags & 1) {
    result = ModelSetSequence(model, sequence, objectID, animFlags);
  } else {
    result = ModelSetRandomSequenceFidget(model, sequence, objectID, animFlags);
  }

  if (result) {
    return 1;
  }

  switch (SErrGetLastError()) {
    case 1:
      ReportMissingAnimation(sequence, 0);
      break;
    case 2:
      ReportMissingBone(objectID, 0);
      break;
    case 3:
      ReportNoAnimation(0);
      break;
  }

  return 0;
}

int CGObject_C::InitModelFileName(char *modelFileName, unsigned int size) {
  FATALASSERT(modelFileName);

  const char *name = 0;
  switch (GetType()) {
    case HIER_TYPE_ITEM:
    case HIER_TYPE_CONTAINER:
      name = static_cast<CGItem_C *>(this)->GetModelFileName();
      if (name) {
        SStrPrintf(modelFileName, size, "%s\\%s", "Item\\GroundObjects", name);
      }
      return modelFileName[0] != 0;

    case HIER_TYPE_UNIT:
      name = static_cast<CGUnit_C *>(this)->CGUnit_C::GetModelFileName();
      break;

    case HIER_TYPE_PLAYER:
      name = static_cast<CGPlayer_C *>(this)->CGPlayer_C::GetModelFileName();
      break;

    case HIER_TYPE_GAMEOBJECT:
      name = static_cast<CGGameObject_C *>(this)->CGGameObject_C::GetModelFileName();
      break;

    case HIER_TYPE_DYNAMICOBJECT:
      name = static_cast<CGDynamicObject_C *>(this)->CGDynamicObject_C::GetModelFileName();
      if (name) {
        SStrPrintf(modelFileName, size, "%s", name);
      }
      return modelFileName[0] != 0;

    case HIER_TYPE_CORPSE:
      name = static_cast<CGCorpse_C *>(this)->CGCorpse_C::GetModelFileName();
      break;

    default:
      break;
  }

  if (name) {
    SStrCopy(modelFileName, name, size);
  } else {
    modelFileName[0] = 0;
  }
  return modelFileName[0] != 0;
}

void CGObject_C::PostMovementUpdate() {
}

void CGObject_C::SetStorage(unsigned long *storage) {
  m_data = storage;
  m_obj = reinterpret_cast<CGObjectData *>(storage);
}

CGObject_C::~CGObject_C() {
  RemoveWorldObject();
  if (m_model) {
    HandleClose(m_model);
  }
}

CGObject_C::CGObject_C(unsigned long *storage, unsigned long, CClientObjCreate *)
    : m_renderScale(1.0f),
      m_model(0),
      m_highlightTypes(0),
      m_objectHeight(1.0f),
      m_worldObject(0),
      m_flags(0),
      m_fadeStartTime(0),
      m_fadeDuration(0),
      m_alpha(0),
      m_startAlpha(0),
      m_endAlpha(0),
      m_maxAlpha(255) {
  SetStorage(storage);

  char modelFileName[260] = {0};
  if (!InitModelFileName(modelFileName, sizeof(modelFileName))) {
    return;
  }

  unsigned int createFlags = 0;
  if (GetType() == HIER_TYPE_UNIT) {
    createFlags = 0x200;
  } else if (GetType() == HIER_TYPE_PLAYER) {
    createFlags = 0x100800;
  } else if (GetType() != HIER_TYPE_GAMEOBJECT) {
    createFlags = 0x200;
  }

  m_model = ObjectModelCreate(modelFileName, GetType(), createFlags);
  ObjectModelSetSequence(m_model, 0, 0, modelFileName);
}

void CGObject_C::PostInit(const CClientObjCreate &init) {
  m_flags |= 8;
  m_renderScale = 1.0f;
  DoFade(255, ShouldFadeIn() ? 2000 : 0);
}

void CGObject_C::AddWorldObject() {
  if (ClntObjMgrGetPlayerType()) {
    return;
  }

  if (m_worldObject) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "OBJECTALREADYACTIVE|0x%016I64X", GetGUID());
    return;
  }

  unsigned int flags = 0;
  if (GetType() & TYPE_GAMEOBJECT) {
    flags = 3;
  }
  if (GetType() & TYPE_DYNAMICOBJECT) {
    flags |= 2;
  }

  m_worldObject = CWorld::AddObject(GetGUID(), 0, m_model, flags);
  UpdateWorldObject();

  if (m_model) {
    ModelSetLightSelectCallback(m_model, CWorld::SelectLight, reinterpret_cast<void *>(m_worldObject), 1);
  }
}

void CGObject_C::UpdateWorldObject() {
  if (ClntObjMgrGetPlayerType() || !m_worldObject) {
    return;
  }

  NTempest::C44Matrix tempMat;
  NTempest::CAaBox    extents;
  memset(&extents, 0, sizeof(extents));
  if (m_model) {
    ModelGetExtents(m_model, &extents);
  }

  tempMat.Translate(GetPosition());

  float facing;
  if (GetType() & TYPE_UNIT) {
    facing = static_cast<CGUnit_C *>(this)->GetDisplayFacing();
  } else {
    facing = GetFacing();
  }

  tempMat.Rotate(facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
  tempMat.Scale(GetScale());
  CWorld::UpdateObject(m_worldObject, tempMat, extents);
}

void CGObject_C::RemoveWorldObject() {
  if (ClntObjMgrGetPlayerType()) {
    return;
  }

  if (m_worldObject) {
    CWorld::RemoveObject(m_worldObject);
    m_worldObject = 0;

    if (m_model) {
      ModelSetLightSelectCallback(m_model, CWorld::SelectLight, 0, 1);
    }
  }
}

int CGObject_C::SetBlock(unsigned int i, unsigned long data) {
  unsigned int blocks;

  switch (GetType()) {
    case HIER_TYPE_OBJECT:
      blocks = 6;
      break;
    case HIER_TYPE_ITEM:
    case HIER_TYPE_CORPSE:
      blocks = 36;
      break;
    case HIER_TYPE_CONTAINER:
      blocks = 78;
      break;
    case HIER_TYPE_UNIT:
      blocks = 184;
      break;
    case HIER_TYPE_PLAYER:
      blocks = 634;
      break;
    case HIER_TYPE_GAMEOBJECT:
      blocks = 20;
      break;
    case HIER_TYPE_DYNAMICOBJECT:
      blocks = 16;
      break;
    default:
      FATALASSERT(0);
      return 0;
  }

  FATALASSERT(i < blocks);
  m_data[i] = data;
  return 1;
}

void CGObject_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes <= sizeof(CGObjectData));
  memcpy(m_obj, data, bytes);
}

unsigned int CGObject_C::OffsetOf(OBJECT_TYPE_ID type) {
  FATALASSERT(type == ID_OBJECT);
  return 0;
}

void CGObject_C::ReportMissingAnimation(unsigned int sequence, const char *modelName) const {
  if (!modelName) {
    modelName = GetModelFileName();
  }
  if (!modelName) {
    modelName = "<unknown model>";
  }

  SysMsgPrintf(SYSMSG_WARNING, 8, "MODELMISSINGANIM|%s|%s|%d", modelName, g_animationNames[sequence], sequence);
}

void CGObject_C::ReportMissingAnimObj(const char *message, unsigned int objectID, const char *modelName) const {
  if (!modelName) {
    modelName = GetModelFileName();
  }
  if (!modelName) {
    modelName = "<unknown model>";
  }

  SysMsgPrintf(SYSMSG_WARNING, 8, "%s|%s|%s|%d", message, modelName, s_boneNames[objectID], objectID);
}

void CGObject_C::ReportMissingAttachment(unsigned int objectID, const char *modelName) const {
  ReportMissingAnimObj("MODELMISSINGATTACHMENT", objectID, modelName);
}

void CGObject_C::ReportMissingEventObject(unsigned int objectID, const char *modelName) const {
  ReportMissingAnimObj("MODELMISSINGEVENTOBJ", objectID, modelName);
}

void CGObject_C::ReportMissingBone(unsigned int objectID, const char *modelName) const {
  ReportMissingAnimObj("MODELMISSINGBONE", objectID, modelName);
}

void CGObject_C::ReportNoAnimation(const char *modelName) {
  if (!modelName) {
    modelName = GetModelFileName();
  }
  if (!modelName) {
    modelName = "<unknown model>";
  }

  SysMsgPrintf(SYSMSG_ERROR, 8, "BADOBJECTNOANIM|%s", modelName);
}

ANIMENUMERATION Object_C_GetAnimIndex(const char* animName) {
  if (!animName || !*animName) {
    return static_cast<ANIMENUMERATION>(-1);
  }
  for (unsigned int i = 0; i < FIRST_ITEMANIMATION + NUM_ITEMANIMATIONS; ++i) {
    if (!SStrCmp(animName, g_animationNames[i], 0x7FFFFFFF)) {
      return static_cast<ANIMENUMERATION>(i);
    }
  }
  return static_cast<ANIMENUMERATION>(-1);
}

void CGObject_C::HideHighlightType(HIGHLIGHTTYPE type) {
  FATALASSERT(type < NUM_HIGHLIGHTTYPES);

  m_highlightTypes &= ~(1 << type);
  if (!m_highlightTypes) {
    ModelSetEmissiveColor(m_model, NTempest::CImVector(0ul), 1);
  }
}

CGBag_C *CGObject_C::GetBag() {
  return 0;
}

float CGObject_C::GetScale() const {
  return m_obj->m_scale;
}

NTempest::C3Vector CGObject_C::GetGroundNormal() const {
  return NTempest::C3Vector(0.0f, 0.0f, 1.0f);
}

void CGObject_C::ShowHighlightType(HIGHLIGHTTYPE type) {
  FATALASSERT(type < NUM_HIGHLIGHTTYPES);

  m_highlightTypes |= 1 << type;
  ModelSetEmissiveColor(
      m_model,
      *reinterpret_cast<NTempest::CImVector *>(&DayNightGetInfo()->unitSelect),
      1
  );
}

const char *CGObject_C::GetModelFileName() const {
  return 0;
}

int CGObject_C::GetSelectionHighlightColor(NTempest::CImVector *outPtr) const {
  FATALASSERT(outPtr);
  outPtr->Set(0xFFFFFFFF);
  return 1;
}

void CGObject_C::RenderTargetSelection() const {
}

int CGObject_C::UpdateTexComponentLoadStatus() {
  return 0;
}

void CGObject_C::PreRender(int currentTime, float elapsed) {
}

void CGObject_C::SetAnimated(int animated) {
  if (animated) {
    m_flags |= 1;
  } else {
    m_flags &= ~1U;
  }
}

void CGObject_C::PostAnimate(CGWorldFrame *worldFrame) {
}

void CGObject_C::ObjectPostAnimate(float renderFacing, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
}

void CGObject_C::ObjectPostAnimate(const NTempest::C34Matrix &matrix, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
}

void CGObject_C::OnSpecialMountAnim() {
}

int CGObject_C::IsSolidSelectable() const {
  return 1;
}

int CGObject_C::IsSolidCollidable() const {
  return 1;
}

int CGObject_C::CanHighlight() const {
  return 0;
}

int CGObject_C::CanBeTargetted() const {
  return 0;
}

int CGObject_C::FloatingTooltip() const {
  return 0;
}

void CGObject_C::OnLeftClick() {
}

void CGObject_C::OnRightClick() {
}

NTempest::C34Matrix CGObject_C::GetMatrix() const {
  return NTempest::C34Matrix();
}

int CGObject_C::ShouldFadeIn() const {
  return 1;
}

const char *CGObject_C::GetObjectName() const {
  return 0;
}

int CGObject_C::GetPageTextID(void(*)(int, const unsigned __int64 &, void *, bool)) const {
  return 0;
}

void CGObject_C::Disable(int shutdown) {
  m_highlightTypes = 0;
  SetAnimated(0);
  m_flags |= 2;
  m_endAlpha = 0;
  m_alpha = 0;
}

void CGObject_C::Reenable() {
  m_flags = (m_flags & ~2U) | 4U;
  DoFade(255, ShouldFadeIn() ? 2000 : 0);
}

int CGObject_C::ShouldRender(unsigned long worldStatus) {
  if (worldStatus & 1) {
    m_flags |= 0x10;
    return 1;
  }

  NTempest::C3Vector groundNormal(0.0f, 0.0f, 1.0f);
  ModelProcessEvents(m_model, GetPosition(), GetFacing(), groundNormal, 1.0f);
  m_flags &= ~0x10U;
  return 0;
}

void CGObject_C::ObjectSetNotRendering() {
  m_flags &= ~0x10U;
}

void CGObject_C::SetObjectModel(HMODEL__ *model) {
  RemoveWorldObject();
  m_model = model;
  if (model) {
    AddWorldObject();
    m_flags &= ~0x20u;
  }
}

int CGObject_C::AddAttachment(HMODEL__ *parent, unsigned int parentIndex, HMODEL__ *child, float scale) {
  FATALASSERT(m_model);

  if (ModelAddLink(parent, parentIndex, child, scale)) {
    m_flags &= ~0x40;
    return 1;
  }

  return 0;
}

void CGObject_C::Initialize() {
  s_objectSelectionCircle = CVar::Register("ObjectSelectionCircle", 0, 0, "1", 0, DEBUG, false, 0);
  s_debugTargetPath = CVar::Register("DebugTargetPath", 0, 0, "0", 0, DEBUG, false, 0);

  if (s_fadeTex) {
    GxTexDestroy(s_fadeTex);
  }

  static const unsigned int FADETEX_WIDTH = static_cast<unsigned int>(Gx_MaxTexAspect * 8.0f);
  static const unsigned int FADETEX_HEIGHT = 8;
  CGxTexFlags               flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  GxTexCreate(FADETEX_HEIGHT, FADETEX_WIDTH, GxTex_Argb8888, flags, 0, s_BlobFadeTex, s_fadeTex);

  if (s_selectionTexture) {
    HandleClose(s_selectionTexture);
  }

  CStatus status;
  s_selectionTexture = TextureCreate("Textures\\UnitSelectTexture.blp", flags, &status, 0);
}

void CGObject_C::Shutdown() {
  if (s_fadeTex) {
    GxTexDestroy(s_fadeTex);
  }
  s_fadeTex = 0;

  if (s_selectionTexture) {
    HandleClose(s_selectionTexture);
  }
  s_selectionTexture = 0;
}

void CGObject_C::PreAnimate(CGWorldFrame *worldFrame) {
  const unsigned __int64 lockedGUID = CGGameUI::GetLockedTarget();
  if (GetGUID() == lockedGUID && GetGUID() != ClntObjMgrGetActivePlayer() && s_objectSelectionCircle->GetInt()) {
    RenderTargetSelection();
  }

  if (s_debugTargetPath->GetInt()) {
    const unsigned __int64 guid = GetGUID();
    const unsigned __int64 activePlayer = ClntObjMgrGetActivePlayer();

    if (guid == lockedGUID) {
      static_cast<CGUnit_C *>(this)->RenderDebugPathing();
    } else if (guid == activePlayer) {
      CGUnit_C          *unit = static_cast<CGUnit_C *>(this);
      const CGUnitData  *unitData = unit->GetUnitData();
      const unsigned int unitFlags = unitData->flags;

      if (!(unitFlags & 0x01000000) &&
          (!(GetType() & TYPE_PLAYER) || unitData->charm || (!(unitFlags & 2) && (unitFlags & 0x00C00004)) || (unitFlags & 1)))
      {
        unit->RenderDebugPathing();
      }
    }
  }

  unsigned char alpha;
  if (!m_fadeStartTime) {
    alpha = m_endAlpha;
  } else {
    int elapsed = OsGetAsyncTimeMs() - m_fadeStartTime;
    if (elapsed > static_cast<int>(m_fadeDuration)) {
      m_fadeStartTime = 0;
      alpha = m_endAlpha;
    } else {
      if (elapsed < 0) {
        elapsed = 0;
      }
      float amount = static_cast<float>(m_startAlpha) + static_cast<float>(elapsed) / static_cast<float>(m_fadeDuration) *
                                                            (static_cast<float>(m_endAlpha) - static_cast<float>(m_startAlpha));
      if (amount < 0.0f) {
        amount = 0.0f;
      } else if (amount > 255.0f) {
        amount = 255.0f;
      }
      alpha = static_cast<unsigned char>(NTempest::CMath::fuint_n(amount));
    }
  }

  if (alpha > m_maxAlpha) {
    alpha = m_maxAlpha;
  }
  if (alpha != m_alpha) {
    ModelSetVertexAlpha(m_model, alpha, 1);
    m_alpha = alpha;
  }

  UpdateRenderFacing();
}

void CGObject_C::Animate() {
  if (ClntObjMgrGetPlayerType()) {
    return;
  }

  NTempest::C34Matrix worldMatrix;
  GetWorldMatrix(&worldMatrix);

  NTempest::C34Matrix camRelativeMatrix = worldMatrix;
  NTempest::C3Vector &cameraPosition = CGWorldFrame::GetActiveCamera()->Position();
  camRelativeMatrix.d0 -= cameraPosition.x;
  camRelativeMatrix.d1 -= cameraPosition.y;
  camRelativeMatrix.d2 -= cameraPosition.z;
  Animate(camRelativeMatrix);
}

void CGObject_C::Animate(const NTempest::C34Matrix &camRelativeMatrix) {
  FATALASSERT(m_model);

  CGCamera          *camera = CGWorldFrame::GetActiveCamera();
  NTempest::C3Vector cameraPosition = camera->Position();
  NTempest::C3Vector cameraVector = camera->Forward();
  ModelAnimate(m_model, camRelativeMatrix, GetScale() * m_renderScale, cameraPosition, cameraVector);
}

void CGObject_C::UpdateRenderFacing() {
}

int CGObject_C::IsObjectModelLoaded() {
  return m_flags & 0x20;
}

int CGObject_C::AreAttachmentsLoaded() const {
  return m_flags & 0x40;
}

int CGObject_C::UpdateAttachmentLoadStatus() {
  if ((m_flags & 0x40) || !m_model || !ModelIsLoaded(m_model, 1)) {
    return 0;
  }

  m_flags |= 0x40;
  return 1;
}

void CGObject_C::SetCircleRenderStates() const {
  if (!s_selectionTexture || !s_fadeTex) {
    return;
  }

  GxRsSet(GxRs_Blend, 3);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(s_selectionTexture, 1, 0));
  GxRsSet(GxRs_Texture1, s_fadeTex);
}

float CGObject_C::GetRenderFacing() const {
  return GetFacing();
}

int CGObject_C::UpdateModelLoadStatus() {
  int modelLoaded = m_model ? ModelIsLoaded(m_model, 0) : 0;
  if ((m_flags & 0x20) || !m_model || !modelLoaded) {
    return 0;
  }

  m_flags |= 0x20;
  UpdateObjectHeight(m_model);
  UpdateWorldObject();
  if (GetType() & TYPE_UNIT) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(this);
    if (!(unit->m_flags & 0x10)) {
      unit->UpdateUnitCollisionBox(m_model, GetModelFileName());
    }
  }
  return 1;
}

void CGObject_C::GetWorldMatrix(NTempest::C34Matrix *worldMatrix) const {
  float scale = GetScale() * m_renderScale;
  ModelGetStandingMatrix(m_model, GetPosition(), GetGroundNormal(), GetRenderFacing(), scale, worldMatrix);
}

void CGObject_C::UpdateObjectHeight(HMODEL__ *model) {
  NTempest::CAaBox extents;
  ModelGetExtents(model, &extents);
  m_objectHeight = extents.t.z - extents.b.z;
}

void CGObject_C::PostReenable() {
  m_flags &= ~4U;
}

int CGObject_C::IsPostInited() {
  return m_flags & 8;
}

const char *g_animationNames[] = {
    "Stand",
    "Death",
    "Spell",
    "Stop",
    "Walk",
    "Run",
    "Dead",
    "Rise",
    "StandWound",
    "CombatWound",
    "CombatCritical",
    "ShuffleLeft",
    "ShuffleRight",
    "Walkbackwards",
    "Stun",
    "HandsClosed",
    "AttackUnarmed",
    "Attack1H",
    "Attack2H",
    "Attack2HL",
    "ParryUnarmed",
    "Parry1H",
    "Parry2H",
    "Parry2HL",
    "ShieldBlock",
    "ReadyUnarmed",
    "Ready1H",
    "Ready2H",
    "Ready2HL",
    "ReadyBow",
    "Dodge",
    "SpellPrecast",
    "SpellCast",
    "SpellCastArea",
    "NPCWelcome",
    "NPCGoodbye",
    "Block",
    "JumpStart",
    "Jump",
    "JumpEnd",
    "Fall",
    "SwimIdle",
    "Swim",
    "SwimLeft",
    "SwimRight",
    "SwimBackwards",
    "AttackBow",
    "FireBow",
    "ReadyRifle",
    "AttackRifle",
    "Loot",
    "ReadySpellDirected",
    "ReadySpellOmni",
    "SpellCastDirected",
    "SpellCastOmni",
    "BattleRoar",
    "ReadyAbility",
    "Special1H",
    "Special2H",
    "ShieldBash",
    "EmoteTalk",
    "EmoteEat",
    "EmoteWork",
    "EmoteUseStanding",
    "EmoteTalkExclamation",
    "EmoteTalkQuestion",
    "EmoteBow",
    "EmoteWave",
    "EmoteCheer",
    "EmoteDance",
    "EmoteLaugh",
    "EmoteSleep",
    "EmoteSitGround",
    "EmoteRude",
    "EmoteRoar",
    "EmoteKneel",
    "EmoteKiss",
    "EmoteCry",
    "EmoteChicken",
    "EmoteBeg",
    "EmoteApplaud",
    "EmoteShout",
    "EmoteFlex",
    "EmoteShy",
    "EmotePoint",
    "Attack1HPierce",
    "Attack2HLoosePierce",
    "AttackOff",
    "AttackOffPierce",
    "Sheath",
    "HipSheath",
    "Mount",
    "RunRight",
    "RunLeft",
    "MountSpecial",
    "Kick",
    "SitGroundDown",
    "SitGround",
    "SitGroundUp",
    "SleepDown",
    "Sleep",
    "SleepUp",
    "SitChairLow",
    "SitChairMed",
    "SitChairHigh",
    "LoadBow",
    "LoadRifle",
    "AttackThrown",
    "ReadyThrown",
    "HoldBow",
    "HoldRifle",
    "HoldThrown",
    "LoadThrown",
    "EmoteSalute",
    "KneelStart",
    "KneelLoop",
    "KneelEnd",
    "AttackUnarmedOff",
    "SpecialUnarmed",
    "StealthWalk",
    "StealthStand",
    "Knockdown",
    "EatingLoop",
    "UseStandingLoop",
    "ChannelCastDirected",
    "ChannelCastOmni",
    "Whirlwind",
    "Birth",
    "UseStandingStart",
    "UseStandingEnd",
    "Howl",
    "Drown",
    "Drowned",
    "FishingCast",
    "FishingLoop",

    "Stand",
    "Closed",
    "Open",
    "Opened",
    "Close",
    "Destroy",
    "Destroyed",
    "Rebuild",
    "Custom0",
    "Custom1",
    "Custom2",
    "Custom3",

    "Stand",
    "Hold",
    "Decay",

    "Stand",
    "InFlight",
    "BowPull",
    "BowRelease"
};

int CGObject_C::ObjectIsRendering() {
  return m_flags & 0x10;
}

int CGObject_C::IsDisabled() {
  return m_flags & 2;
}

int CGObject_C::IsInReenable() {
  return m_flags & 4;
}

void CGObject_C::DoFade(unsigned char alpha, unsigned int fadeTimeMs) {
  if (alpha != m_endAlpha) {
    m_endAlpha = alpha;
    m_fadeStartTime = OsGetAsyncTimeMs();
    m_fadeDuration = fadeTimeMs;
    m_startAlpha = m_alpha;
  }
}

HMODEL__ *CGObject_C::GetCharacterModel(int *mounted) const {
  if (mounted) {
    *mounted = 0;
  }

  return static_cast<HMODEL>(HandleDuplicate(m_model));
}

unsigned int Object_C_AnimHasHitEvent(int anim) {
  FATALASSERT(anim < NUM_OBJECTANIMATIONS);
  return g_seqInformation[anim].flags & 1;
}
static int UpdateAllWorldObjectsCallback(unsigned __int64 obj, void *) {
  CGObject_C *object = ClntObjMgrObjectPtr(obj, __FILE__, __LINE__);
  if (object) {
    object->UpdateWorldObject();
  }
  return 1;
}

void CGObject_C::UpdateAllWorldObjects() {
  ClntObjMgrEnumVisibleObjects(UpdateAllWorldObjectsCallback, 0);
}
