#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "Base/Handle.h"

void CMap::Unload() {
  Purge();

  for (UINT i = 0; i < 4096; ++i) {
    if (areaLowTable[i]) {
      DEL(areaLowTable[i]);
      areaLowTable[i] = 0;
    }
  }

  doodadNames.Clear();
  doodadNamesIndex.Clear();
  mapObjNames.Clear();
  mapObjNamesIndex.Clear();

  CMapBaseObjLink *link;
  CMapBaseObjLink *next;
  for (link = mapObjDefLinkList.Head(); (int)link > 0 ? (next = mapObjDefLinkList.RawNext(link), 1) : 0; link = next) {
    CMapObjDef *mapObjDef = (CMapObjDef *)link->owner;
    FreeBaseObjLink(link);
    PurgeMapObjDef(mapObjDef);
  }

  for (link = doodadDefLinkList.Head(); (int)link > 0 ? (next = doodadDefLinkList.RawNext(link), 1) : 0; link = next) {
    CMapDoodadDef *doodadDef = (CMapDoodadDef *)link->owner;
    FreeBaseObjLink(link);
    PurgeDoodadDef(doodadDef);
  }

  FATALASSERT(doodadDefLinkList.Head() == 0);
  FATALASSERT(mapObjDefLinkList.Head() == 0);

  CMapObj::ClearCache(1);
  if (wdtFile) {
    SFile::Close(wdtFile);
  }
  wdtFile = 0;
  DayNightDestroy();
  DestroyLight(sunLight);
  bActive = 0;
  bDungeon = 0;
}

void DNGlare::Destroy() {
  if (m_texid) {
    HandleClose(m_texid);
  }
}

void DNPlanet::Destroy() {
  if (m_texid) {
    HandleClose(m_texid);
  }
}

void DNStars::Destroy() {
  if (m_hModel) {
    HandleClose(m_hModel);
  }
}
