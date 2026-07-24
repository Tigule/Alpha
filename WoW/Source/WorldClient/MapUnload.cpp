#include "WorldClient/World.h"

#include "Base/Handle.h"
#include "DayNight.h"
#include "WorldClient/CMapObj.h"

void __fastcall CMap::Unload() {
  Purge();

  for (unsigned int i = 0; i < 4096; ++i) {
    if (areaLowTable[i]) {
      DEL(areaLowTable[i]);
      areaLowTable[i] = 0;
    }
  }

  doodadNames.Clear();
  doodadNamesIndex.Clear();
  mapObjNames.Clear();
  mapObjNamesIndex.Clear();

  CMapBaseObjLink *link = mapObjDefLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = mapObjDefLinkList.RawNext(link);
    CMapObjDef      *mapObjDef = static_cast<CMapObjDef *>(link->owner);
    FreeBaseObjLink(link);
    PurgeMapObjDef(mapObjDef);
    link = next;
  }

  link = doodadDefLinkList.Head();
  while (reinterpret_cast<long>(link) > 0) {
    CMapBaseObjLink *next = doodadDefLinkList.RawNext(link);
    CMapDoodadDef   *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
    FreeBaseObjLink(link);
    PurgeDoodadDef(doodadDef);
    link = next;
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
    HandleClose(reinterpret_cast<HOBJECT>(m_texid));
  }
}

void DNPlanet::Destroy() {
  if (m_texid) {
    HandleClose(reinterpret_cast<HOBJECT>(m_texid));
  }
}

void DNStars::Destroy() {
  if (m_hModel) {
    HandleClose(reinterpret_cast<HOBJECT>(m_hModel));
  }
}

CMapAreaLow::~CMapAreaLow() {
}
