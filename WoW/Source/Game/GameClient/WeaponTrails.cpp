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

struct VERTEX {
  NTempest::C3Vector  v;
  NTempest::CImVector c;
};

NODEDECL(SWING) {
  TSGrowableArray<VERTEX>         m_trail;
  TSGrowableArray<unsigned short> m_vertexIndices;
  unsigned int                    m_flags;
  NTempest::C44Matrix             m_lastMatrix;

  SWING();
  ~SWING();
  void Recycle();
  void Render() const;
  void AddVerts(
      const NTempest::C44Matrix &basisMatrix,
      const NTempest::C3Vector  &bottom,
      const NTempest::C3Vector  &top,
      const NTempest::CImVector &color,
      unsigned char              currentAlpha,
      const NTempest::C3Vector  &cameraPos
  );
};

class WTOBJECT {
 public:
  LINKDECLEX(WTOBJECT, m_explicitLink);
  LISTDECL(SWING, m_swings);
  HMODEL                           m_model;
  unsigned int                     m_geosetID;
  NTempest::C3Vector               m_bottomCoord;
  NTempest::C3Vector               m_topCoord;
  NTempest::CImVector              m_color;
  int                              m_fadeOutRate;
  unsigned int                     m_flags;
  unsigned int                     m_timer;
  int                              m_currentAlpha;

  WTOBJECT();
  ~WTOBJECT();
  void Recycle();
  void DisableDrawing();
  void SetDrawTrail(const NTempest::CImVector &color, int fadeOutRate, unsigned int duration);
  void SetColor(NTempest::CImVector color);
  void SetFadeOutRate(int fadeOutRate);
  void Render(const NTempest::C44Matrix &basis);
  void RenderVerts(const NTempest::C3Vector &cameraPos);
  void FadeVerts();
};

void ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void(*renderCallback)(HMODEL, const NTempest::C34Matrix &, void *),
    void         *renderParam,
    unsigned int *custGeosetId
);
void ModelCustGeosetRemove(HMODEL model, unsigned int custGeosetId);
int ModelGetModelSpacePivot(HMODEL model, unsigned int objectId, NTempest::C3Vector *pivot);

static TInstanceAllocator<WTOBJECT> s_unusedObjects(100);
static TInstanceAllocator<SWING>    s_freeSwings(100);

static TSGrowableArray<VERTEX>         s_vertexBuffer;
static TSGrowableArray<unsigned short> s_freeVertexIndices;
static unsigned int                    STEPS_PER_180DEGS = 32;
static int                             ALPHAFADEOUTRATE = 24;
static int                             s_masterEnable = 1;
static CVar                           *s_consoleVarHandle;

static int DiscontinueTimerHandler(const void *data, void *userArg);

SWING::SWING() : m_flags(0) {
  m_trail.SetChunkSize(128);
  m_vertexIndices.SetChunkSize(128);
}

SWING::~SWING() {
  Recycle();
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
    unsigned char              currentAlpha,
    const NTempest::C3Vector  &cameraPos
) {
  NTempest::C44Matrix cameraTranslate;
  cameraTranslate.Translate(cameraPos);
  NTempest::C44Matrix matrix = basisMatrix * cameraTranslate;

  if (m_flags & 1) {
    NTempest::C33Matrix lastRotation(
        m_lastMatrix.a0, m_lastMatrix.a1, m_lastMatrix.a2, m_lastMatrix.b0, m_lastMatrix.b1, m_lastMatrix.b2, m_lastMatrix.c0, m_lastMatrix.c1,
        m_lastMatrix.c2
    );
    NTempest::C4Quaternion q1;
    q1.FromRotationMatrix(lastRotation);

    NTempest::C33Matrix    rotation(matrix.a0, matrix.a1, matrix.a2, matrix.b0, matrix.b1, matrix.b2, matrix.c0, matrix.c1, matrix.c2);
    NTempest::C4Quaternion q2;
    q2.FromRotationMatrix(rotation);

    unsigned int steps = static_cast<unsigned int>(fabs(1.0f - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w)) * STEPS_PER_180DEGS);
    if (steps < 1) {
      steps = 1;
    }

    NTempest::C3Vector startingTranslation(m_lastMatrix.d0, m_lastMatrix.d1, m_lastMatrix.d2);
    NTempest::C3Vector translationStep(
        (matrix.d0 - startingTranslation.x) / steps, (matrix.d1 - startingTranslation.y) / steps, (matrix.d2 - startingTranslation.z) / steps
    );
    float t = 0.0f;
    float tStep = 1.0f / steps;

    for (unsigned int step = 0; step < steps; ++step) {
      NTempest::C4Quaternion slerped = NTempest::C4Quaternion::Slerp(t, q1, q2);

      float xx = slerped.x + slerped.x;
      float yy = slerped.y + slerped.y;
      float zz = slerped.z + slerped.z;
      float wx = slerped.w * xx;
      float wy = slerped.w * yy;
      float wz = slerped.w * zz;
      float x2 = slerped.x * xx;
      float xy = slerped.x * yy;
      float xz = slerped.x * zz;
      float y2 = slerped.y * yy;
      float yz = slerped.y * zz;
      float z2 = slerped.z * zz;

      NTempest::C44Matrix basis(
          1.0f - (y2 + z2), xy + wz, xz - wy, 0.0f, xy - wz, 1.0f - (z2 + x2), yz + wx, 0.0f, xz + wy, yz - wx, 1.0f - (x2 + y2), 0.0f,
          startingTranslation.x, startingTranslation.y, startingTranslation.z, 1.0f
      );

      NTempest::C3Vector newBottom = bottom * basis;
      NTempest::C3Vector newTop = top * basis;
      VERTEX             newVerts[2];
      newVerts[0].v = newBottom;
      newVerts[0].c = color;
      newVerts[0].c.a = currentAlpha >> 1;
      newVerts[1].v = newTop;
      newVerts[1].c = color;
      newVerts[1].c.a = currentAlpha;

      unsigned int firstVertex = m_trail.Count();
      m_trail.Add(2, newVerts);
      m_vertexIndices.SetCount(firstVertex + 2);
      m_vertexIndices[firstVertex] = firstVertex;
      m_vertexIndices[firstVertex + 1] = firstVertex + 1;

      t += tStep;
      startingTranslation.x += translationStep.x;
      startingTranslation.y += translationStep.y;
      startingTranslation.z += translationStep.z;
    }
  } else {
    m_flags |= 1;
  }

  m_lastMatrix = matrix;
}

static bool ToggleCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
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

  while (SWING *swing = m_swings.Head()) {
    s_freeSwings.Put(swing);
  }

  if (m_timer) {
    ClientKillTimer(m_timer, DiscontinueTimerHandler, "DiscontinueTimerHandler");
  }
}

static void GeosetRenderFunction(HMODEL model, const NTempest::C34Matrix &basis, void *param) {
  FATALASSERT(param);

  NTempest::C44Matrix renderBasis(
      basis.a0, basis.a1, basis.a2, 0.0f, basis.b0, basis.b1, basis.b2, 0.0f, basis.c0, basis.c1, basis.c2, 0.0f, basis.d0, basis.d1, basis.d2, 1.0f
  );
  static_cast<WTOBJECT *>(param)->Render(renderBasis);
}

static int DiscontinueTimerHandler(const void *data, void *userArg) {
  WTOBJECT *trail = static_cast<WTOBJECT *>(userArg);
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
  while (SWING *swing = m_swings.Head()) {
    swing->Recycle();
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

  NTempest::C44Matrix world;
  world.Translate(NTempest::C3Vector(-cameraPos.x, -cameraPos.y, -cameraPos.z));
  GxXformPush(GxXform_World, world);

  ITERATELIST(SWING, m_swings, swing) {
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
    if (m_currentAlpha >= ALPHAFADEOUTRATE) {
      m_currentAlpha -= ALPHAFADEOUTRATE;
    } else {
      m_flags &= ~3;
    }
  }

  NTempest::C3Vector cameraPos(0.0f);
  CGWorldFrame::GetCameraPosition(&cameraPos);

  if (s_masterEnable && swing && (m_flags & 1)) {
    swing->AddVerts(basis, m_bottomCoord, m_topCoord, m_color, static_cast<unsigned char>(m_currentAlpha), cameraPos);
  }

  RenderVerts(cameraPos);
  FadeVerts();
}

void WTOBJECT::FadeVerts() {
  for (SWING *swing = m_swings.Head(); swing;) {
    SWING *next = m_swings.Next(swing);
    FATALASSERT(m_fadeOutRate < 0);

    int visible = 0;
    for (unsigned int i = 0; i < swing->m_trail.Count(); ++i) {
      int alpha = swing->m_trail[i].c.a + m_fadeOutRate;
      if (alpha >= 0) {
        swing->m_trail[i].c.a = alpha;
        visible = 1;
      } else {
        swing->m_trail[i].c.a = 0;
      }
    }

    if (swing->m_trail.Count() && !visible) {
      swing->Recycle();
      s_freeSwings.Put(swing);
    }
    swing = next;
  }
}

void WTOBJECT::DisableDrawing() {
  m_flags &= ~1u;
}

void WTOBJECT::SetDrawTrail(const NTempest::CImVector &color, int fadeOutRate, unsigned int duration) {
  if (m_timer) {
    ClientKillTimer(m_timer, DiscontinueTimerHandler, "DiscontinueTimerHandler");
  }

  m_flags |= 1;
  m_timer = 0;
  m_color.Set(*color.IV_());
  m_currentAlpha = color.a;
  m_fadeOutRate = fadeOutRate > 0 ? -fadeOutRate : fadeOutRate;

  SWING *swing = s_freeSwings.Get(0);
  if (swing) {
    m_swings.LinkNode(swing, LIST_TAIL, 0);
  }

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
  FATALASSERT(model);

  WTOBJECT *trail = s_unusedObjects.Get(0);

  trail->m_model = static_cast<HMODEL>(HandleDuplicate(model));
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

  return reinterpret_cast<int>(trail);
}

void WeaponTrailClose(int trail) {
  FATALASSERT(trail);

  WTOBJECT *object = reinterpret_cast<WTOBJECT *>(trail);
  if (object->m_model && object->m_geosetID) {
    ModelCustGeosetRemove(object->m_model, object->m_geosetID);
  }
  s_unusedObjects.Put(object);
}

void WeaponTrailSetColor(int trail, NTempest::CImVector color) {
  if (trail) {
    reinterpret_cast<WTOBJECT *>(trail)->SetColor(color);
  }
}

void WeaponTrailSetFadeOutRate(int trail, int fadeOutRate) {
  if (trail) {
    reinterpret_cast<WTOBJECT *>(trail)->SetFadeOutRate(fadeOutRate);
  }
}

void WeaponTrailDisableDrawing(int trail) {
  if (trail) {
    reinterpret_cast<WTOBJECT *>(trail)->DisableDrawing();
  }
}

void WeaponTrailSetDrawing(int trail, const NTempest::CImVector &color, int fadeOutRate, unsigned int duration) {
  if (trail && duration) {
    reinterpret_cast<WTOBJECT *>(trail)->SetDrawTrail(color, fadeOutRate, duration);
  }
}
