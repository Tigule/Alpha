#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Gx/Gx.h"

CMapObjDefGroup::~CMapObjDefGroup() {
  ASSERT(entityLinkList.Head() == 0);
  ASSERT(sceneLink.IsLinked() == 0);
}

void CMapObjDefGroup::SelectLights() {
  flags &= ~1u;

  GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);

  CMapBaseObjLink *link = lightLinkList.Head();
  unsigned int     whichLight = 1;

  while (link && whichLight < 8) {
    CMapLight *light = static_cast<CMapLight *>(link->owner);
    GxLightSet(whichLight, light->gxLight, CWorldScene::camPos);
    link = lightLinkList.Next(link);
    ++whichLight;
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
}

void CMapObjDefGroup::UpdateLights() {
  flags |= 1u;

  CMapBaseObjLink *link = doodadDefLinkList.Head();
  while (link) {
    link->owner->flags |= 1u;
    link = doodadDefLinkList.Next(link);
  }

  link = entityLinkList.Head();
  while (link) {
    link->owner->flags |= 1u;
    link = entityLinkList.Next(link);
  }
}

void CMapObjDefGroup::Update(NTempest::C44Matrix &newMat) {
  flags |= CMapBaseObj::Flag_LightUpdate;

  CMapBaseObjLink *link = doodadDefLinkList.Head();
  while (link) {
    static_cast<CMapDoodadDef *>(link->owner)->Update(newMat);
    link = doodadDefLinkList.Next(link);
  }

  link = entityLinkList.Head();
  while (link) {
    link->owner->flags |= CMapBaseObj::Flag_LightUpdate;
    link = entityLinkList.Next(link);
  }
}

CMapObjDef::~CMapObjDef() {
  ASSERT(refCount == 0);
}
