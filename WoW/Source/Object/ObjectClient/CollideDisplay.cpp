#include "Object/MovementData.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Ui/WorldFrame.h"

#include <Gx/CGxDevice.h>
#include <Gx/Gx.h>

static int s_debugCollision;

int ToggleCollisionInfo() {
  unsigned __int64 watchTarget = 0;

  s_debugCollision = !s_debugCollision;
  if (s_debugCollision) {
    watchTarget = CGUnit_C::GetActiveMover();
    if (!watchTarget) {
      s_debugCollision = 0;
    }
  }

  CollisionInfoReset();
  CollisionInfoSetWatchGUID(watchTarget);
  return s_debugCollision;
}

void RenderCollisionInfo() {
  if (!s_debugCollision) {
    return;
  }

  GxRsPush();

  CGxLight debugLight;
  debugLight.m_enabled = 1;
  debugLight.m_isOmni = 0;
  debugLight.m_dir = NTempest::C3Vector(0.0f, 0.0f, -1.0f);
  debugLight.m_ambColor = NTempest::CImVector(0xFFFFFFFF);
  debugLight.m_dirColor = NTempest::CImVector(0xFFFFFFFF);
  debugLight.m_ambIntensity = 0.3f;
  debugLight.m_dirIntensity = 1.0f;
  GxLightSet(0, debugLight, NTempest::C3Vector(0.0f));

  for (unsigned int light = 1; light < 8; ++light) {
    GxLightEnable(light, 0);
  }

  GxRsSet(GxRs_PolygonOffset, 0.125f);

  NTempest::C44Matrix worldMtx;
  CGWorldFrame       *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldMtx.Translate(-worldFrame->Camera()->Position());

  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Opaque);
  GxVertexShaderSelect(GxVS_PassThru);
  GxXformPush(GxXform_World, worldMtx);

  if (g_debugVerts.Count() || g_debugIndices.Count()) {
    GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
    GxPrimLockVertexPtrs(
        g_debugVerts.Count(), g_debugVerts.Ptr(), sizeof(NTempest::C3Vector), 0, 0, g_debugVertColors.Ptr(), sizeof(NTempest::CImVector), 0, 0, 0, 0,
        0, 0
    );
    GxPrimDrawElements(GxPrim_Triangles, g_debugIndices.Count(), g_debugIndices.Ptr());
    GxPrimUnlockVertexPtrs();
  }

  if (g_debugBoxVerts.Count() || g_debugBoxIndices.Count()) {
    GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x80FF0000));
    GxPrimLockVertexPtrs(
        g_debugBoxVerts.Count(), g_debugBoxVerts.Ptr(), sizeof(NTempest::C3Vector), g_debugBoxNormals.Ptr(), sizeof(NTempest::C3Vector), 0, 0, 0, 0,
        0, 0, 0, 0
    );
    GxPrimDrawElements(GxPrim_Triangles, g_debugBoxIndices.Count(), g_debugBoxIndices.Ptr());
    GxPrimUnlockVertexPtrs();
  }

  if (g_debugNormalVerts.Count() || g_debugNormalIndices.Count()) {
    GxRsSet(GxRs_Lighting, 0);
    NTempest::CImVector blue(0xFF0000FF);
    GxPrimLockVertexPtrs(g_debugNormalVerts.Count(), g_debugNormalVerts.Ptr(), sizeof(NTempest::C3Vector), 0, 0, &blue, 0, 0, 0, 0, 0, 0, 0);
    GxPrimDrawElements(GxPrim_Lines, g_debugNormalIndices.Count(), g_debugNormalIndices.Ptr());
    GxPrimUnlockVertexPtrs();
  }

  GxXformPop(GxXform_World);
  GxRsPop();
}
