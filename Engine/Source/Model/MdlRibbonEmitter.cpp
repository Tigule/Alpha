#include "Model/ModelInternal.h"

struct MDLDATA;

#include "Model/Material.h"
#include "Services/IParticleMisc.h"
#include "Services/RibbonEmitter.h"

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static void __fastcall LoadRibbonMaterial(
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

static void __fastcall LoadEmitterData(unsigned char *emitterData, CModelComplex *modelptr, CRibbonEmitter *ribbon) {
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

  NTempest::CRect texBox(0.0f, 0.0f, 1.0f, 1.0f);
  ribbon->Initialize(static_cast<float>(edgesPerSecond), edgeLifetime, diffColor, textures, mats, replace, texBox, textureRows, textureCols);
  ribbon->SetAbove(staticHeightAbove);
  ribbon->SetBelow(staticHeightBelow);
  ribbon->SetTexSlot(staticTextureSlot);
  ribbon->SetEnabled(0);
  ribbon->SetGravity(gravity);
}

int __fastcall MdlReadLoadRibbonEmitters(const MDLDATA& data, CModelComplex* modelptr, CModelShared* shared) {
    // TODO: implement
    return 0;
}

void __fastcall MdxReadRibbonEmitters(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr, CModelShared *shared) {
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
