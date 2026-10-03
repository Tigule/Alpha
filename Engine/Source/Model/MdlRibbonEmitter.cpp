#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "MDLFile/MDLTypes.h"
#include "Model/Material.h"
#include "Services/IParticleMisc.h"
#include "Services/ParticleSystem2.h"
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
    CRibbonMat &ribbonMaterial = (*mats)[i];
    const CTexLayer &layer = uniqueMtl.layers[i];

    ribbonMaterial.enableLighting = !layer.disable.lighting;
    ribbonMaterial.enableFog = !layer.disable.fog;
    ribbonMaterial.enableDepthTest = !layer.disable.depthTest;
    ribbonMaterial.enableDepthWrite = !layer.disable.depthWrite;
    ribbonMaterial.enableCulling = !layer.disable.culling;
    ribbonMaterial.alpha = layer.blendMode;

    UINT textureId = uniqueMtl.layers[i].tmuPass[0].textureId;
    (*textures)[i] = modelTextures[textureId].handle;
    (*replace)[i] = modelTextures[textureId].replaceableId;
  }
}

static void LoadEmitterData(BYTE *emitterData, CModelComplex *modelptr, CRibbonEmitter *ribbon) {
  emitterData += *reinterpret_cast<UINT *>(emitterData) + 4;
  float staticHeightAbove = *reinterpret_cast<float *>(emitterData);
  emitterData += 4;
  float staticHeightBelow = *reinterpret_cast<float *>(emitterData);
  emitterData += 4;
  NTempest::CImVector diffColor(
      NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(emitterData)[0] * 255.0f),
      NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(emitterData)[1] * 255.0f),
      NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(emitterData)[2] * 255.0f),
      NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(emitterData)[3] * 255.0f)
  );
  emitterData += 16;
  float edgeLifetime = *reinterpret_cast<float *>(emitterData);
  emitterData += 4;
  UINT staticTextureSlot = *reinterpret_cast<UINT *>(emitterData);
  emitterData += 4;
  UINT edgesPerSecond = *reinterpret_cast<UINT *>(emitterData);
  emitterData += 4;
  UINT textureRows = *reinterpret_cast<UINT *>(emitterData);
  emitterData += 4;
  UINT textureCols = *reinterpret_cast<UINT *>(emitterData);
  emitterData += 4;
  UINT materialId = *reinterpret_cast<UINT *>(emitterData);
  emitterData += 4;
  float gravity = *reinterpret_cast<float *>(emitterData);

  static TSGrowableArray<CRibbonMat> mats;
  static TSGrowableArray<HTEXTURE>   textures;
  static TSGrowableArray<UINT>       replace;

  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(modelptr->m_materials[materialId]);
  ASSERT(uniqueMtl);
  LoadRibbonMaterial(*uniqueMtl, modelptr->m_textures, &mats, &textures, &replace);

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

  data = MDLFileBinarySeek(data, fileBytes, 'BBIR');
  if (!data) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(data);
  data += 4;
  BYTE *dataDone = data + sectionBytes;
  UINT  numEmitters = *reinterpret_cast<UINT *>(data);
  data += 4;

  modelptr->m_ribbons.SetCount(numEmitters);
  shared->ribbonOrder.SetCount(numEmitters);

  for (UINT i = 0; i < numEmitters; ++i) {
    UINT bytesThisEmitter = *reinterpret_cast<UINT *>(data);
    ASSERT(dataDone >= (data + bytesThisEmitter));

    modelptr->m_ribbons[i] = RibbonManager::GetInstance()->CreateEmitter();
    UINT objectId = *reinterpret_cast<UINT *>(data + 0x58);
    shared->ribbonOrder[i] = objectId;
    LoadEmitterData(data + 4, modelptr, modelptr->m_ribbons[i]);

    data += bytesThisEmitter;
  }

  ASSERT(dataDone == data);
}
