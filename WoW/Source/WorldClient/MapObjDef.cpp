#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Gx/Gx.h"

CMapObjDefGroup::CMapObjDefGroup() {
  type |= Type_MapObjDefGroup;
  subzoneName = 0;
  level = 0;
}

CMapObjDefGroup::~CMapObjDefGroup() {
  ASSERT(entityLinkList.Head() == 0);
  ASSERT(sceneLink.IsLinked() == 0);
}

void CMapObjDefGroup::SelectLights() {
  flags &= ~1u;

  GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);

  unsigned int     whichLight = 1;

  ITERATELIST(CMapBaseObjLink, lightLinkList, link) {
    if (whichLight >= 8) {
      break;
    }
    CMapLight *light = static_cast<CMapLight *>(link->owner);
    GxLightSet(whichLight, light->gxLight, CWorldScene::camPos);
    ++whichLight;
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
}

void CMapObjDefGroup::UpdateLights() {
  flags |= 1u;

  {
    ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
      link->owner->flags |= 1u;
    }
  }

  {
    ITERATELIST(CMapBaseObjLink, entityLinkList, link) {
      link->owner->flags |= 1u;
    }
  }
}

void CMapObjDefGroup::Update(const NTempest::C44Matrix &newMat) {
  flags |= CMapBaseObj::Flag_LightUpdate;

  {
    ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
      static_cast<CMapDoodadDef *>(link->owner)->Update(newMat);
    }
  }

  {
    ITERATELIST(CMapBaseObjLink, entityLinkList, link) {
      link->owner->flags |= CMapBaseObj::Flag_LightUpdate;
    }
  }
}

CMapObjDef::CMapObjDef() {
  type |= Type_MapObjDef;
  nameId = 0;
  mapObj = 0;
  zoneName = 0;
  param64 = 0;
}

CMapObjDef::~CMapObjDef() {
  ASSERT(refCount == 0);
}
