#include <Component/Component.h>
#include <Component/CharacterCustomization.h>

#include <Base/Status.h>
#include <Model/IModel.h>
#include <Services/Texture.h>
#include <Services/SysMessage.h>

#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/HelmetGeosetVisDataRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/AnimCompiles.h"

#include <string.h>
#include <storm.h>

static const unsigned int s_tabardSectionFlags = 0x60;
static const char        *s_tabardSectionSuffix[NUM_TEXCOMPONENT_SECTIONS] = {0, 0, 0, 0, 0, 0, "_TU", "_TL", 0, 0};

static HTEXTURECACHE         s_textureCacheHandle;
static unsigned int          s_numSectionsMask;
static const char           *s_boneNames[3] = {"$WTB", "$WTT", "$CCH"};
static const unsigned int    NUM_UNDERWEARHIDESECTIONS = 2;
static TEXCOMPONENT_SECTIONS s_underwearSections[2] = {TCS_UPPERTORSO, TCS_LEGUPPER};
static TEXCOMPONENT_SECTIONS s_tabardSections[2] = {TCS_UPPERTORSO, TCS_LOWERTORSO};

static HMODEL __fastcall ObjComponentBuildSubComponent(SUBCOMPONENTDESC *subComponent, const ItemDisplayInfoRec *displayInfoRec);
static void __fastcall   AddSubcomponentPrefixes(SUBCOMPONENTDESC *subcomponents, unsigned int numSubComponents, unsigned int inventoryType);
static void __fastcall
DecorateComponentFileNames(SUBCOMPONENTDESC *subComponents, unsigned int numSubComponents, unsigned int race, unsigned int sex);

MipBits    *CTexturePiece::m_destImage;
TEXTUREINFO CTexturePiece::m_destTextureInfo;

static void __fastcall PasteOpaque(
    MipBits            *dstMips,
    const MipBits      *srcMips,
    NTempest::C2iVector dstPos,
    NTempest::C2iVector srcPos,
    unsigned int        width,
    unsigned int        height,
    unsigned int        levels,
    unsigned int        dstPitch,
    unsigned int        srcPitch
) {
  unsigned int byteWidth;
  unsigned int level;

  ASSERT(dstMips);
  ASSERT(srcMips);
  ASSERT(width);
  ASSERT(height);
  ASSERT(levels);
  ASSERT(dstPitch);
  ASSERT(srcPitch);

  byteWidth = 4 * width;
  for (level = 0; level < levels; ++level) {
    unsigned char       *dstLine = reinterpret_cast<unsigned char *>(dstMips->mip[level]) + dstPos.y * dstPitch + 4 * dstPos.x;
    const unsigned char *srcLine = reinterpret_cast<const unsigned char *>(srcMips->mip[level]) + srcPos.y * srcPitch + 4 * srcPos.x;
    unsigned int         row;

    for (row = 0; row < height; ++row) {
      memcpy(dstLine, srcLine, byteWidth);
      dstLine += dstPitch;
      srcLine += srcPitch;
    }

    byteWidth = max(byteWidth >> 1, 4U);
    height = max(height >> 1, 1U);
    dstPos.x >>= 1;
    dstPos.y >>= 1;
    srcPos.x >>= 1;
    srcPos.y >>= 1;
    dstPitch >>= 1;
    srcPitch >>= 1;
  }
}

static void __fastcall PasteTransparentOneBit(
    MipBits            *dstMips,
    const MipBits      *srcMips,
    NTempest::C2iVector dstPos,
    NTempest::C2iVector srcPos,
    unsigned int        width,
    unsigned int        height,
    unsigned int        levels,
    unsigned int        dstPitch,
    unsigned int        srcPitch
) {
  unsigned int level;

  ASSERT(dstMips);
  ASSERT(srcMips);
  ASSERT(width);
  ASSERT(height);
  ASSERT(levels);
  ASSERT(dstPitch);
  ASSERT(srcPitch);

  for (level = 0; level < levels; ++level) {
    C4Pixel       *dstLine = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(dstMips->mip[level]) + dstPos.y * dstPitch + 4 * dstPos.x);
    const C4Pixel *srcLine =
        reinterpret_cast<const C4Pixel *>(reinterpret_cast<const unsigned char *>(srcMips->mip[level]) + srcPos.y * srcPitch + 4 * srcPos.x);
    unsigned int row;

    for (row = 0; row < height; ++row) {
      C4Pixel       *dstPixel = dstLine;
      const C4Pixel *srcPixel = srcLine;
      unsigned int   column;

      for (column = 0; column < width; ++column) {
        if (srcPixel->a) {
          dstPixel->b = srcPixel->b;
          dstPixel->g = srcPixel->g;
          dstPixel->r = srcPixel->r;
        }

        ++dstPixel;
        ++srcPixel;
      }

      dstLine = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(dstLine) + dstPitch);
      srcLine = reinterpret_cast<const C4Pixel *>(reinterpret_cast<const unsigned char *>(srcLine) + srcPitch);
    }

    width = max(width >> 1, 1U);
    height = max(height >> 1, 1U);
    dstPos.x >>= 1;
    dstPos.y >>= 1;
    srcPos.x >>= 1;
    srcPos.y >>= 1;
    dstPitch >>= 1;
    srcPitch >>= 1;
  }
}

static void __fastcall PasteTransparentFull(
    MipBits            *dstMips,
    const MipBits      *srcMips,
    NTempest::C2iVector dstPos,
    NTempest::C2iVector srcPos,
    unsigned int        width,
    unsigned int        height,
    unsigned int        levels,
    unsigned int        dstPitch,
    unsigned int        srcPitch
) {
  unsigned int level;

  ASSERT(dstMips);
  ASSERT(srcMips);
  ASSERT(width);
  ASSERT(height);
  ASSERT(levels);
  ASSERT(dstPitch);
  ASSERT(srcPitch);

  for (level = 0; level < levels; ++level) {
    C4Pixel       *dstLine = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(dstMips->mip[level]) + dstPos.y * dstPitch + 4 * dstPos.x);
    const C4Pixel *srcLine =
        reinterpret_cast<const C4Pixel *>(reinterpret_cast<const unsigned char *>(srcMips->mip[level]) + srcPos.y * srcPitch + 4 * srcPos.x);
    unsigned int row;

    for (row = 0; row < height; ++row) {
      C4Pixel       *dstPixel = dstLine;
      const C4Pixel *srcPixel = srcLine;
      unsigned int   column;

      for (column = 0; column < width; ++column) {
        unsigned int alpha = srcPixel->a;

        if (alpha == 0xFF) {
          dstPixel->b = srcPixel->b;
          dstPixel->g = srcPixel->g;
          dstPixel->r = srcPixel->r;
        } else if (alpha) {
          dstPixel->b += static_cast<unsigned short>(alpha * (srcPixel->b - dstPixel->b)) >> 8;
          dstPixel->g += static_cast<unsigned short>(alpha * (srcPixel->g - dstPixel->g)) >> 8;
          dstPixel->r += static_cast<unsigned short>(alpha * (srcPixel->r - dstPixel->r)) >> 8;
        }

        ++dstPixel;
        ++srcPixel;
      }

      dstLine = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(dstLine) + dstPitch);
      srcLine = reinterpret_cast<const C4Pixel *>(reinterpret_cast<const unsigned char *>(srcLine) + srcPitch);
    }

    width = max(width >> 1, 1U);
    height = max(height >> 1, 1U);
    dstPos.x >>= 1;
    dstPos.y >>= 1;
    srcPos.x >>= 1;
    srcPos.y >>= 1;
    dstPitch >>= 1;
    srcPitch >>= 1;
  }
}

void CTextureLayer::AllocBlankTexture(
    TEXCOMPONENT_SECTIONS section,
    CStatus              *status,
    TEXCOMPONENT_LAYERS   layer,
    EGxTexFormat          format,
    unsigned int          width,
    unsigned int          height,
    int                   opaque
) {
  int priority;

  for (priority = LAYERPRIORITY_1; priority < NUM_LAYERPRIORITIES; ++priority) {
    m_priorities[priority].SetTexture(section, layer, static_cast<LAYERPRIORITY>(priority), status, 0, 0, width, height);
  }
}

int CTextureLayer::IsOpaque() const {
  int priority;

  for (priority = NUM_LAYERPRIORITIES - 1; priority >= LAYERPRIORITY_0; --priority) {
    const CTexturePiece &piece = m_priorities[priority];

    if (piece.HasImage() && !piece.HasHolds()) {
      return piece.IsOpaque();
    }
  }

  return 0;
}

int CTextureLayer::SetTexture(
    TEXCOMPONENT_SECTIONS section,
    TEXCOMPONENT_LAYERS   layer,
    LAYERPRIORITY         priority,
    CStatus              *status,
    int                   checkExistingTexture,
    const char           *fileName,
    unsigned int          expectedWidth,
    unsigned int          expectedHeight
) {
  return m_priorities[priority].SetTexture(section, layer, priority, status, checkExistingTexture, fileName, expectedWidth, expectedHeight);
}

int CTexturePiece::SetTexture(
    TEXCOMPONENT_SECTIONS section,
    TEXCOMPONENT_LAYERS   layer,
    LAYERPRIORITY         priority,
    CStatus              *status,
    int                   checkExistingTexture,
    const char           *fileName,
    unsigned int          expectedWidth,
    unsigned int          expectedHeight
) {
  if (checkExistingTexture && fileName && *fileName && m_mippedTexture) {
    status->Add(STATUS_WARNING, "sec%d layer%d pri%d file\"%s\"", section, layer, priority, m_fileName);
    return 0;
  }

  if (m_mippedTexture) {
    HandleClose(m_mippedTexture);
    m_mippedTexture = 0;
  }

  m_textureInfo.opaque = 0;
  if (!fileName || !*fileName) {
    return 0;
  }

  m_mippedTexture = TextureCacheGetTexture(s_textureCacheHandle, fileName, &m_textureInfo);
  if (!m_mippedTexture) {
    return 0;
  }

  SStrPrintf(m_fileName, sizeof(m_fileName), "%s", fileName);
  return 1;
}

void CTexturePiece::SetTexture(int checkExistingTexture, HTEXTURE texture) {
  ASSERT(!checkExistingTexture || !texture || !m_mippedTexture);

  if (m_mippedTexture) {
    HandleClose(m_mippedTexture);
    m_mippedTexture = 0;
  }

  m_textureInfo.opaque = 1;
}

void CTexturePiece::PasteOpaque(const CTexturePiece &source, NTempest::C2iVector dstPos, NTempest::C2iVector srcPos, NTempest::C2iVector size) {
  const MipBits *srcMips = TextureCacheGetImage(source.m_mippedTexture);
  MipBits       *dstMips = m_destImage;
  unsigned int   levels;

  ASSERT(srcMips);
  ASSERT(dstMips);
  ASSERT(size.x <= (int)CTexturePiece::m_destTextureInfo.width);
  ASSERT(size.y <= (int)CTexturePiece::m_destTextureInfo.height);

  levels = min(source.m_textureInfo.levels, m_destTextureInfo.levels);
  ::PasteOpaque(dstMips, srcMips, dstPos, srcPos, size.x, size.y, levels, 4 * m_destTextureInfo.width, 4 * source.m_textureInfo.width);
}

void CTexturePiece::PasteTransparentOneBit(
    const CTexturePiece &source,
    NTempest::C2iVector  dstPos,
    NTempest::C2iVector  srcPos,
    NTempest::C2iVector  size
) {
  const MipBits *srcMips = TextureCacheGetImage(source.m_mippedTexture);
  MipBits       *dstMips = m_destImage;
  unsigned int   levels;

  ASSERT(srcMips);
  ASSERT(dstMips);
  ASSERT(size.x <= (int)CTexturePiece::m_destTextureInfo.width);
  ASSERT(size.y <= (int)CTexturePiece::m_destTextureInfo.height);

  levels = min(source.m_textureInfo.levels, m_destTextureInfo.levels);
  ::PasteTransparentOneBit(dstMips, srcMips, dstPos, srcPos, size.x, size.y, levels, 4 * m_destTextureInfo.width, 4 * source.m_textureInfo.width);
}

void CTexturePiece::PasteTransparentFull(
    const CTexturePiece &source,
    NTempest::C2iVector  dstPos,
    NTempest::C2iVector  srcPos,
    NTempest::C2iVector  size
) {
  const MipBits *srcMips = TextureCacheGetImage(source.m_mippedTexture);
  MipBits       *dstMips = m_destImage;
  unsigned int   levels;

  ASSERT(srcMips);
  ASSERT(dstMips);
  ASSERT(size.x <= (int)CTexturePiece::m_destTextureInfo.width);
  ASSERT(size.y <= (int)CTexturePiece::m_destTextureInfo.height);

  levels = min(source.m_textureInfo.levels, m_destTextureInfo.levels);
  ::PasteTransparentFull(dstMips, srcMips, dstPos, srcPos, size.x, size.y, levels, 4 * m_destTextureInfo.width, 4 * source.m_textureInfo.width);
}

int CTexturePiece::Paste(const CTexturePiece &source, int x, int y) {
  unsigned int        width = source.m_textureInfo.width;
  unsigned int        height = source.m_textureInfo.height;
  NTempest::C2iVector dstPos(x, y);
  NTempest::C2iVector srcPos(0, 0);
  NTempest::C2iVector size(width, height);

  ASSERT(width);
  ASSERT(height);

  if (source.IsOpaque()) {
    PasteOpaque(source, dstPos, srcPos, size);
  } else if (source.m_textureInfo.alphaBits == 1) {
    PasteTransparentOneBit(source, dstPos, srcPos, size);
  } else {
    PasteTransparentFull(source, dstPos, srcPos, size);
  }

  return 1;
}

int CTexturePiece::Paste(const CTexturePiece &source, int x, int y, int width, int height) {
  NTempest::C2iVector dstPos(x, y);
  NTempest::C2iVector srcPos(x, y);
  NTempest::C2iVector size(width, height);

  if (source.IsOpaque()) {
    PasteOpaque(source, dstPos, srcPos, size);
  } else if (source.m_textureInfo.alphaBits == 1) {
    PasteTransparentOneBit(source, dstPos, srcPos, size);
  } else {
    PasteTransparentFull(source, dstPos, srcPos, size);
  }

  return 1;
}

void CTexComponent::UpdateSections(CStatus *status, int bUpdate) {
  TEXCOMPONENT_SECTIONS section;

  for (section = TCS_UPPERARM; section < NUM_TEXCOMPONENT_SECTIONS; section = static_cast<TEXCOMPONENT_SECTIONS>(section + 1)) {
    if (m_dirtyFlags & (1 << section)) {
      UpdateSection(status, section, bUpdate);
    }
  }

  m_dirtyFlags = 0;
}

int CTexComponent::CheckSections(int bForce) {
  TEXCOMPONENT_SECTIONS section;

  for (section = TCS_UPPERARM; section < NUM_TEXCOMPONENT_SECTIONS; section = static_cast<TEXCOMPONENT_SECTIONS>(section + 1)) {
    if ((m_dirtyFlags & (1 << section)) && !CheckSection(section, bForce)) {
      return 0;
    }
  }

  return 1;
}

bool CTexComponent::IsTabardSectionLayerAndPriority(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority) const {
  if (!HasTabard() || !((1 << section) & s_tabardSectionFlags)) {
    return false;
  }

  if (m_flags & 2) {
    return true;
  }

  if (layer != g_sectionLayers[INDEX_TABARD_TYPE].layers[section] || priority != g_sectionPriorities[INDEX_TABARD_TYPE].priorities[section]) {
    return false;
  }

  const CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];
  return piece.HasImage() && !piece.HasHolds();
}

void CTexComponent::PasteTabardTexture(CStatus *status, TEXCOMPONENT_SECTIONS section) {
  char                emblemName[MAX_PATH];
  char                backgroundName[MAX_PATH];
  char                borderName[MAX_PATH];
  CTexturePiece       background;
  CTexturePiece       emblem;
  CTexturePiece       border;
  TEXCOMPONENT_LAYERS layer;
  unsigned int        x;
  unsigned int        width;
  unsigned int        y;
  LAYERPRIORITY       priority;
  unsigned int        height;

  if (!CompUtilGetSectionOffset(section, &x, &y) || !CompUtilGetSectionDimensions(section, &width, &height)) {
    return;
  }

  layer = g_sectionLayers[INDEX_TABARD_TYPE].layers[section];
  priority = g_sectionPriorities[INDEX_TABARD_TYPE].priorities[section];
  CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];

  GetTabardBackgroundFileName(section, m_background, backgroundName, sizeof(backgroundName));
  GetTabardEmblemFileName(section, m_emblemStyle, m_emblemColor, emblemName, sizeof(emblemName));
  GetTabardBorderFileName(section, m_borderStyle, m_borderColor, borderName, sizeof(borderName));

  background.SetTexture(section, layer, priority, status, 0, backgroundName, width, height);
  emblem.SetTexture(section, layer, priority, status, 0, emblemName, width, height);
  border.SetTexture(section, layer, priority, status, 0, borderName, width, height);

  if (!background.m_textureInfo.width) {
    TextureCacheGetInfo(background.m_mippedTexture, background.m_textureInfo, 1);
  }
  if (!emblem.m_textureInfo.width) {
    TextureCacheGetInfo(emblem.m_mippedTexture, emblem.m_textureInfo, 1);
  }
  if (!border.m_textureInfo.width) {
    TextureCacheGetInfo(border.m_mippedTexture, border.m_textureInfo, 1);
  }

  piece.Paste(background, x, y);
  piece.Paste(emblem, x, y);
  piece.Paste(border, x, y);
}

void CTexComponent::UpdateSection(CStatus *status, TEXCOMPONENT_SECTIONS section, int bUpdate) {
  unsigned int        width;
  unsigned int        height;
  unsigned int        y;
  unsigned int        x;
  TEXCOMPONENT_LAYERS layer;

  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);

  if (!CompUtilGetSectionOffset(section, &x, &y) || !CompUtilGetSectionDimensions(section, &width, &height)) {
    return;
  }

  layer = NUM_TEXLAYERS;
  for (;;) {
    layer = static_cast<TEXCOMPONENT_LAYERS>(layer - 1);
    if (m_sections[section].m_layers[layer].IsOpaque()) {
      break;
    }

    if (layer == TEXLAYER_SKIN) {
      layer = TEXLAYER_NONE;
      break;
    }
  }

  while (static_cast<unsigned int>(layer) < NUM_TEXLAYERS) {
    Paste(status, section, layer, x, y, width, height);
    layer = static_cast<TEXCOMPONENT_LAYERS>(layer + 1);
  }

  if (bUpdate) {
    CGxTex *texture = TextureGetGxTex(m_texture, 1, 0);
    GxTexUpdate(texture, x, y, x + width - 1, y + height - 1, 1);
  }
}

int CTexComponent::CheckSection(TEXCOMPONENT_SECTIONS section, int bForce) {
  unsigned int j;
  unsigned int priority;

  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);

  for (j = 0; j < NUM_TEXLAYERS; ++j) {
    for (priority = 0; priority < NUM_LAYERPRIORITIES; ++priority) {
      CTexturePiece &piece = m_sections[section].m_layers[j].m_priorities[priority];

      if (piece.m_mippedTexture && !piece.m_textureInfo.width && !TextureCacheGetInfo(piece.m_mippedTexture, piece.m_textureInfo, bForce)) {
        return 0;
      }
    }
  }

  return 1;
}

int CTexComponent::Paste(CStatus *status, TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, int x, int y, int width, int height) {
  CTextureLayer &source = m_sections[section].m_layers[layer];
  LAYERPRIORITY  priority = static_cast<LAYERPRIORITY>(NUM_LAYERPRIORITIES);

  do {
    if (priority == LAYERPRIORITY_0) {
      return 0;
    }
    priority = static_cast<LAYERPRIORITY>(priority - 1);

    if (IsTabardSectionLayerAndPriority(section, layer, priority)) {
      PasteTabardTexture(status, section);
      return 1;
    }
  } while (!CheckPastingRules(section, layer, priority));

  if (layer == TEXLAYER_SKIN && priority == LAYERPRIORITY_0) {
    return CTexturePiece::Paste(source.m_priorities[priority], x, y, width, height);
  }

  return CTexturePiece::Paste(source.m_priorities[priority], x, y);
}

int CTexComponent::CheckPastingRules(TEXCOMPONENT_SECTIONS section, TEXCOMPONENT_LAYERS layer, LAYERPRIORITY priority) {
  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  ASSERT(layer < NUM_TEXLAYERS);
  ASSERT(priority < NUM_LAYERPRIORITIES);

  CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];
  if (!piece.HasImage() || piece.HasHolds()) {
    return 0;
  }

  if (section != TCS_LEGUPPER) {
    return 1;
  }

  TEXCOMPONENT_LAYERS tabardLayer;
  LAYERPRIORITY       tabardPriority;
  if (!CompUtilItemSectionInfo(INDEX_TABARD_TYPE, TCS_LEGUPPER, &tabardLayer, &tabardPriority)) {
    return 1;
  }

  TEXCOMPONENT_LAYERS bodyLayer;
  LAYERPRIORITY       bodyPriority;
  if (!CompUtilItemSectionInfo(INDEX_BODY_TYPE, TCS_LEGUPPER, &bodyLayer, &bodyPriority)) {
    return 1;
  }

  TEXCOMPONENT_LAYERS chestLayer;
  LAYERPRIORITY       chestPriority;
  if (!CompUtilItemSectionInfo(INDEX_CHEST_TYPE, TCS_LEGUPPER, &chestLayer, &chestPriority)) {
    return 1;
  }

  if ((layer != tabardLayer || priority != tabardPriority) && (layer != bodyLayer || priority != bodyPriority)) {
    return 1;
  }

  return !m_sections[TCS_LEGUPPER].m_layers[chestLayer].m_priorities[chestPriority].HasImage();
}

void CTexComponent::BuildSkinPieces(CStatus *status, unsigned int *layerHoldSectionFlags) {
  unsigned int x;
  unsigned int y;
  const char  *fileName;
  unsigned int width;
  unsigned int height;

  fileName = TextureGetFilename(m_texture);
  FATALASSERT(fileName);

  unsigned int section;
  for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    int success = CompUtilGetSectionOffset(section, &x, &y);
    FATALASSERT(success);

    success = CompUtilGetSectionDimensions(section, &width, &height);
    FATALASSERT(success);

    m_sections[section].m_layers[TEXLAYER_SKIN].AllocBlankTexture(
        static_cast<TEXCOMPONENT_SECTIONS>(section), status, TEXLAYER_SKIN, GxTex_Argb8888, width, height, 1
    );
    m_sections[section].m_layers[TEXLAYER_SKIN].SetTexture(
        static_cast<TEXCOMPONENT_SECTIONS>(section), TEXLAYER_SKIN, LAYERPRIORITY_0, status, 0, fileName, 0, 0
    );
  }

  if (layerHoldSectionFlags) {
    unsigned int layer;
    for (layer = 0; layer < NUM_TEXLAYERS; ++layer) {
      if (layerHoldSectionFlags[layer]) {
        for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
          if (layerHoldSectionFlags[layer] & (1 << section)) {
            unsigned int priority;
            for (priority = 0; priority < NUM_LAYERPRIORITIES; ++priority) {
              m_sections[section].m_layers[layer].m_priorities[priority].SetHold(1);
            }
          }
        }
      }
    }
  }
}

void CTexComponent::BuildNakedPieces(CStatus *status, unsigned int race, unsigned int sex, unsigned int skinID, int isNPC) {
  char         buffer[128];
  unsigned int underwearSection;

  for (underwearSection = 0; underwearSection < NUM_UNDERWEARHIDESECTIONS; ++underwearSection) {
    if (CharCustomizationGetNakedSectionName(race, sex, skinID, underwearSection, buffer, sizeof(buffer), isNPC) && *buffer) {
      TEXCOMPONENT_SECTIONS section = s_underwearSections[underwearSection];
      m_sections[section].m_layers[TEXLAYER_SKIN].SetTexture(section, TEXLAYER_SKIN, LAYERPRIORITY_1, status, 0, buffer, 0, 0);
      ShowUnderwear(underwearSection);
    }
  }
}

void CTexComponent::HideUnderwear(unsigned int underwearSection) {
  ASSERT(underwearSection < NUM_UNDERWEARHIDESECTIONS);

  m_sections[s_underwearSections[underwearSection]].m_layers[TEXLAYER_SKIN].m_priorities[LAYERPRIORITY_1].ClearHold(2);
}

void CTexComponent::ShowUnderwear(unsigned int underwearSection) {
  ASSERT(underwearSection < NUM_UNDERWEARHIDESECTIONS);

  m_sections[s_underwearSections[underwearSection]].m_layers[TEXLAYER_SKIN].m_priorities[LAYERPRIORITY_1].SetHold(2);
}

void CTexComponent::SetTexture(int checkExistingTexture, HTEXTURE texture) {
  if (m_texture) {
    HandleClose(m_texture);
  }

  m_texture = static_cast<HTEXTURE>(HandleDuplicate(texture));
  CTexturePiece::SetTexture(checkExistingTexture && !(m_flags & 1), texture);
  m_textureInfo.opaque = 1;
}

void CTexComponent::SetTexture(
    CStatus              *status,
    int                   checkExistingTexture,
    const char           *fileName,
    TEXCOMPONENT_SECTIONS section,
    TEXCOMPONENT_LAYERS   layer,
    LAYERPRIORITY         priority,
    unsigned int          expectedWidth,
    unsigned int          expectedHeight
) {
  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  ASSERT(layer < NUM_TEXLAYERS);

  m_sections[section].m_layers[layer].SetTexture(
      section, layer, priority, status, checkExistingTexture && !(m_flags & 1), fileName, expectedWidth, expectedHeight
  );
  m_dirtyFlags |= 1 << section;
}

void CTexComponent::UpdateUnderwearVisibility() {
  unsigned int section;
  for (section = 0; section < NUM_UNDERWEARHIDESECTIONS; ++section) {
    if (m_underwearHideCounts[section]) {
      HideUnderwear(section);
    } else {
      ShowUnderwear(section);
    }
  }
}

void CTexComponent::IncUnderwearHideCount(int itemInventoryType, TEXCOMPONENT_SECTIONS sectionID) {
  static const int sectionToUnderwear[NUM_TEXCOMPONENT_SECTIONS] = {-1, -1, -1, -1, -1, 0, -1, 1, -1, -1};
  static const int inventoryHidesUnderwear[INDEX_NUMSLOTS][2] = {
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {1, 1},
      {1, 1},
      {0, 1},
      {0, 1},
      {0, 1},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {1, 1},
      {1, 1},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0}
  };

  unsigned int section = sectionToUnderwear[sectionID];
  if (section != static_cast<unsigned int>(-1)) {
    ASSERT(section < NUM_UNDERWEARHIDESECTIONS);
    if (inventoryHidesUnderwear[itemInventoryType][section]) {
      ++m_underwearHideCounts[section];
    }
  }
}

void __fastcall UpdateComponentTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  FATALASSERT(userArg);

  CStatus        status;
  CTexComponent *component = reinterpret_cast<CTexComponent *>(userArg);

  switch (cmd) {
    case GxTex_Lock:
      if (!component->m_dirtyFlags) {
        unsigned int section;
        for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
          component->m_dirtyFlags |= 1 << section;
        }
        component->CheckSections(1);
        component->UpdateSections(&status, 0);
      }
      break;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = CTexturePiece::m_destImage->mip[mipLevel];
      break;
  }
}

HTEXCOMPONENT __fastcall
TexComponentCreate(HTEXTURE texture, unsigned int race, unsigned int sex, unsigned int skinID, int isNPC, int ignoreExistingTexture) {
  unsigned int sectionFlags[NUM_TEXLAYERS];

  CharCustomizationGetTextureLayerHolds(race, sex, sectionFlags, NUM_TEXLAYERS);

  if (!texture) {
    return 0;
  }

  CTexComponent *component = new (SMemAlloc(sizeof(CTexComponent), "HTEXCOMPONENT", SERR_LINECODE_OBJECT, 0)) CTexComponent;

  CGxTex *gxTex = TextureGetGxTex(texture, 1, 0);
  GxTexSetUserData(gxTex, UpdateComponentTexture, component);

  if (ignoreExistingTexture) {
    component->m_flags |= 1;
  }

  CStatus status;
  component->SetTexture(0, texture);
  component->BuildSkinPieces(&status, sectionFlags);
  component->BuildNakedPieces(&status, race, sex, skinID, isNPC);

  unsigned int section;
  for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    component->m_dirtyFlags |= 1 << section;
  }

  return reinterpret_cast<HTEXCOMPONENT>(HandleCreate(component, "HTEXCOMPONENT"));
}

void __fastcall TexComponentAdd(
    CStatus                  *status,
    int                       playerSex,
    HTEXCOMPONENT             component,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       checkForExistingTexture
) {
  CSectionFileNames     sectionArt;
  char                  buffer[MAX_PATH];
  LAYERPRIORITY         priorityList[6];
  TEXCOMPONENT_LAYERS   layerList[6];
  TEXCOMPONENT_SECTIONS sectionList[6];
  unsigned int          height;
  unsigned int          width;
  unsigned int          numTextureComponents;

  ASSERT(playerSex < UNITSEX_LAST);

  CTexComponent *componentptr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentptr);

  if (!displayInfoRec ||
      !CompUtilItemSectionInfo(displayInfoRec, itemInventoryType, &numTextureComponents, sectionList, layerList, priorityList, &sectionArt))
  {
    return;
  }

  unsigned int i;
  for (i = 0; i < numTextureComponents; ++i) {
    if (!sectionArt.fileName[i][0]) {
      continue;
    }

    TEXCOMPONENT_SECTIONS section = sectionList[i];
    if (section >= NUM_TEXCOMPONENT_SECTIONS) {
      continue;
    }

    int success = CompUtilGetSectionDimensions(section, &width, &height);
    FATALASSERT(success);

    CompDecorateTexName(sectionArt.fileName[i], section, buffer, sizeof(buffer), playerSex, 1);
    if (buffer[0]) {
      componentptr->SetTexture(status, checkForExistingTexture, buffer, section, layerList[i], priorityList[i], width, height);
    }

    if (displayInfoRec->m_flags & 2) {
      componentptr->IncUnderwearHideCount(itemInventoryType, section);
    }
  }

  componentptr->UpdateUnderwearVisibility();
}

void __fastcall TexComponentChangeCharacterHead(HTEXCOMPONENT component, const char *upperHead, const char *lowerHead, unsigned int layer) {
  CTexComponent *componentptr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentptr);

  unsigned int upperWidth;
  unsigned int upperHeight;
  unsigned int lowerWidth;
  unsigned int lowerHeight;
  if (!CompUtilGetSectionDimensions(TCS_UPPERHEAD, &upperWidth, &upperHeight) ||
      !CompUtilGetSectionDimensions(TCS_LOWERHEAD, &lowerWidth, &lowerHeight))
  {
    return;
  }

  CStatus status;
  if (!layer) {
    componentptr->SetUpperHeadTexture(upperHead);
    componentptr->SetLowerHeadTexture(lowerHead);
  } else if (layer == TEXLAYER_CLOTH || layer == TEXLAYER_ARMOR) {
    if (!upperHead || !*upperHead) {
      componentptr->SetTexture(&status, 0, componentptr->m_upperFaceTexture, TCS_UPPERHEAD, TEXLAYER_SKIN, LAYERPRIORITY_0, upperWidth, upperHeight);
    }
    if (!lowerHead || !*lowerHead) {
      componentptr->SetTexture(&status, 0, componentptr->m_lowerFaceTexture, TCS_LOWERHEAD, TEXLAYER_SKIN, LAYERPRIORITY_0, lowerWidth, lowerHeight);
    }
  }

  componentptr->SetTexture(&status, 0, upperHead, TCS_UPPERHEAD, static_cast<TEXCOMPONENT_LAYERS>(layer), LAYERPRIORITY_3, upperWidth, upperHeight);
  componentptr->SetTexture(&status, 0, lowerHead, TCS_LOWERHEAD, static_cast<TEXCOMPONENT_LAYERS>(layer), LAYERPRIORITY_3, lowerWidth, lowerHeight);
}

void CTexComponent::SetUpperHeadTexture(const char *upperHead) {
  SStrCopy(m_upperFaceTexture, upperHead, sizeof(m_upperFaceTexture));
}

void CTexComponent::SetLowerHeadTexture(const char *lowerHead) {
  SStrCopy(m_lowerFaceTexture, lowerHead, sizeof(m_lowerFaceTexture));
}

void __fastcall HeadGeosetHideCharGeosets(
    HCHARGEOSET               geosetHandle,
    const ItemDisplayInfoRec *displayInfoRec,
    unsigned int              raceID,
    const unsigned int       *preferredGeosets,
    unsigned int              numPreferredGeosets
) {
  if (!geosetHandle) {
    return;
  }

  FATALASSERT(raceID != 0);
  FATALASSERT(raceID <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(!preferredGeosets || numPreferredGeosets == NUM_CHARGEOSETS);

  if (!displayInfoRec || !displayInfoRec->m_helmetGeosetVisID) {
    return;
  }
  const HelmetGeosetVisDataRec *helmData = g_helmetGeosetVisDataDB.GetRecord(displayInfoRec->m_helmetGeosetVisID);
  if (!helmData) {
    return;
  }

  FATALASSERT(raceID < sizeof(helmData->m_DefaultFlags) / sizeof(helmData->m_DefaultFlags[0]));
  unsigned int section;
  for (section = 0; section < NUM_CHARGEOSETS; ++section) {
    unsigned int sectionFlag = 1 << section;
    if (!(sectionFlag & 0x8F)) {
      continue;
    }

    CharCustomizationHideGeosetSection(geosetHandle, static_cast<CHARACTER_GEOSET_SECTIONS>(section));
    if (preferredGeosets && !(sectionFlag & helmData->m_DefaultFlags[raceID]) && preferredGeosets[section] &&
        (sectionFlag & helmData->m_PreferredFlags[raceID]))
    {
      CharCustomizationShowGeoset(geosetHandle, static_cast<CHARACTER_GEOSET_SECTIONS>(section), preferredGeosets[section]);
    } else if (!(sectionFlag & helmData->m_HideFlags[raceID])) {
      CharCustomizationShowGeoset(geosetHandle, static_cast<CHARACTER_GEOSET_SECTIONS>(section), 1);
    }
  }
}

static void __fastcall UpdateSubComponentPathNames(SUBCOMPONENTDESC *subComponent, const char *modelName) {
  FATALASSERT(subComponent);
  if (subComponent->modelName) {
    SMemFree(subComponent->modelName, __FILE__, __LINE__, 0);
  }
  subComponent->modelName = 0;
  if (modelName && *modelName) {
    unsigned int length = SStrLen(modelName) + 1;
    subComponent->modelName = static_cast<char *>(SMemAlloc(length, __FILE__, __LINE__, 0));
    SStrCopy(subComponent->modelName, modelName, length);
  }
}

static void __fastcall AddSubcomponentPrefixes(SUBCOMPONENTDESC *subcomponents, unsigned int numSubComponents, unsigned int inventoryType) {
  static const char *const inventoryNames[INDEX_NUMSLOTS] = {"UNUSED", "Head",   "Neck",   "Shoulder", "Body",   "Chest",   "Waist",
                                                             "Legs",   "Feet",   "Wrist",  "Hand",     "Finger", "Trinket", "Weapon",
                                                             "Shield", "Weapon", "Ammo",   "Weapon",   "Bag",    "UNUSED",  "UNUSED",
                                                             "Weapon", "Weapon", "Weapon", "Ammo",     "Weapon", "Weapon"};
  if (!subcomponents || !numSubComponents) {
    return;
  }
  ASSERT(inventoryType < INDEX_NUMSLOTS);

  unsigned int i;
  for (i = 0; i < numSubComponents; ++i) {
    char buffer[MAX_PATH];
    SStrPrintf(buffer, sizeof(buffer), "Item\\ObjectComponents\\%s\\%s", inventoryNames[inventoryType], subcomponents[i].modelName);
    UpdateSubComponentPathNames(&subcomponents[i], buffer);

    if (subcomponents[i].textureName && *subcomponents[i].textureName) {
      SStrPrintf(buffer, sizeof(buffer), "Item\\ObjectComponents\\%s\\%s", inventoryNames[inventoryType], subcomponents[i].textureName);
      unsigned int length = SStrLen(buffer) + 1;
      SMemFree(subcomponents[i].textureName, __FILE__, __LINE__, 0);
      subcomponents[i].textureName = static_cast<char *>(SMemAlloc(length, __FILE__, __LINE__, 0));
      SStrCopy(subcomponents[i].textureName, buffer, length);
    }
  }
}

static void __fastcall
DecorateComponentFileNames(SUBCOMPONENTDESC *subComponents, unsigned int numSubComponents, unsigned int race, unsigned int sex) {
  if (!subComponents || !numSubComponents) {
    return;
  }
  FATALASSERT(race != 0);
  FATALASSERT(race <= static_cast<unsigned int>(g_chrRacesDB.GetMaxID()));
  FATALASSERT(sex < UNITSEX_LAST);

  unsigned int i;
  for (i = 0; i < numSubComponents; ++i) {
    char modelBuffer[MAX_PATH];
    CompDecorateObjName(subComponents[i].modelName, modelBuffer, sizeof(modelBuffer), race, sex);
    UpdateSubComponentPathNames(&subComponents[i], modelBuffer);
  }
}

static HMODEL __fastcall ObjComponentBuildSubComponent(SUBCOMPONENTDESC *subComponent, const ItemDisplayInfoRec *displayInfoRec) {
  FATALASSERT(subComponent);
  FATALASSERT(displayInfoRec);
  if (!subComponent->modelName || !*subComponent->modelName) {
    return 0;
  }

  CModelCreate createData;
  createData.flags = 10246;
  createData.sequenceNames = &g_animationNames[FIRST_ITEMANIMATION];
  createData.numSequences = NUM_ITEMANIMATIONS;
  createData.boneNames = s_boneNames;
  createData.numBones = 3;
  createData.cameraNames = 0;
  createData.numCameras = 0;
  CStatus status;
  HMODEL  subCompModel = ModelCreate(subComponent->modelName, &createData, &status);
  FATALASSERT(subCompModel);
  ModelSetSequence(subCompModel, 0, 0);

  if (subComponent->textureName && *subComponent->textureName) {
    HTEXTURE texture = TextureCreate(subComponent->textureName, CGxTexFlags(GxTex_LinearMipLinear, 0, 0, 0, 0, 0, 1), &status, 0);
    if (!texture) {
      SysMsgPrintf(SYSMSG_ERROR, 2, "TEXCOMPONENTNOTEXTURE|%d:%s!", displayInfoRec->m_ID, subComponent->textureName);
      return subCompModel;
    }
    if (!ModelReplaceTexture(subCompModel, 2, texture, 0)) {
      SysMsgPrintf(SYSMSG_ERROR, 2, "TEXCOMPONENTNOREPLACEABLEID|%d", displayInfoRec->m_ID);
    }
    HandleClose(texture);
  }
  return subCompModel;
}

int __fastcall ObjComponentAdd(
    int                       unitSex,
    int                       unitRace,
    int                       unitPlayer,
    HMODEL                    model,
    const ItemDisplayInfoRec *displayInfoRec,
    int                       itemInventoryType,
    int                       useAlternateSlot,
    HMODEL                    existingModel,
    OBJCALLBACK               callback,
    void                     *param,
    unsigned int              inventorySlot
) {
  SUBCOMPONENTDESC subComponents[2];
  unsigned int     numSubComponents = CompUtilGetObjComponents(displayInfoRec, itemInventoryType, subComponents, 2, useAlternateSlot);
  if (!numSubComponents) {
    return 1;
  }

  if (unitPlayer && itemInventoryType == INDEX_HEAD_TYPE) {
    DecorateComponentFileNames(subComponents, numSubComponents, unitRace, unitSex);
  }
  AddSubcomponentPrefixes(subComponents, numSubComponents, itemInventoryType);

  unsigned int componentIndex;
  for (componentIndex = 0; componentIndex < numSubComponents; ++componentIndex) {
    HMODEL itemModel = existingModel && numSubComponents == 1 ? static_cast<HMODEL>(HandleDuplicate(existingModel))
                                                              : ObjComponentBuildSubComponent(&subComponents[componentIndex], displayInfoRec);
    if (!itemModel) {
      continue;
    }

    if (!ModelAddLink(model, subComponents[componentIndex].attachmentPoint, itemModel, 1.0f)) {
      SysMsgPrintf(SYSMSG_WARNING, 0x10, "PLAYERMODELNOCONNECTION|%d|%d|%d", unitRace, unitSex, itemInventoryType);
    }
    if (callback) {
      callback(param, inventorySlot, itemModel, subComponents[componentIndex].attachmentPoint, 1);
    } else {
      HandleClose(itemModel);
    }
  }
  return 1;
}

int __fastcall TexComponentCommitSections(CStatus *status, HTEXCOMPONENT component, int bForce) {
  CTexComponent *componentPtr = reinterpret_cast<CTexComponent *>(component);

  if (!componentPtr) {
    return 0;
  }

  if (bForce) {
    componentPtr->CheckSections(1);
  }

  if (componentPtr->m_dirtyFlags) {
    componentPtr->UpdateSections(status, 1);
  }

  return 1;
}

int __fastcall TexComponentCheckSections(HTEXCOMPONENT component, int bForce) {
  if (component) {
    return reinterpret_cast<CTexComponent *>(component)->CheckSections(bForce);
  }
  return 0;
}

void CTexComponent::RemoveSections(const TEXCOMPONENT_SECTIONS *sectionPointers, const unsigned int *startLayerList, unsigned int size) {
  FATALASSERT(sectionPointers);
  FATALASSERT(startLayerList);
  FATALASSERT(size);

  for (unsigned int entry = 0; entry < size; ++entry) {
    TEXCOMPONENT_SECTIONS section = sectionPointers[entry];
    for (unsigned int layer = startLayerList[entry]; layer < NUM_TEXLAYERS; ++layer) {
      for (unsigned int priority = 0; priority < NUM_LAYERPRIORITIES; ++priority) {
        m_sections[section].m_layers[layer].m_priorities[priority].SetTexture(0, 0);
      }
    }
  }
}

void CTexComponent::AddHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section) {
  FATALASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  FATALASSERT(inventory < INDEX_NUMSLOTS);

  TEXCOMPONENT_LAYERS layer = g_sectionLayers[inventory].layers[section];
  LAYERPRIORITY       priority = g_sectionPriorities[inventory].priorities[section];
  FATALASSERT(layer < NUM_TEXLAYERS);
  FATALASSERT(priority < NUM_LAYERPRIORITIES);

  CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];
  if (!(piece.m_holds & 1)) {
    piece.m_holds |= 1;
    m_dirtyFlags |= 1 << section;
  }
}

void CTexComponent::RemoveHold(INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section) {
  FATALASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  FATALASSERT(inventory < INDEX_NUMSLOTS);

  TEXCOMPONENT_LAYERS layer = g_sectionLayers[inventory].layers[section];
  LAYERPRIORITY       priority = g_sectionPriorities[inventory].priorities[section];
  FATALASSERT(layer < NUM_TEXLAYERS);
  FATALASSERT(priority < NUM_LAYERPRIORITIES);

  CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];
  if (piece.m_holds & 1) {
    piece.m_holds &= ~1u;
    m_dirtyFlags |= 1 << section;
  }
}

void CTexComponent::RemoveHolds() {
  for (unsigned int section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    for (unsigned int layer = 0; layer < NUM_TEXLAYERS; ++layer) {
      for (unsigned int priority = 0; priority < NUM_LAYERPRIORITIES; ++priority) {
        CTexturePiece &piece = m_sections[section].m_layers[layer].m_priorities[priority];
        int            hadHolds = piece.m_holds != 0;
        piece.m_holds &= ~1u;
        if (hadHolds && !piece.m_holds) {
          m_dirtyFlags |= 1 << section;
        }
      }
    }
  }
}

void __fastcall TexComponentRemoveSections(
    HTEXCOMPONENT                component,
    const TEXCOMPONENT_SECTIONS *sectionPointers,
    const unsigned int          *startLayerList,
    unsigned int                 size
) {
  CTexComponent *componentPtr = reinterpret_cast<CTexComponent *>(component);
  if (componentPtr && sectionPointers && startLayerList && size) {
    componentPtr->RemoveSections(sectionPointers, startLayerList, size);
  }
}

void __fastcall TexComponentRemoveAllHolds(HTEXCOMPONENT component) {
  CTexComponent *componentPtr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentPtr);
  componentPtr->RemoveHolds();
}

void __fastcall TexComponentAddHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section) {
  CTexComponent *componentPtr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentPtr);
  componentPtr->AddHold(inventory, section);
}

void __fastcall TexComponentRemoveHold(HTEXCOMPONENT component, INVENTORY_TYPES inventory, TEXCOMPONENT_SECTIONS section) {
  CTexComponent *componentPtr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentPtr);
  componentPtr->RemoveHold(inventory, section);
}

void __fastcall ComponentInitialize() {
  unsigned int section;

  if (s_textureCacheHandle) {
    HandleClose(s_textureCacheHandle);
  }

  s_textureCacheHandle = TextureCacheCreateSizeCache(0x02000000);

  s_numSectionsMask = 0;
  for (section = 0; section < NUM_TEXCOMPONENT_SECTIONS; ++section) {
    s_numSectionsMask |= 1 << section;
  }

  CTexturePiece::m_destImage = TextureAllocMippedImg(GxTex_Argb8888, 0x100, 0x100);
  CTexturePiece::m_destTextureInfo.width = 0x100;
  CTexturePiece::m_destTextureInfo.height = 0x100;
  CTexturePiece::m_destTextureInfo.format = GxTex_Argb8888;
  CTexturePiece::m_destTextureInfo.levels = TextureCalcMipCount(0x100, 0x100);
  CTexturePiece::m_destTextureInfo.opaque = 1;
}

void __fastcall ComponentShutdown() {
  if (s_textureCacheHandle) {
    HandleClose(s_textureCacheHandle);
  }

  s_textureCacheHandle = 0;
  TextureFreeMippedImg(CTexturePiece::m_destImage);
}

int __fastcall
GetObjComponentInfo(int race, int sex, int displayID, int inventoryType, bool isPlayer, bool useAlternate, HMODEL *models, int *attachmentPoints) {
  SUBCOMPONENTDESC subComponents[2];
  int              added;

  FATALASSERT(models);
  FATALASSERT(attachmentPoints);

  ItemDisplayInfoRec *displayInfoRec = g_itemDisplayInfoDB.GetRecord(displayID);
  unsigned int        numSubComponents = CompUtilGetObjComponents(displayInfoRec, inventoryType, subComponents, 2, useAlternate);
  if (!numSubComponents) {
    return 0;
  }

  if (isPlayer && inventoryType == INDEX_HEAD_TYPE) {
    DecorateComponentFileNames(subComponents, numSubComponents, race, sex);
  }
  AddSubcomponentPrefixes(subComponents, numSubComponents, inventoryType);

  added = 0;
  for (unsigned int componentIndex = 0; componentIndex < numSubComponents; ++componentIndex) {
    HMODEL model = ObjComponentBuildSubComponent(&subComponents[componentIndex], displayInfoRec);
    if (model) {
      models[added] = model;
      attachmentPoints[added] = subComponents[componentIndex].attachmentPoint;
      ++added;
    }
  }
  return added;
}

bool __fastcall ComponentApplyTabardTexture(HTEXCOMPONENT component, int eStyle, int eColor, int bStyle, int bColor, int b) {
  CTexComponent               *componentptr = reinterpret_cast<CTexComponent *>(component);
  const TEXCOMPONENT_SECTIONS *section;

  FATALASSERT(componentptr);

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

  for (section = s_tabardSections; section < s_tabardSections + 2; ++section) {
    componentptr->m_dirtyFlags |= 1 << *section;
  }

  return true;
}

void __fastcall GetTabardBackgroundFileName(int section, int background, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Background_%02d%s_U", background, s_tabardSectionSuffix[section]);
}

void __fastcall GetTabardEmblemFileName(int section, int emblem, int color, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Emblem_%02d_%02d%s_U", emblem, color, s_tabardSectionSuffix[section]);
}

void __fastcall GetTabardBorderFileName(int section, int border, int color, char *buffer, int size) {
  SStrPrintf(buffer, size, "Textures\\GuildEmblems\\Border_%02d_%02d%s_U", border, color, s_tabardSectionSuffix[section]);
}

void __fastcall ComponentRemoveTabardTexture(int sex, HTEXCOMPONENT component, ItemDisplayInfoRec *displayInfo, int inventoryType) {
  CTexComponent *componentptr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentptr);

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

void __fastcall ComponentForceTabardDraw(HTEXCOMPONENT component) {
  CTexComponent *componentptr = reinterpret_cast<CTexComponent *>(component);
  FATALASSERT(componentptr);
  componentptr->m_flags |= 2;
  componentptr->m_dirtyFlags |= 0x60;
}

void __fastcall TexComponentCopy(HTEXCOMPONENT d, HTEXCOMPONENT s) {
  if (d && s) {
    *d = *s;
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
    m_underwearHideCounts[0] = rhs.m_underwearHideCounts[0];
    m_underwearHideCounts[1] = rhs.m_underwearHideCounts[1];

    for (unsigned int i = 0; i < NUM_TEXCOMPONENT_SECTIONS; ++i) {
      m_sections[i] = rhs.m_sections[i];
      m_dirtyFlags |= 1 << i;
    }

    m_flags |= rhs.m_flags;
  }

  return *this;
}

CSection &CSection::operator=(const CSection &rhs) {
  if (this != &rhs) {
    for (unsigned int i = 0; i < NUM_TEXLAYERS; ++i) {
      m_layers[i] = rhs.m_layers[i];
    }
  }

  return *this;
}

CTextureLayer &CTextureLayer::operator=(const CTextureLayer &rhs) {
  if (this != &rhs) {
    for (unsigned int i = 0; i < NUM_LAYERPRIORITIES; ++i) {
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

    m_mippedTexture = static_cast<HMIPPEDTEXTURE>(HandleDuplicate(rhs.m_mippedTexture));
    m_textureInfo = rhs.m_textureInfo;
    m_holds = rhs.m_holds;
  }

  return *this;
}

HMODEL __fastcall ObjComponentBuildAmmoModel(ItemDisplayInfoRec *displayInfoRec, unsigned int inventoryType, unsigned int &seqDuration) {
  seqDuration = 0;
  if (!displayInfoRec || !displayInfoRec->m_modelName[1] || !displayInfoRec->m_modelTexture[1] || !inventoryType || inventoryType >= INDEX_NUMSLOTS) {
    return 0;
  }

  static const char *const inventoryNames[INDEX_NUMSLOTS] = {"UNUSED", "Head",   "Neck",   "Shoulder", "Body",   "Chest",   "Waist",
                                                             "Legs",   "Feet",   "Wrist",  "Hand",     "Finger", "Trinket", "Weapon",
                                                             "Shield", "Weapon", "Ammo",   "Weapon",   "Bag",    "UNUSED",  "UNUSED",
                                                             "Weapon", "Weapon", "Weapon", "Ammo",     "Weapon", "Weapon"};

  char texturePath[MAX_PATH];
  char modelPath[MAX_PATH];
  SStrPrintf(modelPath, sizeof(modelPath), "Item\\ObjectComponents\\%s\\%s", inventoryNames[inventoryType], displayInfoRec->m_modelName[1]);
  SStrPrintf(texturePath, sizeof(texturePath), "Item\\ObjectComponents\\%s\\%s", inventoryNames[inventoryType], displayInfoRec->m_modelTexture[1]);

  SUBCOMPONENTDESC subComponent;
  unsigned int     length = SStrLen(modelPath) + 1;
  subComponent.modelName = static_cast<char *>(SMemAlloc(length, __FILE__, __LINE__, 0));
  SStrCopy(subComponent.modelName, modelPath, length);
  length = SStrLen(texturePath) + 1;
  subComponent.textureName = static_cast<char *>(SMemAlloc(length, __FILE__, __LINE__, 0));
  SStrCopy(subComponent.textureName, texturePath, length);

  HMODEL model = ObjComponentBuildSubComponent(&subComponent, displayInfoRec);
  if (model) {
    if (ModelHasSequenceId(model, 1)) {
      ModelSetSequence(model, 1, 0);
    } else {
      ModelSetSequence(model, 0, 0);
      ModelGetSequenceDuration(model, 0, &seqDuration);
    }
  }
  return model;
}
