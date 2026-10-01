#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "MDLFile/MDLTypes.h"
#include "Model/Material.h"
#include "Services/IParticleMisc.h"
#include "Services/RibbonEmitter.h"

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static void LoadRibbonMaterial(
    const CMaterial                      &uniqueMtl,
    const TSGrowableArray<CModelTexture> &modelTextures,
    TSGrowableArray<CRibbonMat>          *mats,
    TSGrowableArray<HTEXTURE>            *textures,
    TSGrowableArray<UINT>                *replace
) {
  UINT numLayers = uniqueMtl.layers.Count();
  mats->SetCount(numLayers);
  textures->SetCount(numLayers);
  replace->SetCount(numLayers);

  for (UINT i = 0; i < numLayers; ++i) {
    const CTexLayer &layer = uniqueMtl.layers[i];
    CRibbonMat      &ribbonMaterial = (*mats)[i];

    ribbonMaterial.enableLighting = !layer.disable.lighting;
    ribbonMaterial.enableFog = !layer.disable.fog;
    ribbonMaterial.enableDepthTest = !layer.disable.depthTest;
    ribbonMaterial.enableDepthWrite = !layer.disable.depthWrite;
    ribbonMaterial.enableCulling = !layer.disable.culling;
    ribbonMaterial.alpha = layer.blendMode;

    UINT textureId = layer.tmuPass[0].textureId;
    (*textures)[i] = modelTextures[textureId].handle;
    (*replace)[i] = modelTextures[textureId].replaceableId;
  }
}

static void LoadEmitterData(BYTE *emitterData, CModelComplex *modelptr, CRibbonEmitter *ribbon) {
  static TSGrowableArray<CRibbonMat> mats;
  static TSGrowableArray<HTEXTURE>   textures;
  static TSGrowableArray<UINT>       replace;

  UINT   staticDataOffset = *reinterpret_cast<UINT *>(emitterData);
  BYTE  *staticData = emitterData + staticDataOffset + 4;
  float *values = reinterpret_cast<float *>(staticData);

  float               staticHeightAbove = values[0];
  float               staticHeightBelow = values[1];
  NTempest::CImVector diffColor;
  diffColor.Set(values[2], values[3], values[4], values[5]);
  float edgeLifetime = values[6];
  UINT  staticTextureSlot = *reinterpret_cast<UINT *>(staticData + 28);
  UINT  edgesPerSecond = *reinterpret_cast<UINT *>(staticData + 32);
  UINT  textureRows = *reinterpret_cast<UINT *>(staticData + 36);
  UINT  textureCols = *reinterpret_cast<UINT *>(staticData + 40);
  UINT  materialId = *reinterpret_cast<UINT *>(staticData + 44);
  float gravity = *reinterpret_cast<float *>(staticData + 48);

  CMaterial *material = static_cast<CMaterial *>(HandleDereference(reinterpret_cast<HOBJECT>(modelptr->m_materials[materialId])));
  ASSERT(material);
  LoadRibbonMaterial(*material, modelptr->m_textures, &mats, &textures, &replace);

  ribbon->Initialize(
      static_cast<float>(edgesPerSecond), edgeLifetime, diffColor, textures, mats, replace, NTempest::CRect(0.0f, 0.0f, 1.0f, 1.0f), textureRows,
      textureCols
  );
  ribbon->SetAbove(staticHeightAbove);
  ribbon->SetBelow(staticHeightBelow);
  ribbon->SetTexSlot(staticTextureSlot);
  ribbon->SetEnabled(0);
  ribbon->SetGravity(gravity);
}

BOOL MdlReadLoadRibbonEmitters(const MDLDATA &data, CModelComplex *modelptr, CModelShared *shared) {
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  UINT numRibbons = data.ribbonEmitters.Count();
  modelptr->m_ribbons.SetCount(numRibbons);
  shared->ribbonOrder.SetCount(numRibbons);

  TSGrowableArray<CRibbonMat> mats;
  TSGrowableArray<HTEXTURE>   textures;
  TSGrowableArray<UINT>       replace;
  for (UINT i = 0; i < numRibbons; ++i) {
    shared->ribbonOrder[i] = data.ribbonEmitters[i].objectId;

    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(modelptr->m_materials[data.ribbonEmitters[i].materialId]);
    FATALASSERT(uniqueMtl);
    LoadRibbonMaterial(*uniqueMtl, modelptr->m_textures, &mats, &textures, &replace);

    NTempest::CImVector diffColor;
    diffColor.Set(data.ribbonEmitters[i].staticAlpha, data.ribbonEmitters[i].staticColor.r,
                  data.ribbonEmitters[i].staticColor.g, data.ribbonEmitters[i].staticColor.b);

    CRibbonEmitter *ribbon = RibbonManager::GetInstance()->CreateEmitter();
    modelptr->m_ribbons[i] = ribbon;
    ribbon->Initialize(data.ribbonEmitters[i].edgesPerSecond, data.ribbonEmitters[i].edgeLifetime, diffColor, textures, mats,
                       replace, NTempest::CRect(0.0f, 0.0f, 1.0f, 1.0f), data.ribbonEmitters[i].textureRows,
                       data.ribbonEmitters[i].textureCols);
    ribbon->SetAbove(data.ribbonEmitters[i].staticHeightAbove);
    ribbon->SetBelow(data.ribbonEmitters[i].staticHeightBelow);
    ribbon->SetTexSlot(data.ribbonEmitters[i].staticTextureSlot);
    ribbon->SetEnabled(0);
    ribbon->SetGravity(data.ribbonEmitters[i].gravity);
  }
  return 1;
}

void MdxReadRibbonEmitters(BYTE *data, UINT fileBytes, CModelComplex *modelptr, CModelShared *shared) {
  ASSERT(data);
  ASSERT(modelptr);
  ASSERT(shared);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'BBIR');
  if (!section) {
    return;
  }

  BYTE *dataDone = section + 4 + *reinterpret_cast<UINT *>(section);
  UINT  numEmitters = *reinterpret_cast<UINT *>(section + 4);
  BYTE *ribbonData = section + 8;

  modelptr->m_ribbons.SetCount(numEmitters);
  shared->ribbonOrder.SetCount(numEmitters);

  RibbonManager *manager = RibbonManager::GetInstance();
  for (UINT i = 0; i < numEmitters; ++i) {
    UINT  bytesThisRibbon = *reinterpret_cast<UINT *>(ribbonData);
    BYTE *ribbonDone = ribbonData + bytesThisRibbon;
    ASSERT(ribbonDone <= dataDone);

    CRibbonEmitter *ribbon = manager->CreateEmitter();
    modelptr->m_ribbons[i] = ribbon;
    shared->ribbonOrder[i] = *reinterpret_cast<UINT *>(ribbonData + 0x58);
    LoadEmitterData(ribbonData + 4, modelptr, ribbon);

    ribbonData = ribbonDone;
  }

  ASSERT(ribbonData == dataDone);
}
