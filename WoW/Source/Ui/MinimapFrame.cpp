#include <WowConst.h>
#include <MapDefs.h>

#include "MinimapFrame.h"
#include <Os/OsTime.h>

#include "Base/CDataStore.h"
#include "DayNight.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/AreaPOIRec.h"
#include "Game/GameClient/Minimap.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/Handle.h>
#include <Base/Status.h>
#include <Base/Coordinate.h>
#include <DB/WowLocale.h>
#include <Event/CMouseEvent.h>
#include <Frame/CSimpleModel.h>
#include <Frame/CSimpleRender.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameXML/XMLTree.h>
#include <FrameScript/FrameScript.h>
#include <Services/Texture.h>
#include <Services/Camera.h>
#include <Tempest/c2vector.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <Tempest/cimvector.h>
#include <storm.h>
#include <lauxlib.h>
#include <lua.h>

struct POIINFO {
  unsigned int       icon;
  NTempest::C3Vector vertices[4];
  NTempest::C2Vector position;
  const char        *string;
};

struct OBJINFO {
  unsigned __int64   object;
  NTempest::C2Vector position;
};

struct MINIMAPINFO {
  CGPlayer_C        *player;
  NTempest::C3Vector currentPos;
  float              radius;
  float              layoutScale;
};

static struct {
  float        scale;
  unsigned int color;
} s_miniMapTypeInfo[5] = {
    {1.0f, 0xFFFFFF00},
    {1.0f, 0xFFFF0000},
    {1.0f, 0xFF7F7FFF},
    {1.0f, 0xFF00FF00},
    {1.3f, 0xFF00FF00}
};

static float const ICON_SIZE = 0.0125f;
static float const BLIP_SIZE = 0.00625f;
static float const ICON_HALF = ICON_SIZE * 0.5f;
static float const BLIP_HALF = BLIP_SIZE * 0.5f;
static float ONETHIRD = 0.33333334f;
static float const POI_ARROW_RADIUS = 0.048f;
static float const MINIMAPSIDELENGTH = 0.108f;

static int                   s_tooltipDisplay;
static int                   s_tooltipDisplayDistant = -1;
static int                   s_tooltipDisplayParty = -1;
static int                   s_initialized;
static QUADDATA              s_quadData[1024];
struct QUADINFO {
  NTempest::C2Vector m_UL;
  NTempest::C2Vector m_LR;
  NTempest::C2Vector m_conversion;

  QUADINFO(
      const NTempest::C2Vector &upperLeft,
      const NTempest::C2Vector &lowerRight,
      const NTempest::C2Vector &conversion
  ) : m_UL(upperLeft), m_LR(lowerRight), m_conversion(conversion) {
  }
};
static const QUADINFO s_mapBoxExtents[4] = {
    QUADINFO(NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(0.5f, 0.5f), NTempest::C2Vector(0.0f, 0.0f)),
    QUADINFO(NTempest::C2Vector(0.5f, 0.0f), NTempest::C2Vector(1.0f, 0.5f), NTempest::C2Vector(1.0f, 0.0f)),
    QUADINFO(NTempest::C2Vector(0.5f, 0.5f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f)),
    QUADINFO(NTempest::C2Vector(0.0f, 0.5f), NTempest::C2Vector(0.5f, 1.0f), NTempest::C2Vector(0.0f, 1.0f))
};
static HTEXTURE                          s_iconTexture;
static HTEXTURE                          s_blipTexture;
static HTEXTURE                          s_minimapMaskTexture;
static TSGrowableArray<POIINFO>          s_POIInfo;
static TSGrowableArray<POIDIRECTIONDATA> s_POIDirectionData;
static NTempest::C2Vector                s_iconCoords[16][4];
static NTempest::C3Vector                s_iconVertices[4] = {
    NTempest::C3Vector(-ICON_HALF, -ICON_HALF, 0.0f), NTempest::C3Vector(ICON_HALF, -ICON_HALF, 0.0f),
    NTempest::C3Vector(-ICON_HALF, ICON_HALF, 0.0f), NTempest::C3Vector(ICON_HALF, ICON_HALF, 0.0f)
};
static NTempest::C3Vector                s_blipVertices[4] = {
    NTempest::C3Vector(-BLIP_HALF, -BLIP_HALF, 0.0f), NTempest::C3Vector(BLIP_HALF, -BLIP_HALF, 0.0f),
    NTempest::C3Vector(-BLIP_HALF, BLIP_HALF, 0.0f), NTempest::C3Vector(BLIP_HALF, BLIP_HALF, 0.0f)
};
static TSGrowableArray<OBJINFO>          s_miniMapObjects[5];
static PARTYMEMBERINFO                   s_partyDirectionData[5];
static unsigned short idx[4] = {0, 1, 3, 2};

NTempest::C2Vector CGMinimapFrame::m_pingPosition;
MinimapTexParams   CGMinimapFrame::s_minimapTexParams;

NTempest::C2Vector CGMinimapFrame::WorldPosToMinimapFrameCoords(
    const NTempest::C3Vector centerPoint,
    float                    radius,
    float                    x,
    float                    y,
    float                    layoutScale
) {
  const float halfSize = MINIMAPSIDELENGTH * layoutScale * 0.5f;
  const float ooRadius = 1.0f / radius;
  return NTempest::C2Vector(halfSize - halfSize * (y - centerPoint.y) * ooRadius, halfSize + halfSize * (x - centerPoint.x) * ooRadius);
}

int CGMinimapFrame::ObjectEnumProc(unsigned __int64 object, void *param) {
  MINIMAPINFO *info = static_cast<MINIMAPINFO *>(param);
  FATALASSERT(info);

  if (object == ClntObjMgrGetActivePlayer()) {
    return 1;
  }

  CGPlayer_C *player = info->player;
  CGObject_C *objectPtr = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  if (!objectPtr) {
    return 1;
  }

  NTempest::C3Vector pos;
  objectPtr->GetPosition(pos);
  if (info->radius * info->radius < (pos - info->currentPos).SquaredMag()) {
    return 1;
  }

  unsigned int type;
  switch (objectPtr->GetType()) {
    case HIER_TYPE_PLAYER:
      if (CGGameUI::IsPartyMember(object)) {
        return 1;
      }
    case HIER_TYPE_UNIT: {
      CGUnit_C               *unit = static_cast<CGUnit_C *>(objectPtr);
      const CGUnitData       *unitData = unit->GetUnitData();
      const unsigned __int64 &owner = unitData->charmedBy ? unitData->charmedBy : unitData->summonedBy;
      if (owner == ClntObjMgrGetActivePlayer()) {
        type = 2;
      } else {
        if (unitData->health <= 0 || !player->CanTrack(unit)) {
          return 1;
        }
        type = 1;
      }
      break;
    }
    case HIER_TYPE_GAMEOBJECT:
      type = 0;
      if (!player->CanTrack(static_cast<CGGameObject_C *>(objectPtr))) {
        return 1;
      }
      break;
    default:
      return 1;
  }

  OBJINFO *objectInfo = s_miniMapObjects[type].New();
  objectPtr->GetPosition(pos);
  NTempest::C2Vector framePos = WorldPosToMinimapFrameCoords(info->currentPos, info->radius, pos.x, pos.y, info->layoutScale);
  objectInfo->object = object;
  objectInfo->position = framePos;
  return 1;
}

QUADDATA::QUADDATA() : m_texture(0) {
}

NTempest::CRect QUADDATA::NormalizeToQuad(unsigned int quad, NTempest::CRect clippedRect) {
  FATALASSERT(quad < 1024);
  const QUADINFO &box = s_mapBoxExtents[quad];
  return NTempest::CRect(
      2.0f * (clippedRect.t - box.m_UL.y), 2.0f * (clippedRect.l - box.m_UL.x),
      2.0f * (clippedRect.b - box.m_UL.y), 2.0f * (clippedRect.r - box.m_UL.x)
  );
}

void QUADDATA::UpdateData(unsigned int quad, const NTempest::C2Vector centerPoint, float radius, float layoutScale) {
  FATALASSERT(quad < 1024);
  m_flags &= ~1u;
  if (quad >= 4) {
    return;
  }

  NTempest::CRect maskBox(centerPoint.y - radius, centerPoint.x - radius, centerPoint.y + radius, centerPoint.x + radius);
  const QUADINFO &box = s_mapBoxExtents[quad];
  NTempest::CRect clippedRect;
  clippedRect.t = box.m_UL.y > maskBox.t ? box.m_UL.y : maskBox.t;
  clippedRect.l = box.m_UL.x > maskBox.l ? box.m_UL.x : maskBox.l;
  clippedRect.b = box.m_LR.y < maskBox.b ? box.m_LR.y : maskBox.b;
  clippedRect.r = box.m_LR.x < maskBox.r ? box.m_LR.x : maskBox.r;
  if (clippedRect.NotEmpty()) {
    GenerateVertTexInfo(clippedRect, quad, centerPoint, radius, maskBox, layoutScale);
    m_flags |= 1;
  }
}

void CGMinimapFrame::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  unsigned int modelFlags = m_playerArrowFrame->m_flags;
  if (m_playerArrowFrame->ModelJustLoaded() || (!(modelFlags & 0x8) && (modelFlags & 0x1))) {
    SetPlayerArrowPosition();
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    UpdateArrowRotation(player->GetFacing());
  }
}

int CGMinimapFrame::OnLayerTrackUpdate(const CMouseEvent &evt) {
  s_tooltipDisplayDistant = -1;
  s_tooltipDisplayParty = -1;
  s_tooltipDisplay = 0;

  if (!CSimpleFrame::OnLayerTrackUpdate(evt)) {
    m_tooltip->Hide();
    return 0;
  }

  NTempest::CRect baseRect;
  GetRect(&baseRect);

  NTempest::C2Vector framecoords;
  unsigned int       count;
  for (count = 0; count < s_POIInfo.Count(); ++count) {
    framecoords.x = baseRect.l + s_POIInfo[count].position.x;
    framecoords.y = baseRect.t + s_POIInfo[count].position.y;

    if (framecoords.x - ICON_HALF <= evt.x && framecoords.x + ICON_HALF >= evt.x && framecoords.y - ICON_HALF <= evt.y &&
        framecoords.y + ICON_HALF >= evt.y)
    {
      if (!m_tooltipText->GetText() || !*m_tooltipText->GetText() || !s_POIInfo[count].string ||
          SStrCmp(m_tooltipText->GetText(), s_POIInfo[count].string, 0x7FFFFFFF))
      {
        m_tooltipText->SetText(s_POIInfo[count].string);
        m_tooltipText->SetWidth(m_tooltipText->GetWidth() + 0.015f);
      }

      float x = evt.x;
      if (x > 0.8f - m_tooltipText->GetWidth() * 0.5f) {
        x = 0.8f - m_tooltipText->GetWidth() * 0.5f;
      }
      m_tooltip->SetPoint(FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, x / GetLayoutScale(), evt.y / GetLayoutScale(), 1);
      m_tooltip->Show();
      s_tooltipDisplay = 1;
      return 1;
    }
  }

  for (unsigned int type = 0; type < 5; ++type) {
    for (count = 0; count < s_miniMapObjects[type].Count(); ++count) {
      framecoords.x = baseRect.l + s_miniMapObjects[type][count].position.x;
      framecoords.y = baseRect.t + s_miniMapObjects[type][count].position.y;
      if (framecoords.x - BLIP_HALF * s_miniMapTypeInfo[type].scale <= evt.x &&
          framecoords.x + BLIP_HALF * s_miniMapTypeInfo[type].scale >= evt.x &&
          framecoords.y - BLIP_HALF * s_miniMapTypeInfo[type].scale <= evt.y &&
          framecoords.y + BLIP_HALF * s_miniMapTypeInfo[type].scale >= evt.y)
      {
        const char *string = 0;
        CGObject_C *object = ClntObjMgrObjectPtr(s_miniMapObjects[type][count].object, __FILE__, __LINE__);
        if (object) {
          string = object->GetObjectName();
        } else {
          const NameCache *name = g_nameDBCache.GetRecord(s_miniMapObjects[type][count].object, s_miniMapObjects[type][count].object, 0, 0);
          if (name) {
            string = name->m_name;
          }
        }

        if (string) {
          if (!m_tooltipText->GetText() || !*m_tooltipText->GetText() || SStrCmp(m_tooltipText->GetText(), string, 0x7FFFFFFF)) {
            m_tooltipText->SetText(string);
            m_tooltipText->SetWidth(m_tooltipText->GetWidth() + 0.015f);
          }

          float x = evt.x;
          if (x > 0.8f - m_tooltipText->GetWidth() * 0.5f) {
            x = 0.8f - m_tooltipText->GetWidth() * 0.5f;
          }
          m_tooltip->SetPoint(
              FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, x / GetLayoutScale(), evt.y / GetLayoutScale(), 1
          );
          m_tooltip->Show();
          s_tooltipDisplay = 1;
          return 1;
        }
      }
    }
  }

  for (count = 0; count < 3; ++count) {
    if (m_rotatingArrowFrame[count]->IsVisible()) {
      NTempest::C3Vector pos = m_rotatingArrowFrame[count]->GetPosition();
      float              x = baseRect.l + pos.x * GetLayoutScale();
      float              y = baseRect.t + pos.y * GetLayoutScale();

      if (x <= evt.x && x + 0.015f >= evt.x && y + 0.001f <= evt.y && y + 0.016f >= evt.y) {
        if (!m_tooltipText->GetText() || !*m_tooltipText->GetText() ||
            SStrCmp(m_tooltipText->GetText(), s_POIDirectionData[count].POIName, 0x7FFFFFFF))
        {
          m_tooltipText->SetText(s_POIDirectionData[count].POIName);
          m_tooltipText->SetWidth(m_tooltipText->GetWidth() + 0.015f);
        }

        float tooltipX = evt.x;
        if (tooltipX > 0.8f - m_tooltipText->GetWidth() * 0.5f) {
          tooltipX = 0.8f - m_tooltipText->GetWidth() * 0.5f;
        }
        m_tooltip->SetPoint(
            FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, tooltipX / GetLayoutScale(), evt.y / GetLayoutScale(), 1
        );
        m_tooltip->Show();
        s_tooltipDisplayDistant = count;
        return 1;
      }
    }
  }

  for (count = 0; count < 5; ++count) {
    if (m_rotatingPartyFrame[count]->IsVisible()) {
      NTempest::C3Vector pos = m_rotatingPartyFrame[count]->GetPosition();
      float              x = baseRect.l + pos.x * GetLayoutScale();
      float              y = baseRect.t + pos.y * GetLayoutScale();

      if (x <= evt.x && x + 0.015f >= evt.x && y + 0.001f <= evt.y && y + 0.016f >= evt.y) {
        if (!m_tooltipText->GetText() || !*m_tooltipText->GetText() ||
            SStrCmp(m_tooltipText->GetText(), s_partyDirectionData[count].name, 0x7FFFFFFF))
        {
          m_tooltipText->SetText(s_partyDirectionData[count].name);
          m_tooltipText->SetWidth(m_tooltipText->GetWidth() + 0.015f);
        }

        float tooltipX = evt.x;
        if (tooltipX > 0.8f - m_tooltipText->GetWidth() * 0.5f) {
          tooltipX = 0.8f - m_tooltipText->GetWidth() * 0.5f;
        }
        m_tooltip->SetPoint(
            FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, tooltipX / GetLayoutScale(), evt.y / GetLayoutScale(), 1
        );
        m_tooltip->Show();
        s_tooltipDisplayParty = count;
        return 1;
      }
    }
  }

  m_tooltip->Hide();
  return 1;
}

void CGMinimapFrame::OnLayerCursorExit() {
  m_tooltip->Hide();
  s_tooltipDisplay = 0;
  s_tooltipDisplayDistant = -1;
  s_tooltipDisplayParty = -1;
}

void CGMinimapFrame::UpdateArrowRotation(float angle) {
  m_lastFacing = angle;
  m_playerArrowFrame->SetFacing(angle);
}

void QUADDATA::GenerateVertTexInfo(
    const NTempest::CRect    &rect,
    unsigned int              quad,
    const NTempest::C2Vector &centerPoint,
    float                     radius,
    const NTempest::CRect    &maskBox,
    float                     layoutScale
) {
  FATALASSERT(centerPoint.x + radius <= 1.0f);
  FATALASSERT(centerPoint.y + radius <= 1.0f);
  FATALASSERT(centerPoint.x >= radius);
  FATALASSERT(centerPoint.y >= radius);
  FATALASSERT(rect.l >= maskBox.l);
  FATALASSERT(rect.r <= maskBox.r);
  FATALASSERT(rect.t >= maskBox.t);
  FATALASSERT(rect.b <= maskBox.b);

  const float diameter = radius + radius;
  const float ooDiameter = 1.0f / diameter;
  const float minx = (rect.l - maskBox.l) * ooDiameter;
  const float maxx = (rect.r - maskBox.l) * ooDiameter;
  const float miny = (rect.t - maskBox.t) * ooDiameter;
  const float maxy = (rect.b - maskBox.t) * ooDiameter;

  maskTexCoords[0] = NTempest::C2Vector(minx, maxy);
  maskTexCoords[1] = NTempest::C2Vector(maxx, maxy);
  maskTexCoords[2] = NTempest::C2Vector(minx, miny);
  maskTexCoords[3] = NTempest::C2Vector(maxx, miny);

  NTempest::CRect normalized = NormalizeToQuad(quad, rect);
  texCoords[0] = NTempest::C2Vector(normalized.l, normalized.b);
  texCoords[1] = NTempest::C2Vector(normalized.r, normalized.b);
  texCoords[2] = NTempest::C2Vector(normalized.l, normalized.t);
  texCoords[3] = NTempest::C2Vector(normalized.r, normalized.t);

  const float minimapSize = MINIMAPSIDELENGTH * layoutScale;
  const float left = minx * minimapSize;
  const float right = maxx * minimapSize;
  const float top = minimapSize - miny * minimapSize;
  const float bottom = minimapSize - maxy * minimapSize;
  verts[0] = NTempest::C3Vector(left, bottom, 0.0f);
  verts[1] = NTempest::C3Vector(right, bottom, 0.0f);
  verts[2] = NTempest::C3Vector(left, top, 0.0f);
  verts[3] = NTempest::C3Vector(right, top, 0.0f);
}

void CGMinimapFrame::UpdateGeometry(const NTempest::C2Vector &centerPoint, float radius) {
  for (unsigned int quad = 0; quad < 1024; ++quad) {
    s_quadData[quad].UpdateData(quad, centerPoint, radius, GetLayoutScale());
  }
}

void CGMinimapFrame::RenderObjectBlips(const DNInfo *dnInfo) {
  FATALASSERT(dnInfo);

  static NTempest::C3Vector   normal(0.0f, 0.0f, 1.0f);
  static NTempest::C2Vector   texCoords[4] = {
      NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(0.0f, 0.0f),
      NTempest::C2Vector(1.0f, 0.0f)
  };
  static const unsigned short iconVertIndices[4] = {0, 1, 2, 3};

  for (unsigned int type = 0; type < 5; ++type) {
    if (!s_miniMapObjects[type].Count() || !s_blipTexture) {
      continue;
    }

    GxRsSet(GxRs_Texture0, TextureGetGxTex(s_blipTexture, 1, 0));
    NTempest::CImVector white(0xFFFFFFFF);

    for (unsigned int index = 0; index < s_miniMapObjects[type].Count(); ++index) {
      const NTempest::C2Vector &position = s_miniMapObjects[type][index].position;
      NTempest::C3Vector        verts[4];
      for (unsigned int vertex = 0; vertex < 4; ++vertex) {
        verts[vertex] = NTempest::C3Vector(
            position.x + s_blipVertices[vertex].x * s_miniMapTypeInfo[type].scale,
            position.y + s_blipVertices[vertex].y * s_miniMapTypeInfo[type].scale,
            s_blipVertices[vertex].z * s_miniMapTypeInfo[type].scale
        );
      }

      GxPrimLockVertexPtrs(
          4, verts, sizeof(NTempest::C3Vector), &normal, 0, &white, 0, 0, 0, s_iconCoords[type], sizeof(NTempest::C2Vector), 0, 0
      );
      GxPrimDrawElements(GxPrim_TriangleStrip, 4, iconVertIndices);
      GxPrimUnlockVertexPtrs();
    }
  }
}

void CGMinimapFrame::OnFrameRender(CRenderBatch *batch, unsigned int layer) {
  CSimpleFrame::OnFrameRender(batch, layer);
  batch->QueueCallback(RenderCallback, this);
}

void CGMinimapFrame::RenderCallback(void *param) {
  if (param) {
    static_cast<CGMinimapFrame *>(param)->Render();
  }
}

void CGMinimapFrame::RenderInsideSortQuads(QUADDATA *&rHead) {
  for (unsigned int index = 0; index < 1024; ++index) {
    QUADDATA *quad = &s_quadData[index];
    if (quad->m_flags & 2) {
      QUADDATA **link = &rHead;
      while (*link && quad->sortz > (*link)->sortz) {
        link = &(*link)->rLink;
      }
      quad->rLink = *link;
      *link = quad;
    }
  }
}

void CGMinimapFrame::RenderInsideQuad(QUADDATA *q) {
  NTempest::C3Vector t(q->aaBox.b.x, q->aaBox.b.y, q->sortz);
  NTempest::C3Vector geo[4] = {
      t, NTempest::C3Vector(q->aaBox.t.x, q->aaBox.b.y, q->sortz), NTempest::C3Vector(q->aaBox.t.x, q->aaBox.t.y, q->sortz),
      NTempest::C3Vector(q->aaBox.b.x, q->aaBox.t.y, q->sortz)
  };
  static NTempest::C2Vector tex[4] = {
      NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(1.0f, 0.0f),
      NTempest::C2Vector(0.0f, 0.0f)
  };
  NTempest::CImVector WHITE(0xFFFFFFFF);

  CGxTex *texture = TextureGetGxTex(q->m_texture, 0, 0);
  if (!texture) {
    s_minimapTexParams.asyncTexWait = 1;
    return;
  }

  GxRsSet(GxRs_Texture0, texture);
  GxPrimLockVertexPtrs(4, geo, sizeof(NTempest::C3Vector), 0, 0, &WHITE, 0, 0, 0, tex, sizeof(NTempest::C2Vector), 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, idx);
  GxPrimUnlockVertexPtrs();
}

void CGMinimapFrame::RenderInsideTexture() {
  NTempest::C44Matrix oldViewMtx;
  NTempest::C44Matrix oldProjMtx;
  NTempest::C44Matrix projMtx;
  QUADDATA           *rHead = 0;
  const float         orthoSize = s_minimapTexParams.size * 1.5f;

  GxXformProjection(oldProjMtx);
  GxuXformCreateOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, -1000.0f, 1000.0f, projMtx);
  GxXformSetProjection(projMtx);
  GxXformView(oldViewMtx);
  GxXformSetView(NTempest::C44Matrix());

  GxRsPush();
  GxRsInit();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);

  GxXformPush(GxXform_World);
  GxXformIdentity(GxXform_World);
  GxXformMult(GxXform_World, s_minimapTexParams.worldRotation);
  GxXformTranslate(
      GxXform_World,
      NTempest::C3Vector(-s_minimapTexParams.localCenter.x, -s_minimapTexParams.localCenter.y, -s_minimapTexParams.localCenter.z)
  );

  s_minimapTexParams.asyncTexWait = 0;
  RenderInsideSortQuads(rHead);
  for (QUADDATA *quad = rHead; quad; quad = quad->rLink) {
    RenderInsideQuad(quad);
  }

  GxXformPop(GxXform_World);
  GxXformSetView(oldViewMtx);
  GxXformSetProjection(oldProjMtx);
  GxRsPop();
}

void CGMinimapFrame::RenderInside(float minimapSize, const NTempest::C2Vector &localOffset) {
  NTempest::C44Matrix oldProjMtx;
  NTempest::C44Matrix projMtx;
  GxXformProjection(oldProjMtx);

  const float orthoSize = minimapSize * 0.5f;
  GxuXformCreateOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, -1000.0f, 1000.0f, projMtx);
  GxXformSetProjection(projMtx);

  GxRsPush();
  GxRsSet(GxRs_TexGen0, GxTexGen_View);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxRsSet(GxRs_Texture0, s_minimapTexParams.texture);
  GxXformPush(GxXform_Tex0);
  GxXformTranslate(GxXform_Tex0, NTempest::C3Vector(0.0f, 1.0f, 0.0f));
  GxXformScale(GxXform_Tex0, NTempest::C3Vector(1.0f, -1.0f, 1.0f));
  GxXformTranslate(GxXform_Tex0, NTempest::C3Vector(localOffset.x, localOffset.y, 0.0f));
  const float ooOrthoSize = 1.0f / orthoSize;
  const float texScale = ooOrthoSize * (1.0f / 3.0f);
  GxXformScale(GxXform_Tex0, NTempest::C3Vector(texScale, texScale, texScale));

  GxRsSet(GxRs_TexGen1, GxTexGen_View);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);
  GxRsSet(GxRs_Texture1, TextureGetGxTex(s_minimapMaskTexture, 1, 0));
  GxXformPush(GxXform_Tex1);
  GxXformTranslate(GxXform_Tex1, NTempest::C3Vector(0.0f, 1.0f, 0.0f));
  GxXformScale(GxXform_Tex1, NTempest::C3Vector(1.0f, -1.0f, 1.0f));
  GxXformTranslate(GxXform_Tex1, NTempest::C3Vector(0.5f, 0.5f, 0.0f));
  const float maskScale = ooOrthoSize * 0.5f;
  GxXformScale(GxXform_Tex1, NTempest::C3Vector(maskScale, maskScale, maskScale));

  GxXformPush(GxXform_World);
  GxXformScale(GxXform_World, NTempest::C3Vector(orthoSize, orthoSize, orthoSize));

  static NTempest::C3Vector geo[4] = {
      NTempest::C3Vector(-1.0f, -1.0f, 0.0f), NTempest::C3Vector(-1.0f, 1.0f, 0.0f), NTempest::C3Vector(1.0f, 1.0f, 0.0f),
      NTempest::C3Vector(1.0f, -1.0f, 0.0f)
  };
  static NTempest::CImVector  white(0xFFFFFFFF);
  static const unsigned short vertIndices[4] = {0, 1, 3, 2};
  GxPrimLockVertexPtrs(4, geo, sizeof(NTempest::C3Vector), 0, 0, &white, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, vertIndices);
  GxPrimUnlockVertexPtrs();

  GxXformSetProjection(oldProjMtx);
  GxXformPop(GxXform_World);
  GxXformPop(GxXform_Tex1);
  GxXformPop(GxXform_Tex0);
  GxRsPop();
}

void CGMinimapFrame::MinimapTextureCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (cmd == GxTex_Latch) {
    NTempest::CAaBox    vp;

    GxXformViewport(vp.b.x, vp.t.x, vp.b.y, vp.t.y, vp.b.z, vp.t.z);
    GxDevSetRenderTarget(GxBuffers_Color, s_minimapTexParams.texture, 0);
    GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    NTempest::CImVector saveClearColor = GxSceneClearColor();
    GxSceneSetClearColor(NTempest::CImVector(0xFF000000));
    GxSceneClear(1);
    GxSceneSetClearColor(saveClearColor);
    RenderInsideTexture();
    GxDevSetRenderTarget(GxBuffers_Color, 0, 0);
    GxXformSetViewport(vp.b.x, vp.t.x, vp.b.y, vp.t.y, vp.b.z, vp.t.z);
  }
}

void QUADDATA::Render(unsigned int quad, const NTempest::CImVector &color) const {
  FATALASSERT(quad < 1024);

  if (!(m_flags & 1) || !m_texture) {
    return;
  }

  CGxTex *texture = TextureGetGxTex(m_texture, 0, 0);
  if (!texture) {
    return;
  }

  GxRsSet(GxRs_Texture0, texture);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxRsSet(GxRs_Texture1, TextureGetGxTex(s_minimapMaskTexture, 1, 0));
  GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);

  static NTempest::C3Vector   normal(0.0f, 0.0f, 1.0f);
  static const unsigned short vertIndices[4] = {0, 1, 2, 3};
  GxPrimLockVertexPtrs(
      4, verts, sizeof(NTempest::C3Vector), &normal, 0, &color, 0, 0, 0, texCoords, sizeof(NTempest::C2Vector), maskTexCoords,
      sizeof(NTempest::C2Vector)
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, vertIndices);
  GxPrimUnlockVertexPtrs();
  GxRsSet(GxRs_Texture1, 0);
}

void CGMinimapFrame::Initialize(int continentID) {
  s_initialized = 1;
  MinimapInitialize(continentID);

  CStatus status;

  s_minimapMaskTexture = TextureCreate("Textures\\MinimapMask", CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
  FATALASSERT(s_minimapMaskTexture);

  s_iconTexture = TextureCreate("Interface\\Minimap\\POIIcons", CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
  FATALASSERT(s_iconTexture);

  s_blipTexture = TextureCreate("Interface\\Minimap\\ObjectIcons", CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
  FATALASSERT(s_blipTexture);

  s_minimapTexParams.size = 10.0f;

  EGxTexFormat format;
  if (GxCaps().m_rttFormat[GxTex_Argb8888]) {
    format = GxTex_Argb8888;
  } else if (GxCaps().m_rttFormat[GxTex_Rgb565]) {
    format = GxTex_Rgb565;
  } else {
    FATALASSERT(!("CGMinimapFrame::Initialize(): can't get render target for minimap"));
  }

  GxTexCreate(
      256, 256, format, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 1, 1), 0, MinimapTextureCallback, s_minimapTexParams.texture
  );

  for (unsigned int icon = 0; icon < 16; ++icon) {
    float left = static_cast<float>(32 * (icon & 3));
    float right = left + 32.0f;
    float top = static_cast<float>(32 * (icon / 4));
    float bottom = top + 32.0f;

    s_iconCoords[icon][0] = NTempest::C2Vector(left * 0.0078125f, bottom * 0.0078125f);
    s_iconCoords[icon][1] = NTempest::C2Vector(right * 0.0078125f, bottom * 0.0078125f);
    s_iconCoords[icon][2] = NTempest::C2Vector(left * 0.0078125f, top * 0.0078125f);
    s_iconCoords[icon][3] = NTempest::C2Vector(right * 0.0078125f, top * 0.0078125f);
  }
}

void CGMinimapFrame::Render() {
  if (!s_minimapMaskTexture || !(CLayoutFrame::m_flags & 1) || !s_initialized) {
    return;
  }

  CGPlayer_C *playerPtr = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!playerPtr) {
    return;
  }

  NTempest::CRect viewRect;
  if (!GetRect(&viewRect)) {
    return;
  }

  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  float               minX;
  float               maxX;
  float               minY;
  float               maxY;
  float               minZ;
  float               maxZ;
  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

  const NTempest::C2Vector position(viewRect.l, viewRect.t);
  CameraSetupScreenProjection(viewRect, position, 0.0f);
  DDCToNDC(viewRect.l, viewRect.t, &viewRect.l, &viewRect.t);
  DDCToNDC(viewRect.r, viewRect.b, &viewRect.r, &viewRect.b);
  GxXformSetViewport(viewRect.l, viewRect.r, viewRect.t, viewRect.b, 0.0f, 1.0f);
  GxSceneClear(2);

  NTempest::C3Vector currentPos;
  playerPtr->GetPosition(currentPos);
  NTempest::C2Vector centerPoint;
  float              radius;
  int                updateNeeded =
      MinimapUpdate(playerPtr->GetWorldObject(), CGPlayer_C::GetNewContinentID(), currentPos, centerPoint, radius, s_quadData, s_minimapTexParams);
  if (updateNeeded && !s_minimapTexParams.inside) {
    FATALASSERT(radius >= 0.0f);
    UpdateGeometry(centerPoint, radius);
  }

  static NTempest::C3Vector normal(0.0f, 0.0f, 1.0f);
  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);

  DNInfo             *dnInfo = DayNightGetInfo();
  NTempest::CImVector color = dnInfo->lightInfo.dirColor;
  NTempest::CImVector ambient = dnInfo->lightInfo.ambColor;
  unsigned int        brightness = ((151 * ambient.g + 77 * ambient.r + 28 * ambient.b) >> 8) + 96;
  if (brightness > 255) {
    brightness = 255;
  }
  ambient.r = static_cast<unsigned char>(ambient.r + ((brightness * (255 - ambient.r)) >> 8));
  ambient.g = static_cast<unsigned char>(ambient.g + ((brightness * (255 - ambient.g)) >> 8));
  ambient.b = static_cast<unsigned char>(ambient.b + ((brightness * (255 - ambient.b)) >> 8));
  color.r = static_cast<unsigned char>(color.r + ((192 * (ambient.r - color.r)) >> 8));
  color.g = static_cast<unsigned char>(color.g + ((192 * (ambient.g - color.g)) >> 8));
  color.b = static_cast<unsigned char>(color.b + ((192 * (ambient.b - color.b)) >> 8));
  GxVertexShaderSelect(GxVS_PassThru);

  if (s_minimapTexParams.inside) {
    NTempest::C44Matrix oldViewMtx;
    GxXformView(oldViewMtx);
    GxXformSetView(NTempest::C44Matrix());
    if (s_minimapTexParams.updateTexture) {
      GxTexUpdate(s_minimapTexParams.texture, 0, 0, 0, 0, 0);
    }

    const float               ooSize = 1.0f / s_minimapTexParams.size;
    const NTempest::C3Vector &offset = s_minimapTexParams.localOffset;
    NTempest::C2Vector        texOffset(
        ooSize *
                (s_minimapTexParams.worldRotation.a0 * offset.x + s_minimapTexParams.worldRotation.b0 * offset.y +
                 s_minimapTexParams.worldRotation.c0 * offset.z) *
                ONETHIRD +
            0.5f,
        ooSize *
                (s_minimapTexParams.worldRotation.a1 * offset.x + s_minimapTexParams.worldRotation.b1 * offset.y +
                 s_minimapTexParams.worldRotation.c1 * offset.z) *
                ONETHIRD +
            0.5f
    );
    RenderInside(s_minimapTexParams.size, texOffset);
    GxXformSetView(oldViewMtx);
  } else {
    for (unsigned int quad = 0; quad < 1024; ++quad) {
      s_quadData[quad].Render(quad, color);
    }
  }

  int                                        updatePOI;
  const TSGrowableArray<const AreaPOIRec *> &poi = MinimapGetPOI(updatePOI);
  if (updatePOI) {
    s_POIInfo.SetCount(poi.Count());
    for (unsigned int poiIndex = 0; poiIndex < poi.Count(); ++poiIndex) {
      const AreaPOIRec *rec = poi[poiIndex];
      POIINFO          &info = s_POIInfo[poiIndex];
      info.icon = rec->m_icon;
      info.string = rec->m_name_lang[CURRENT_LANGUAGE];
    }
  }

  NTempest::CImVector          white(0xFFFFFFFF);
  const float                  worldRadius = MinimapGetWorldRadius();
  static const unsigned short iconVertIndices[4] = {0, 1, 2, 3};
  for (unsigned int POICoord = 0; POICoord < s_POIInfo.Count(); ++POICoord) {
    POIINFO &info = s_POIInfo[POICoord];
    if (info.icon > 15) {
      continue;
    }

    const AreaPOIRec *rec = poi[POICoord];
    NTempest::C2Vector position =
        WorldPosToMinimapFrameCoords(currentPos, worldRadius, rec->m_x, rec->m_y, GetLayoutScale());
    for (unsigned int vertex = 0; vertex < 4; ++vertex) {
      info.vertices[vertex] = NTempest::C3Vector(
          position.x + s_iconVertices[vertex].x, position.y + s_iconVertices[vertex].y, s_iconVertices[vertex].z
      );
    }

    if (s_iconTexture) {
      GxRsSet(GxRs_Texture0, TextureGetGxTex(s_iconTexture, 1, 0));
      GxPrimLockVertexPtrs(
          4, info.vertices, sizeof(NTempest::C3Vector), &normal, 0, &white, 0, 0, 0, s_iconCoords[info.icon], sizeof(NTempest::C2Vector), 0, 0
      );
      GxPrimDrawElements(GxPrim_TriangleStrip, 4, iconVertIndices);
      GxPrimUnlockVertexPtrs();
      info.position = position;
    }
  }

  MinimapGetDistantPOI(s_POIDirectionData);
  MinimapGetPartyMembers(s_partyDirectionData);
  if (s_minimapTexParams.inside) {
    for (unsigned int hideArrow = 0; hideArrow < 3; ++hideArrow) {
      m_rotatingArrowFrame[hideArrow]->Hide();
      if (s_tooltipDisplayDistant == static_cast<int>(hideArrow) && !s_tooltipDisplay) {
        m_tooltip->Hide();
      }
    }
  } else {
    unsigned int arrowCount = s_POIDirectionData.Count();
    for (unsigned int showArrow = 0; showArrow < arrowCount; ++showArrow) {
      m_rotatingArrowFrame[showArrow]->Show();
    }
    for (unsigned int hideArrow = arrowCount; hideArrow < 3; ++hideArrow) {
      m_rotatingArrowFrame[hideArrow]->Hide();
      if (s_tooltipDisplayDistant == static_cast<int>(hideArrow) && !s_tooltipDisplay) {
        m_tooltip->Hide();
      }
    }
    while (arrowCount) {
      --arrowCount;
      const float   rotation = s_POIDirectionData[arrowCount].rotation;
      CSimpleModel *arrow = m_rotatingArrowFrame[arrowCount];
      arrow->SetPosition(NTempest::C3Vector(sin(rotation) * POI_ARROW_RADIUS * -0.95f + POI_ARROW_RADIUS, cos(rotation) * POI_ARROW_RADIUS * 0.95f + POI_ARROW_RADIUS, 0.0f));
      arrow->SetFacing(rotation);
      arrow->SetScale(0.4f);
    }
  }
  s_tooltipDisplayDistant = -1;

  for (unsigned int partyVisibility = 0; partyVisibility < 5; ++partyVisibility) {
    if (s_partyDirectionData[partyVisibility].showArrow) {
      m_rotatingPartyFrame[partyVisibility]->Show();
    } else {
      m_rotatingPartyFrame[partyVisibility]->Hide();
      if (s_tooltipDisplayParty == static_cast<int>(partyVisibility) && !s_tooltipDisplay) {
        m_tooltip->Hide();
      }
    }
  }
  s_tooltipDisplayParty = -1;
  for (unsigned int partyPosition = 0; partyPosition < 5; ++partyPosition) {
    if (s_partyDirectionData[partyPosition].showArrow) {
      const float   rotation = s_partyDirectionData[partyPosition].rotation;
      CSimpleModel *arrow = m_rotatingPartyFrame[partyPosition];
      arrow->SetPosition(NTempest::C3Vector(sin(rotation) * POI_ARROW_RADIUS * -0.95f + POI_ARROW_RADIUS, cos(rotation) * POI_ARROW_RADIUS * 0.95f + POI_ARROW_RADIUS, 0.0f));
      arrow->SetFacing(rotation);
      arrow->SetScale(0.13f);
    }
  }

  if (updateNeeded || !m_lastBlipUpdate || OsGetAsyncTimeMs() - m_lastBlipUpdate >= 1000) {
    m_lastBlipUpdate = OsGetAsyncTimeMs();
    if (!m_lastBlipUpdate) {
      m_lastBlipUpdate = 1;
    }

    for (unsigned int type = 0; type < 5; ++type) {
      s_miniMapObjects[type].SetCount(0);
    }

    MINIMAPINFO info;
    info.player = playerPtr;
    info.currentPos = currentPos;
    info.radius = worldRadius;
    info.layoutScale = GetLayoutScale();
    ClntObjMgrEnumVisibleObjects(ObjectEnumProc, &info);

    for (unsigned int member = 0; member < 5; ++member) {
      if (s_partyDirectionData[member].showBlip) {
        OBJINFO *objectInfo = s_miniMapObjects[4].New();
        objectInfo->object = s_partyDirectionData[member].guid;
        objectInfo->position = WorldPosToMinimapFrameCoords(
            currentPos, info.radius, s_partyDirectionData[member].position.x, s_partyDirectionData[member].position.y, info.layoutScale
        );
      }
    }
  }

  RenderObjectBlips(dnInfo);
  GxRsPop();
  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
}

void CGMinimapFrame::Shutdown() {
  MinimapShutdown();

  for (unsigned int quad = 0; quad < 1024; ++quad) {
    if (s_quadData[quad].m_texture) {
      HandleClose(s_quadData[quad].m_texture);
      s_quadData[quad].m_texture = 0;
    }
    s_quadData[quad].m_flags = 0;
    s_quadData[quad].m_areaNum.x = -1;
    s_quadData[quad].m_areaNum.y = -1;
  }

  if (s_iconTexture) {
    HandleClose(s_iconTexture);
  }
  s_iconTexture = 0;

  if (s_blipTexture) {
    HandleClose(s_blipTexture);
  }
  s_blipTexture = 0;

  if (s_minimapMaskTexture) {
    HandleClose(s_minimapMaskTexture);
  }
  s_minimapMaskTexture = 0;

  if (s_minimapTexParams.texture) {
    GxTexDestroy(s_minimapTexParams.texture);
  }
  s_minimapTexParams.texture = 0;

  s_POIInfo.Clear();
  for (unsigned int type = 0; type < 5; ++type) {
    s_miniMapObjects[type].Clear();
  }
}

CGMinimapFrame::CGMinimapFrame(CSimpleFrame *parent) : CSimpleFrame(parent), m_lastFacing(-10000.0f), m_lastBlipUpdate(0) {
  EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
}

void CGMinimapFrame::SetPlayerArrowPosition() {
  NTempest::C3Vector offset(m_playerArrowFrame->GetWidth() * 0.5f, m_playerArrowFrame->GetHeight() * 0.5f, 0.0f);
  m_playerArrowFrame->SetPosition(offset);
  m_playerArrowFrame->m_flags |= MODEL_ARROW_LOADED;
}

void CGMinimapFrame::PostLoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::PostLoadXML(node, status);

  unsigned int index;
  for (index = 0; index < 3; ++index) {
    m_rotatingArrowFrame[index] = NEW(CSimpleModel)(this);
    m_rotatingArrowFrame[index]->SetPoint(FRAMEPOINT_CENTER, this, FRAMEPOINT_CENTER, 0.003f, 0.003f, 1);
    m_rotatingArrowFrame[index]->SetWidth(0.1f);
    m_rotatingArrowFrame[index]->SetHeight(0.1f);
    m_rotatingArrowFrame[index]->Hide();
    m_rotatingArrowFrame[index]->SetModel(node->GetAttributeByName("minimapLandmarkModel"), 0, status);
  }

  for (index = 0; index < 5; ++index) {
    m_rotatingPartyFrame[index] = NEW(CSimpleModel)(this);
    m_rotatingPartyFrame[index]->SetPoint(FRAMEPOINT_CENTER, this, FRAMEPOINT_CENTER, 0.003f, 0.003f, 1);
    m_rotatingPartyFrame[index]->SetWidth(0.1f);
    m_rotatingPartyFrame[index]->SetHeight(0.1f);
    m_rotatingPartyFrame[index]->Hide();
    m_rotatingPartyFrame[index]->SetModel(node->GetAttributeByName("minimapPartyMemberModel"), 0, status);
  }

  m_playerArrowFrame = NEW(CSimpleModel)(this);
  m_playerArrowFrame->SetPoint(FRAMEPOINT_CENTER, this, FRAMEPOINT_CENTER, 0.001f, 0.001f, 1);
  m_playerArrowFrame->SetModel(node->GetAttributeByName("minimapPlayerModel"), 0, status);
  if (m_playerArrowFrame->m_flags & 0x1) {
    SetPlayerArrowPosition();
  } else {
    m_playerArrowFrame->m_flags &= ~MODEL_ARROW_LOADED;
  }

  char name[128];
  SStrPrintf(name, sizeof(name), "%sTooltip", GetName());
  m_tooltip = SimpleFrameRegistryGetEntry(name, 0);
  if (m_tooltip) {
    SStrPrintf(name, sizeof(name), "%sTooltipText", GetName());
    m_tooltipText = SimpleFontStringRegistryGetEntry(name, 0);
    FATALASSERT(m_tooltipText);
  }
  FATALASSERT(m_tooltip);
}

void CGMinimapFrame::SetPingPosition(const unsigned __int64 &sender, const NTempest::C2Vector &pos) {
  char               name[32] = "player";
  NTempest::C2Vector diff;
  CGPlayer_C        *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  m_pingPosition = pos;
  for (int index = 0; index < 4; ++index) {
    if (sender == CGGameUI::GetPartyMember(index)) {
      SStrPrintf(name, sizeof(name), "party%d", index + 1);
      break;
    }
  }

  diff = pos - static_cast<NTempest::C2Vector>(player->GetPosition());
  float scale = 1.0f / (MinimapGetViewRadius() * 2.0f);
  FrameScript_SignalEvent(337, "%s%f%f", name, -diff.y * scale, diff.x * scale);
}

static int CGMinimapFrame_GetZoomLevels(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(MinimapGetZoomLevels()));
  return 1;
}

#define GET_MINIMAP_THIS(L, object)                                \
  CGMinimapFrame *object = 0;                                      \
  if (lua_type(L, 1) == LUA_TTABLE) {                              \
    lua_rawgeti(L, 1, 0);                                          \
    object = static_cast<CGMinimapFrame *>(lua_touserdata(L, -1)); \
    lua_pop(L, 1);                                                 \
  } else {                                                         \
    return luaL_error(                                             \
        L,                                                         \
        "Attempt to find 'this' in non-table object (used '.' "    \
        "instead of ':' ?)"                                        \
    );                                                             \
  }                                                                \
  FATALASSERT(object)

static int CGMinimapFrame_GetZoom(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(MinimapGetZoom()));
  return 1;
}

static int CGMinimapFrame_SetZoom(lua_State *L) {
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetZoom(level)");
  }
  MinimapSetZoom(static_cast<unsigned int>(lua_tonumber(L, 2)));
  return 0;
}

static int CGMinimapFrame_PingLocation(lua_State *L);
static int CGMinimapFrame_GetPingPosition(lua_State *L);

static FrameScript_Method CGMinimapFrameMethods[5] = {
    {  "GetZoomLevels",   CGMinimapFrame_GetZoomLevels},
    {        "GetZoom",         CGMinimapFrame_GetZoom},
    {        "SetZoom",         CGMinimapFrame_SetZoom},
    {   "PingLocation",    CGMinimapFrame_PingLocation},
    {"GetPingPosition", CGMinimapFrame_GetPingPosition}
};

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CGMinimapFrame::s_scriptMethods;

static int CGMinimapFrame_PingLocation(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  float x = 0.0f;
  float y = 0.0f;
  float viewSize = MinimapGetViewRadius() * 2.0f;
  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    GET_MINIMAP_THIS(L, object);
    y = -static_cast<float>(lua_tonumber(L, 2)) * 0.0009765625f * 0.8f / object->GetHeight() * viewSize;
    x = static_cast<float>(lua_tonumber(L, 3)) * 0.0009765625f * 0.8f / object->GetWidth() * viewSize;
  }

  NTempest::C3Vector playerPosition = player->GetPosition();
  x += playerPosition.x;
  y += playerPosition.y;

  if (CGGameUI::GetPartyMember(0)) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(MSG_MINIMAP_PING));
    msg.Put(x);
    msg.Put(y);
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    NTempest::C2Vector position(x, y);
    CGMinimapFrame::SetPingPosition(player->GetGUID(), position);
  }
  return 0;
}

static int CGMinimapFrame_GetPingPosition(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    NTempest::C2Vector diff = CGMinimapFrame::GetPingPosition() - static_cast<NTempest::C2Vector>(player->GetPosition());
    lua_pushnumber(L, -diff.y / (MinimapGetViewRadius() * 2.0f));
    lua_pushnumber(L, diff.x / (MinimapGetViewRadius() * 2.0f));
  } else {
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
  }
  return 2;
}

#undef GET_MINIMAP_THIS

void CGMinimapFrame::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGMinimapFrameMethods, 5, s_scriptMethods);
}

void CGMinimapFrame::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

int CGMinimapFrame::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CSimpleFrame::LookupScriptMethod(L, name);
}
