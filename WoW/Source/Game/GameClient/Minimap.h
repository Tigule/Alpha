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
  unsigned int        inside;
  CGxTex             *texture;
  unsigned int        updateTexture;
  float               size;
  NTempest::C44Matrix worldRotation;
  NTempest::C44Matrix invMapObjMtx;
  NTempest::C3Vector  localCenter;
  NTempest::C3Vector  localOffset;
  unsigned int        asyncTexWait;
};

struct QUADDATA {
  NTempest::C3Vector  verts[4];
  NTempest::C2Vector  texCoords[4];
  NTempest::C2Vector  maskTexCoords[4];
  NTempest::CAaBox    aaBox;
  QUADDATA           *rLink;
  float               sortz;
  HTEXTURE            m_texture;
  unsigned int        m_flags;
  NTempest::C2iVector m_areaNum;
  unsigned int        groupNum;

  QUADDATA() : rLink(0), sortz(0.0f), m_texture(0), m_flags(0), m_areaNum(-1), groupNum(0) {
  }

  void            Render(unsigned int quad, const NTempest::CImVector &color) const;
  NTempest::CRect NormalizeToQuad(unsigned int quad, NTempest::CRect clippedRect);
  void            GenerateVertTexInfo(
      const NTempest::CRect    &rect,
      unsigned int              quad,
      const NTempest::C2Vector &centerPoint,
      float                     radius,
      const NTempest::CRect    &maskBox,
      float                     layoutScale
  );
  void UpdateData(unsigned int quad, NTempest::C2Vector centerPoint, float radius, float layoutScale);
};

struct POIDIRECTIONDATA {
  char  POIName[64];
  float rotation;
};

struct PARTYMEMBERINFO {
  unsigned __int64   guid;
  char               name[48];
  int                showArrow;
  int                showBlip;
  NTempest::C2Vector position;
  float              rotation;
};

int __fastcall  MinimapInitialize(int continentID);
void __fastcall MinimapShutdown();
int __fastcall  MinimapUpdate(
    unsigned long             hWorldObject,
    unsigned int              continent,
    const NTempest::C3Vector &pos,
    NTempest::C2Vector       &centerPoint,
    float                    &radius,
    QUADDATA                 *quads,
    MinimapTexParams         &mmtp
);
void __fastcall                                 MinimapSetZoom(unsigned int zoomFactor);
unsigned int __fastcall                         MinimapGetZoom();
unsigned int __fastcall                         MinimapGetZoomLevels();
float __fastcall                                MinimapGetViewRadius();
TSGrowableArray<const AreaPOIRec *> &__fastcall MinimapGetPOI(int &updatePOI);
int __fastcall                                  MinimapGetDistantPOI(TSGrowableArray<POIDIRECTIONDATA> &directionData);
float __fastcall                                MinimapGetWorldRadius();
void __fastcall                                 MinimapSetQuestPOI(float x, float y, int priority, const char *name);
void __fastcall                                 MinimapGetPartyMembers(PARTYMEMBERINFO *array);

#endif
