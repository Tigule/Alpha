#include "Model/ModelInternal.h"

#include "MDLFile/MDLTypes.h"
#include "Model/Material.h"
#include "Services/IParticleMisc.h"
#include "Services/RibbonEmitter.h"

unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static void LoadRibbonMaterial(
    const CMaterial                      &uniqueMtl,
    const TSGrowableArray<CModelTexture> &modelTextures,
    TSGrowableArray<CRibbonMat>          *mats,
    TSGrowableArray<HTEXTURE>            *textures,
    TSGrowableArray<unsigned int>        *replace
) {
  unsigned int numLayers = uniqueMtl.layers.Count();
  mats->SetCount(numLayers);
  textures->SetCount(numLayers);
  replace->SetCount(numLayers);

  for (unsigned int i = 0; i < numLayers; ++i) {
    const CTexLayer &layer = uniqueMtl.layers[i];
    CRibbonMat      &ribbonMaterial = (*mats)[i];

    ribbonMaterial.enableLighting = !layer.disable.lighting;
    ribbonMaterial.enableFog = !layer.disable.fog;
    ribbonMaterial.enableDepthTest = !layer.disable.depthTest;
    ribbonMaterial.enableDepthWrite = !layer.disable.depthWrite;
    ribbonMaterial.enableCulling = !layer.disable.culling;
    ribbonMaterial.alpha = layer.blendMode;

    unsigned int textureId = layer.tmuPass[0].textureId;
    (*textures)[i] = modelTextures[textureId].handle;
    (*replace)[i] = modelTextures[textureId].replaceableId;
  }
}

static void LoadEmitterData(unsigned char *emitterData, CModelComplex *modelptr, CRibbonEmitter *ribbon) {
  static TSGrowableArray<CRibbonMat>   mats;
  static TSGrowableArray<HTEXTURE>     textures;
  static TSGrowableArray<unsigned int> replace;

  unsigned int   staticDataOffset = *reinterpret_cast<unsigned int *>(emitterData);
  unsigned char *staticData = emitterData + staticDataOffset + 4;
  float         *values = reinterpret_cast<float *>(staticData);

  float               staticHeightAbove = values[0];
  float               staticHeightBelow = values[1];
  NTempest::CImVector diffColor;
  diffColor.Set(values[2], values[3], values[4], values[5]);
  float        edgeLifetime = values[6];
  unsigned int staticTextureSlot = *reinterpret_cast<unsigned int *>(staticData + 28);
  unsigned int edgesPerSecond = *reinterpret_cast<unsigned int *>(staticData + 32);
  unsigned int textureRows = *reinterpret_cast<unsigned int *>(staticData + 36);
  unsigned int textureCols = *reinterpret_cast<unsigned int *>(staticData + 40);
  unsigned int materialId = *reinterpret_cast<unsigned int *>(staticData + 44);
  float        gravity = *reinterpret_cast<float *>(staticData + 48);

  CMaterial *material = static_cast<CMaterial *>(HandleDereference(reinterpret_cast<HOBJECT>(modelptr->m_materials[materialId])));
  ASSERT(material);
  LoadRibbonMaterial(*material, modelptr->m_textures, &mats, &textures, &replace);

  ribbon->Initialize(
      static_cast<float>(edgesPerSecond), edgeLifetime, diffColor, textures, mats, replace,
      NTempest::CRect(0.0f, 0.0f, 1.0f, 1.0f), textureRows, textureCols
  );
  ribbon->SetAbove(staticHeightAbove);
  ribbon->SetBelow(staticHeightBelow);
  ribbon->SetTexSlot(staticTextureSlot);
  ribbon->SetEnabled(0);
  ribbon->SetGravity(gravity);
}

int MdlReadLoadRibbonEmitters(const MDLDATA& data, CModelComplex* modelptr, CModelShared* shared) {
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  unsigned int numRibbons = data.ribbonEmitters.Count();
  modelptr->m_ribbons.SetCount(numRibbons);
  shared->ribbonOrder.SetCount(numRibbons);

  TSGrowableArray<CRibbonMat> mats;
  TSGrowableArray<HTEXTURE> textures;
  TSGrowableArray<unsigned int> replace;
  const unsigned char *ribbonData =
      reinterpret_cast<const unsigned char *>(
          data.ribbonEmitters.Ptr());
  RibbonManager *manager = RibbonManager::GetInstance();
  unsigned int i;
  for (i = 0; i < numRibbons; ++i) {
    const unsigned char *src = ribbonData + i * 392;
    shared->ribbonOrder[i] =
        *reinterpret_cast<const unsigned int *>(src + 0x50);

    unsigned int materialId =
        *reinterpret_cast<const unsigned int *>(src + 0x184);
    CMaterial *material = static_cast<CMaterial *>(
        HandleDereference(
            reinterpret_cast<HOBJECT>(
                modelptr->m_materials[materialId])));
    FATALASSERT(material);
    LoadRibbonMaterial(
        *material, modelptr->m_textures,
        &mats, &textures, &replace);

    const float *color =
        reinterpret_cast<const float *>(src + 0x110);
    float alpha = *reinterpret_cast<const float *>(src + 0xF0);
    NTempest::CImVector diffColor;
    diffColor.Set(color[0], color[1], color[2], alpha);

    CRibbonEmitter *ribbon = manager->CreateEmitter();
    modelptr->m_ribbons[i] = ribbon;
    NTempest::CRect texBox(0.0f, 0.0f, 1.0f, 1.0f);
    ribbon->Initialize(
        static_cast<float>(
            *reinterpret_cast<const unsigned int *>(src + 0x138)),
        *reinterpret_cast<const float *>(src + 0x13C),
        diffColor, textures, mats, replace, texBox,
        *reinterpret_cast<const unsigned int *>(src + 0x144),
        *reinterpret_cast<const unsigned int *>(src + 0x148));
    ribbon->SetAbove(
        *reinterpret_cast<const float *>(src + 0xB0));
    ribbon->SetBelow(
        *reinterpret_cast<const float *>(src + 0xD0));
    ribbon->SetTexSlot(
        *reinterpret_cast<const unsigned int *>(src + 0x14C));
    ribbon->SetEnabled(0);
    ribbon->SetGravity(
        *reinterpret_cast<const float *>(src + 0x140));
  }
  return 1;
}

void MdxReadRibbonEmitters(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr, CModelShared *shared) {
  ASSERT(data);
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x42424952);
  if (!section) {
    return;
  }

  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numEmitters = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *ribbonData = section + 8;

  modelptr->m_ribbons.SetCount(numEmitters);
  shared->ribbonOrder.SetCount(numEmitters);

  RibbonManager *manager = RibbonManager::GetInstance();
  for (unsigned int i = 0; i < numEmitters; ++i) {
    unsigned int   bytesThisRibbon = *reinterpret_cast<unsigned int *>(ribbonData);
    unsigned char *ribbonDone = ribbonData + bytesThisRibbon;
    ASSERT(ribbonDone <= dataDone);

    CRibbonEmitter *ribbon = manager->CreateEmitter();
    modelptr->m_ribbons[i] = ribbon;
    shared->ribbonOrder[i] = *reinterpret_cast<unsigned int *>(ribbonData + 0x58);
    LoadEmitterData(ribbonData + 4, modelptr, ribbon);

    ribbonData = ribbonDone;
  }

  ASSERT(ribbonData == dataDone);
}
