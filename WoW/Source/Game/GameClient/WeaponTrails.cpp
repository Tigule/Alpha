#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include <Console/ConsoleCommand.h>
#include <Console/ConsoleVar.h>
#include <Client.h>
#include <Base/CDataAllocator.h>
#include <Base/Handle.h>
#include <Gx/Gx.h>
#include <Model/IModel.h>
#include <Tempest/c34matrix.h>
#include <Tempest/c33matrix.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c4quaternion.h>
#include <Tempest/caabox.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>
#include <Ui/WorldFrame.h>
#include <storm.h>
#include <stpl.h>

#include <math.h>

enum WEAPONSWINGTYPES {
  SWING_NORMAL,
  SWING_CRITICAL,
  NUM_SWINGTYPES
};

struct VERTEX {
  NTempest::C3Vector  v;
  NTempest::CImVector c;
};

NODEDECL(SWING) {
  SWING();
  ~SWING() {
    Recycle();
  }
  void Recycle();
  void Render() const;
  void AddVerts(
      const NTempest::C44Matrix &basisMatrix, const NTempest::C3Vector &bottom, const NTempest::C3Vector &top, const NTempest::CImVector &color,
      BYTE currentAlpha, const NTempest::C3Vector &cameraPos
  );

  TSGrowableArray<VERTEX> m_trail;
  TSGrowableArray<WORD>   m_vertexIndices;
  UINT                    m_flags;
  NTempest::C44Matrix     m_lastMatrix;
};

class WTOBJECT {
 public:
  LINKDECLEX(WTOBJECT, m_explicitLink);
  LISTDECL(SWING, m_swings);
  HMODEL              m_model;
  UINT                m_geosetID;
  NTempest::C3Vector  m_bottomCoord;
  NTempest::C3Vector  m_topCoord;
  NTempest::CImVector m_color;
  int                 m_fadeOutRate;
  UINT                m_flags;
  UINT                m_timer;
  int                 m_currentAlpha;

  WTOBJECT();
  ~WTOBJECT();
  void Recycle();
  void DisableDrawing();
  void SetDrawTrail(const NTempest::CImVector &color, int fadeOutRate, UINT duration);
  void SetColor(NTempest::CImVector color);
  void SetFadeOutRate(int fadeOutRate);
  void Render(const NTempest::C44Matrix &basis);
  void RenderVerts(const NTempest::C3Vector &cameraPos);
  void FadeVerts();
} *WTOBJECTPTR;

void ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void (*renderCallback)(HMODEL, const NTempest::C34Matrix &, LPVOID),
    LPVOID renderParam,
    UINT  *custGeosetId
);
void ModelCustGeosetRemove(HMODEL model, UINT custGeosetId);
BOOL ModelGetModelSpacePivot(HMODEL model, UINT objectId, NTempest::C3Vector *pivot);

static TInstanceAllocator<WTOBJECT> s_unusedObjects(100);
static TInstanceAllocator<SWING>    s_freeSwings(100);

static TSGrowableArray<VERTEX> s_vertexBuffer;
static TSGrowableArray<WORD>   s_freeVertexIndices;
static UINT                    STEPS_PER_180DEGS = 32;
static int                     ALPHAFADEOUTRATE = 24;
static int                     s_masterEnable = 1;
static CVar                   *s_consoleVarHandle;

static BOOL DiscontinueTimerHandler(LPCVOID data, LPVOID userArg);

SWING::SWING() {
  m_trail.SetChunkSize(128);
  m_vertexIndices.SetChunkSize(128);
}

void SWING::Recycle() {
  m_trail.SetCount(0);
  m_vertexIndices.SetCount(0);
  m_flags = 0;
}

void SWING::Render() const {
  if (m_vertexIndices.Count() < 3 || m_trail.Count() < 3) {
    return;
  }

  GxPrimLockVertexPtrs(m_trail.Count(), &m_trail[0].v, sizeof(VERTEX), 0, 0, &m_trail[0].c, sizeof(VERTEX), 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, m_vertexIndices.Count(), m_vertexIndices.Ptr());
  GxPrimUnlockVertexPtrs();
}

void SWING::AddVerts(
    const NTempest::C44Matrix &basisMatrix,
    const NTempest::C3Vector  &bottom,
    const NTempest::C3Vector  &top,
    const NTempest::CImVector &color,
    BYTE                       currentAlpha,
    const NTempest::C3Vector  &cameraPos
) {
  NTempest::C44Matrix cameraTranslate(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
  NTempest::C44Matrix basis = basisMatrix * cameraTranslate;

  if (m_flags & 1) {
    NTempest::C4Quaternion q1;
    NTempest::C4Quaternion q2;
    q1.FromRotationMatrix(m_lastMatrix);
    q2.FromRotationMatrix(basis);

    NTempest::C3Vector startingTranslation(m_lastMatrix.d0, m_lastMatrix.d1, m_lastMatrix.d2);
    UINT               steps = NTempest::CMath::fabs_(1.0f - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w)) * STEPS_PER_180DEGS;
    steps = max(steps, 1);

    float              t = 0.0f;
    float              tStep = 1.0f / steps;
    NTempest::C3Vector translationStep = (NTempest::C3Vector(basis.d0, basis.d1, basis.d2) - startingTranslation) / (float)steps;

    for (UINT step = 0; step < steps; ++step) {
      NTempest::C4Quaternion slerped = NTempest::C4Quaternion::Slerp(t, q1, q2);
      NTempest::C44Matrix    matrix(slerped);
      matrix.d0 = startingTranslation.x;
      matrix.d1 = startingTranslation.y;
      matrix.d2 = startingTranslation.z;

      NTempest::C3Vector newBottom = bottom * matrix;
      NTempest::C3Vector newTop = top * matrix;

      VERTEX newVerts[2];
      newVerts[0].v = newBottom;
      newVerts[0].c = color;
      newVerts[0].c.a = currentAlpha >> 1;
      newVerts[1].v = newTop;
      newVerts[1].c = color;
      newVerts[1].c.a = currentAlpha;

      UINT firstVertex = m_trail.Count();
      m_trail.Add(2, newVerts);
      m_vertexIndices.SetCount(firstVertex + 2);
      m_vertexIndices[firstVertex] = firstVertex;
      m_vertexIndices[firstVertex + 1] = firstVertex + 1;

      t += tStep;
      startingTranslation += translationStep;
    }
  } else {
    m_flags |= 1;
  }

  m_lastMatrix = basis;
}

static bool ToggleCallback(CVar *h, LPCSTR oldValue, LPCSTR newValue, LPVOID arg) {
  s_masterEnable = SStrToInt(newValue);
  return true;
}

WTOBJECT::~WTOBJECT() {
  if (m_model) {
    if (m_geosetID != -1) {
      ModelCustGeosetRemove(m_model, m_geosetID);
    }
    m_geosetID = -1;
    HandleClose(m_model);
  }

  ITERATELIST(SWING, m_swings, swing) {
    s_freeSwings.Put(swing);
  }
  if (m_timer) {
    ClientKillTimer(m_timer, DiscontinueTimerHandler, "DiscontinueTimerHandler");
  }
}

static void GeosetRenderFunction(HMODEL model, const NTempest::C34Matrix &basis, LPVOID param) {
  FATALASSERT(param);

  ((WTOBJECT *)param)->Render(basis);
}

static BOOL DiscontinueTimerHandler(LPCVOID data, LPVOID userArg) {
  WTOBJECT *trail = (WTOBJECT *)userArg;
  FATALASSERT(trail);

  trail->m_flags |= 2;
  trail->m_timer = 0;
  return 1;
}

WTOBJECT::WTOBJECT()
    : m_model(0), m_geosetID(-1), m_bottomCoord(0.0f), m_topCoord(0.0f), m_color(0ul), m_fadeOutRate(-1), m_flags(0), m_timer(0), m_currentAlpha(0) {
}

void WTOBJECT::Recycle() {
  m_flags = 0;
  ITERATELIST(SWING, m_swings, swing) {
    s_freeSwings.Put(swing);
  }
  if (m_geosetID != -1) {
    ModelCustGeosetRemove(m_model, m_geosetID);
  }
  m_geosetID = -1;
  if (m_model) {
    HandleClose(m_model);
  }
  m_model = 0;
}

void WTOBJECT::RenderVerts(const NTempest::C3Vector &cameraPos) {
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_DepthTest, 1);

  NTempest::C44Matrix world(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -cameraPos.x, -cameraPos.y, -cameraPos.z, 1.0f);
  GxXformPush(GxXform_World, world);

  SAFEITERATELIST(SWING, m_swings, swing) {
    swing->Render();
  }

  GxXformPop(GxXform_World);
  GxRsPop();
}

void WTOBJECT::Render(const NTempest::C44Matrix &basis) {
  if (!m_model) {
    return;
  }

  SWING *swing = m_swings.Head();
  if (m_flags & 2) {
    if (m_currentAlpha < ALPHAFADEOUTRATE) {
      m_flags &= ~3;
    } else {
      m_currentAlpha -= ALPHAFADEOUTRATE;
    }
  }

  NTempest::C3Vector cameraPos(0.0f);
  CGWorldFrame::GetCameraPosition(&cameraPos);

  if (s_masterEnable && swing && (m_flags & 1)) {
    swing->AddVerts(basis, m_bottomCoord, m_topCoord, m_color, m_currentAlpha, cameraPos);
  }

  RenderVerts(cameraPos);
  FadeVerts();
}

void WTOBJECT::FadeVerts() {
  ITERATELIST(SWING, m_swings, swing) {
    FATALASSERT(m_fadeOutRate < 0);

    int visible = 0;
    VERTEX *vertex = swing->m_trail.Ptr();
    for (UINT i = swing->m_trail.Count(); i; --i, ++vertex) {
      int alpha = vertex->c.a + m_fadeOutRate;
      if (alpha < 0) {
        vertex->c.a = 0;
      } else {
        vertex->c.a = alpha;
        visible = 1;
      }
    }

    if (swing->m_trail.Count() && !visible) {
      s_freeSwings.Put(swing);
    }
  }
}

void WTOBJECT::DisableDrawing() {
  m_flags &= ~1u;
}

void WTOBJECT::SetDrawTrail(const NTempest::CImVector &color, int fadeOutRate, UINT duration) {
  if (m_timer) {
    ClientKillTimer(m_timer, DiscontinueTimerHandler, "DiscontinueTimerHandler");
  }

  m_flags |= 1;
  m_timer = 0;
  m_color.Set(*color.IV_());
  m_currentAlpha = color.a;
  m_currentAlpha = max(m_currentAlpha, 0);
  m_fadeOutRate = fadeOutRate;
  if (m_fadeOutRate > 0) {
    m_fadeOutRate = -m_fadeOutRate;
  }

  SWING *swing = s_freeSwings.Get(0);
  m_swings.LinkNode(swing, LIST_HEAD, 0);

  m_timer = ClientSetTimer(duration, DiscontinueTimerHandler, this);
}

void WTOBJECT::SetColor(NTempest::CImVector color) {
  m_color.Set(*color.IV_());
}

void WTOBJECT::SetFadeOutRate(int fadeOutRate) {
  m_fadeOutRate = fadeOutRate < -1 ? fadeOutRate : -1;
}

void WeaponTrailsInitialize() {
  s_consoleVarHandle = CVar::Register("weapontrails", "Toggles weapon trails on or off", 0, "1", ToggleCallback, DEFAULT, false, 0);
}

void WeaponTrailsShutdown() {
  s_vertexBuffer.Clear();
  s_freeVertexIndices.Clear();
}

int WeaponTrailCreate(HMODEL model) {
  VALIDATEBEGIN;
  VALIDATE(model);
  VALIDATEEND;

  WTOBJECT *trail = s_unusedObjects.Get(0);

  trail->m_model = (HMODEL)HandleDuplicate(model);
  trail->m_fadeOutRate = -1;
  trail->m_color = NTempest::CImVector(-1);
  trail->m_geosetID = -1;

  ModelCustGeosetAdd(trail->m_model, NTempest::C3Vector(0.0f), GeosetRenderFunction, trail, &trail->m_geosetID);

  if (trail->m_geosetID == -1) {
    s_unusedObjects.Put(trail);
    return 0;
  }

  if (!ModelGetModelSpacePivot(trail->m_model, 0, &trail->m_bottomCoord) || !ModelGetModelSpacePivot(trail->m_model, 1, &trail->m_topCoord)) {
    NTempest::CAaBox e;
    ModelGetExtents(trail->m_model, &e);
    trail->m_bottomCoord = NTempest::C3Vector(e.b.x, (e.b.y + e.t.y) * 0.5f, (e.b.z + e.t.z) * 0.5f);
    trail->m_topCoord = NTempest::C3Vector(e.t.x, (e.b.y + e.t.y) * 0.5f, (e.b.z + e.t.z) * 0.5f);
  }

  return (int)trail;
}

void WeaponTrailClose(int trail) {
  VALIDATEBEGIN;
  VALIDATE(trail);
  VALIDATEENDVOID;

  WTOBJECT *object = (WTOBJECT *)trail;
  if (object->m_model && object->m_geosetID) {
    ModelCustGeosetRemove(object->m_model, object->m_geosetID);
  }
  s_unusedObjects.Put(object);
}

void WeaponTrailSetColor(int trail, NTempest::CImVector color) {
  VALIDATEBEGIN;
  VALIDATE(trail);
  VALIDATEENDVOID;
  ((WTOBJECT *)trail)->SetColor(color);
}

void WeaponTrailSetFadeOutRate(int trail, int fadeOutRate) {
  VALIDATEBEGIN;
  VALIDATE(trail);
  VALIDATEENDVOID;
  ((WTOBJECT *)trail)->SetFadeOutRate(fadeOutRate);
}

void WeaponTrailDisableDrawing(int trail) {
  VALIDATEBEGIN;
  VALIDATE(trail);
  VALIDATEENDVOID;
  ((WTOBJECT *)trail)->DisableDrawing();
}

void WeaponTrailSetDrawing(int trail, const NTempest::CImVector &color, int fadeOutRate, UINT duration) {
  if (duration && trail) {
    ((WTOBJECT *)trail)->SetDrawTrail(color, fadeOutRate, duration);
  }
}
