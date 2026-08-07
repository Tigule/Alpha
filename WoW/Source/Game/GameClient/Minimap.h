#ifndef WOW_SOURCE_GAME_GAMECLIENT_MINIMAP_H
#define WOW_SOURCE_GAME_GAMECLIENT_MINIMAP_H

#include <Base/Handle.h>
#include <Gx/Gx.h>
#include <Services/Texture.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <stpl.h>

class AreaPOIRec;

struct MinimapTexParams {
  BYTE                inside;
  CGxTex             *texture;
  BYTE                updateTexture;
  float               size;
  NTempest::C44Matrix worldRotation;
  NTempest::C44Matrix invMapObjMtx;
  NTempest::C3Vector  localCenter;
  NTempest::C3Vector  localOffset;
  BYTE                asyncTexWait;
};

struct QUADDATA {
  NTempest::C3Vector  verts[4];
  NTempest::C2Vector  texCoords[4];
  NTempest::C2Vector  maskTexCoords[4];
  NTempest::CAaBox    aaBox;
  QUADDATA           *rLink;
  float               sortz;
  HTEXTURE            m_texture;
  UINT                m_flags;
  NTempest::C2iVector m_areaNum;
  UINT                groupNum;

  QUADDATA();

  void            Render(UINT quad, const NTempest::CImVector &color) const;
  NTempest::CRect NormalizeToQuad(UINT quad, NTempest::CRect clippedRect);
  void            GenerateVertTexInfo(
      const NTempest::CRect    &rect,
      UINT                      quad,
      const NTempest::C2Vector &centerPoint,
      float                     radius,
      const NTempest::CRect    &maskBox,
      float                     layoutScale
  );
  void UpdateData(UINT quad, const NTempest::C2Vector centerPoint, float radius, float layoutScale);
};

struct POIDIRECTIONDATA {
  char  POIName[64];
  float rotation;
};

struct PARTYMEMBERINFO {
  DWORDLONG          guid;
  char               name[48];
  int                showArrow;
  int                showBlip;
  NTempest::C2Vector position;
  float              rotation;
};

int  MinimapInitialize(int continentID);
void MinimapShutdown();
BOOL MinimapUpdate(
    DWORD                     hWorldObject,
    UINT                      continent,
    const NTempest::C3Vector &pos,
    NTempest::C2Vector       &centerPoint,
    float                    &radius,
    QUADDATA                 *quads,
    MinimapTexParams         &mmtp
);
void                                       MinimapSetZoom(UINT zoomFactor);
UINT                                       MinimapGetZoom();
UINT                                       MinimapGetZoomLevels();
float                                      MinimapGetViewRadius();
const TSGrowableArray<const AreaPOIRec *> &MinimapGetPOI(int &updatePOI);
BOOL                                       MinimapGetDistantPOI(TSGrowableArray<POIDIRECTIONDATA> &directionData);
float                                      MinimapGetWorldRadius();
void                                       MinimapSetQuestPOI(float x, float y, int priority, LPCSTR name);
void                                       MinimapGetPartyMembers(PARTYMEMBERINFO *array);

#endif
