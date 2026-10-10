#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <BLPFile/blp.h>
#include "Object/ObjectClient/Unit_C.h"

#include "../../Common/ComponentCore/ComponentCore.cpp"

BOOL GetObjComponentInfo(
    int     race,
    int     sex,
    int     displayID,
    int     inventoryType,
    bool    isPlayer,
    bool    useAlternate,
    HMODEL *models,
    int    *attachmentPoints
) {
  FATALASSERT(models);
  FATALASSERT(attachmentPoints);

  SUBCOMPONENTDESC subComponents[2];
  int numSubComponents = CompUtilGetObjComponents(g_itemDisplayInfoDB.GetRecord(displayID), inventoryType, subComponents, 2, useAlternate);
  if (!numSubComponents) {
    return 0;
  }

  if (isPlayer && inventoryType == INDEX_HEAD_TYPE) {
    DecorateComponentFileNames(subComponents, numSubComponents, race, sex);
  }
  AddSubcomponentPrefixes(subComponents, numSubComponents, inventoryType);

  int added = 0;
  for (int componentIndex = 0; componentIndex < numSubComponents; ++componentIndex) {
    HMODEL model = ObjComponentBuildSubComponent(&subComponents[componentIndex], g_itemDisplayInfoDB.GetRecord(displayID));
    if (model) {
      models[added] = model;
      attachmentPoints[added] = subComponents[componentIndex].connectionPointIndex;
      ++added;
    }
  }
  return added;
}

bool ComponentApplyTabardTexture(HTEXCOMPONENT component, int eStyle, int eColor, int bStyle, int bColor, int b) {
  CTexComponent *componentptr = (CTexComponent *)component;
  VALIDATEBEGIN;
  VALIDATE(componentptr);
  VALIDATEEND;

  if (eStyle == componentptr->m_emblemStyle && eColor == componentptr->m_emblemColor && bStyle == componentptr->m_borderStyle &&
      bColor == componentptr->m_borderColor && b == componentptr->m_background)
  {
    return true;
  }

  componentptr->m_emblemStyle = eStyle;
  componentptr->m_emblemColor = eColor;
  componentptr->m_borderStyle = bStyle;
  componentptr->m_borderColor = bColor;
  componentptr->m_background = b;

  if (eStyle == -1 || eColor == -1 || bStyle == -1 || bColor == -1 || b == -1) {
    return false;
  }

  for (int i = 0; i < 2; ++i) {
    componentptr->m_dirtyFlags |= 1 << s_tabardSections[i];
  }

  return true;
}

void GetTabardBackgroundFileName(int section, int background, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Background_%02d%s_U", background, s_tabardSectionSuffix[section]);
}

void GetTabardEmblemFileName(int section, int emblem, int color, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Emblem_%02d_%02d%s_U", emblem, color, s_tabardSectionSuffix[section]);
}

void GetTabardBorderFileName(int section, int border, int color, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Border_%02d_%02d%s_U", border, color, s_tabardSectionSuffix[section]);
}

void ComponentRemoveTabardTexture(int sex, HTEXCOMPONENT component, const ItemDisplayInfoRec *displayInfo, int inventoryType) {
  CTexComponent *componentptr = (CTexComponent *)component;
  VALIDATEBEGIN;
  VALIDATE(componentptr);
  VALIDATEENDVOID;

  if (displayInfo) {
    CStatus status;
    TexComponentAdd(&status, sex, component, displayInfo, inventoryType, 0);
    componentptr->m_emblemStyle = -1;
    componentptr->m_emblemColor = -1;
    componentptr->m_borderStyle = -1;
    componentptr->m_borderColor = -1;
    componentptr->m_background = -1;
  }
}

bool CTexComponent::HasTabard() const {
  return m_emblemStyle != -1 && m_emblemColor != -1 && m_borderStyle != -1 && m_borderColor != -1 && m_background != -1;
}

void ComponentForceTabardDraw(HTEXCOMPONENT component) {
  CTexComponent *componentptr = (CTexComponent *)component;
  VALIDATEBEGIN;
  VALIDATE(componentptr);
  VALIDATEENDVOID;
  componentptr->m_flags |= 2;
  componentptr->m_dirtyFlags |= 0x60;
}

void TexComponentCopy(HTEXCOMPONENT d, HTEXCOMPONENT s) {
  if (d && s) {
    *(CTexComponent *)d = *(CTexComponent *)s;
  }
}

CTexComponent &CTexComponent::operator=(const CTexComponent &rhs) {
  if (this != &rhs) {
    m_emblemStyle = rhs.m_emblemStyle;
    m_emblemColor = rhs.m_emblemColor;
    m_borderStyle = rhs.m_borderStyle;
    m_borderColor = rhs.m_borderColor;
    m_background = rhs.m_background;
    SStrPrintf(m_upperFaceTexture, sizeof(m_upperFaceTexture), "%s", rhs.m_upperFaceTexture);
    SStrPrintf(m_lowerFaceTexture, sizeof(m_lowerFaceTexture), "%s", rhs.m_lowerFaceTexture);

    UINT i;
    for (i = 0; i < 2; ++i) {
      m_underwearHideCounts[i] = rhs.m_underwearHideCounts[i];
    }

    for (i = 0; i < NUM_TEXCOMPONENT_SECTIONS; ++i) {
      m_sections[i] = rhs.m_sections[i];
      m_dirtyFlags |= 1 << i;
    }

    m_flags |= rhs.m_flags;
  }

  return *this;
}

CSection &CSection::operator=(const CSection &rhs) {
  if (this != &rhs) {
    for (UINT i = 0; i < NUM_TEXLAYERS; ++i) {
      m_layers[i] = rhs.m_layers[i];
    }
  }

  return *this;
}

CTextureLayer &CTextureLayer::operator=(const CTextureLayer &rhs) {
  if (this != &rhs) {
    for (UINT i = 0; i < NUM_LAYERPRIORITIES; ++i) {
      m_priorities[i] = rhs.m_priorities[i];
    }
  }

  return *this;
}

CTexturePiece &CTexturePiece::operator=(const CTexturePiece &rhs) {
  if (this != &rhs) {
    if (m_mippedTexture) {
      HandleClose(m_mippedTexture);
    }

    m_mippedTexture = (HMIPPEDTEXTURE)HandleDuplicate(rhs.m_mippedTexture);
    m_textureInfo = rhs.m_textureInfo;
    m_holds = rhs.m_holds;
  }

  return *this;
}
