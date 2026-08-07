#include "Base/Status.h"
#include "Gx/Gx.h"
#include "MDLFile/MDLTypes.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Services/Texture.h"
#include "Tempest/cmath.h"

#include <storm.h>
#include <string.h>

static const char WOW_DATA_PATH[35] = "\\\\Guldan\\Drive2\\Projects\\WoW\\Data\\";

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);
BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status);
BOOL  MDLFileRead(LPCSTR path, MDLDATA *data, CStatus *status);

CParticleEmitter2 *CreateEmitter(BYTE *emitterData, const MDLTEXTURESECTION *textures, UINT flags, CStatus *status);
static CParticleEmitter2 *
CreateEmitter(const MDLPARTICLEEMITTER2 &emitterData, const TSGrowableArray<MDLTEXTURESECTION> &textures, UINT flags, CStatus *status);
HTEXTURE LoadModelTexture(LPCSTR texturePath, UINT modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);

static CParticleEmitter2 *CreateEmitterObject(UINT type) {
  ParticleSystemManager *manager = ParticleSystemManager::GetInstance();
  ASSERT(manager);

  if (type == CParticleEmitter2::PET_SPHERE_EMITTER) {
    return manager->CreateSphereEmitter();
  }
  if (type == CParticleEmitter2::PET_SPLINE_EMITTER) {
    return manager->CreateSplineEmitter();
  }
  return manager->CreateQuadEmitter();
}

static void SetMaterialBlendMode(UINT blendMode, CParticleMat *mat) {
  switch (blendMode) {
    case 0:
      mat->alpha = GxBlend_Alpha;
      mat->enableDepthWrites = 0;
      break;
    case 1:
      mat->alpha = GxBlend_Add;
      mat->enableDepthWrites = 0;
      break;
    case 2:
      mat->alpha = GxBlend_Mod;
      mat->enableDepthWrites = 0;
      break;
    case 3:
      mat->alpha = GxBlend_Mod2x;
      mat->enableDepthWrites = 0;
      break;
    case 4:
      mat->alpha = GxBlend_AlphaKey;
      break;
  }
}

static void SetParticleStyle(const MDLPARTICLEEMITTER2 &emitterData, CParticleEmitter2 *emitter) {
  UINT flags = emitterData.flags;
  if (flags & 0x00080000) {
    emitter->SetUseModelSpace(1);
  }
  if (flags & 0x00200000) {
    emitter->SetInstantVel(1);
  }
  if (flags & 0x00100000) {
    emitter->SetInheritScale(1);
  }
  if (flags & 0x04000000) {
    emitter->SetExtrude(1);
  }
  if (flags & 0x08000000) {
    emitter->SetXYQuads(1);
  }
  if (emitterData.emitterType == MDLPARTICLEEMITTER2::PET_SPHERE) {
    if (flags & 0x00400000) {
      emitter->Set0XKill(1);
    }
    if (flags & 0x00800000) {
      emitter->SetZVelOnly(1);
    }
  }
  if (flags & 0x01000000) {
    emitter->SetTumbleReverse(1);
  }
  if (flags & 0x10000000) {
    emitter->SetProject(1);
  }
  if (flags & 0x20000000) {
    emitter->SetFollow(1);
  }

  UINT tailGrows = (flags & 0x02000000) != 0;
  switch (emitterData.type) {
    case MDLPARTICLEEMITTER2::PT_HEAD:
      emitter->SetParticleStyle(1, 0, emitterData.tailLength, tailGrows);
      break;
    case MDLPARTICLEEMITTER2::PT_TAIL:
      emitter->SetParticleStyle(0, 1, emitterData.tailLength, tailGrows);
      break;
    case MDLPARTICLEEMITTER2::PT_BOTH:
      emitter->SetParticleStyle(1, 1, emitterData.tailLength, tailGrows);
      break;
  }
}

static UINT GetEmitterFlags(const BYTE *emitterData) {
  return *reinterpret_cast<const UINT *>(emitterData + 0x5C);
}

UINT SetParticleStyle(const BYTE *emitterData, UINT flags, CParticleEmitter2 *emitter) {
  const UINT  style = *reinterpret_cast<const UINT *>(emitterData);
  const float tailLength = *reinterpret_cast<const float *>(emitterData + 4);

  if (flags & 0x00080000) {
    emitter->SetUseModelSpace(1);
  }
  if (flags & 0x00200000) {
    emitter->SetInstantVel(1);
  }
  if (flags & 0x00100000) {
    emitter->SetInheritScale(1);
  }
  if (flags & 0x04000000) {
    emitter->SetExtrude(1);
  }
  if (flags & 0x08000000) {
    emitter->SetXYQuads(1);
  }
  if (emitter->EmitterType() == CParticleEmitter2::PET_SPHERE_EMITTER) {
    if (flags & 0x00400000) {
      emitter->Set0XKill(1);
    }
    if (flags & 0x00800000) {
      emitter->SetZVelOnly(1);
    }
  }
  if (flags & 0x01000000) {
    emitter->SetTumbleReverse(1);
  }
  if (flags & 0x10000000) {
    emitter->SetProject(1);
  }
  if (flags & 0x20000000) {
    emitter->SetFollow(1);
  }

  const UINT tailGrows = (flags & 0x02000000) != 0;
  switch (style) {
    case 0:
      emitter->SetParticleStyle(1, 0, tailLength, tailGrows);
      break;
    case 1:
      emitter->SetParticleStyle(0, 1, tailLength, tailGrows);
      break;
    case 2:
      emitter->SetParticleStyle(1, 1, tailLength, tailGrows);
      break;
  }
  return 8;
}

static BYTE *SetKeyColors(BYTE *emitterData, CParticleKey *key1, CParticleKey *key2) {
  NTempest::CImVector startColor;
  NTempest::CImVector middleColor;
  NTempest::CImVector endColor;

  const float *colors = reinterpret_cast<const float *>(emitterData);
  startColor.r = NTempest::CMath::ftol_0_256_(colors[0] * 255.0f);
  startColor.g = NTempest::CMath::ftol_0_256_(colors[1] * 255.0f);
  startColor.b = NTempest::CMath::ftol_0_256_(colors[2] * 255.0f);
  middleColor.r = NTempest::CMath::ftol_0_256_(colors[3] * 255.0f);
  middleColor.g = NTempest::CMath::ftol_0_256_(colors[4] * 255.0f);
  middleColor.b = NTempest::CMath::ftol_0_256_(colors[5] * 255.0f);
  endColor.r = NTempest::CMath::ftol_0_256_(colors[6] * 255.0f);
  endColor.g = NTempest::CMath::ftol_0_256_(colors[7] * 255.0f);
  endColor.b = NTempest::CMath::ftol_0_256_(colors[8] * 255.0f);

  const BYTE *alpha = emitterData + 9 * sizeof(float);
  startColor.a = alpha[0];
  middleColor.a = alpha[1];
  endColor.a = alpha[2];

  key1->SetColors(startColor, middleColor);
  key2->SetColors(middleColor, endColor);
  return const_cast<BYTE *>(alpha + 3);
}

BYTE *SetParticleKeys(BYTE *emitterData, float lifeSpan, CParticleEmitter2 *emitter) {
  CParticleKey key2;
  CParticleKey key1;
  float        middleScale;
  float        startScale;
  float        middleTime;
  float        endScale;
  UINT         repeat;

  middleTime = *reinterpret_cast<float *>(emitterData);
  emitterData = SetKeyColors(emitterData + sizeof(float), &key1, &key2);

  const float *scales = reinterpret_cast<const float *>(emitterData);
  startScale = scales[0];
  middleScale = scales[1];
  endScale = scales[2];
  key1.SetScales(startScale, middleScale);
  key2.SetScales(middleScale, endScale);
  emitterData += 3 * sizeof(float);

  const int *cells = reinterpret_cast<const int *>(emitterData);
  key1.SetHeadCells(cells[0], cells[1]);
  key1.SetSegment(0.0f, middleTime);
  repeat = static_cast<UINT>(cells[2]);
  key1.SetRepeat(static_cast<float>(repeat));
  key1.SetLifeSpan(lifeSpan);

  cells += 3;
  key2.SetHeadCells(cells[0], cells[1]);
  key2.SetSegment(middleTime, 1.0f);
  repeat = static_cast<UINT>(cells[2]);
  key2.SetRepeat(static_cast<float>(repeat));
  key2.SetLifeSpan(lifeSpan);

  cells += 3;
  key1.SetTailCells(cells[0], cells[1]);
  cells += 3;
  key2.SetTailCells(cells[0], cells[1]);
  cells += 3;

  emitter->SetKey(0, key1);
  emitter->SetKey(1, key2);
  return reinterpret_cast<BYTE *>(const_cast<int *>(cells));
}

static void SetParticleKeys(const MDLPARTICLEEMITTER2 &emitterData, CParticleEmitter2 *emitter) {
  CParticleKey key1;
  CParticleKey key2;

  key1.SetSegment(0.0f, emitterData.middleTime);
  key1.SetRepeat(static_cast<float>(emitterData.lifespanUVAnimRepeat));
  key1.SetLifeSpan(emitterData.staticLife);
  key2.SetSegment(emitterData.middleTime, 1.0f);
  key2.SetRepeat(static_cast<float>(emitterData.decayUVAnimRepeat));
  key2.SetLifeSpan(emitterData.staticLife);

  NTempest::CImVector startColor;
  startColor.r = NTempest::CMath::ftol_0_256_(emitterData.startColor.r * 255.0f);
  startColor.g = NTempest::CMath::ftol_0_256_(emitterData.startColor.g * 255.0f);
  startColor.b = NTempest::CMath::ftol_0_256_(emitterData.startColor.b * 255.0f);
  startColor.a = emitterData.startAlpha;

  NTempest::CImVector middleColor;
  middleColor.r = NTempest::CMath::ftol_0_256_(emitterData.middleColor.r * 255.0f);
  middleColor.g = NTempest::CMath::ftol_0_256_(emitterData.middleColor.g * 255.0f);
  middleColor.b = NTempest::CMath::ftol_0_256_(emitterData.middleColor.b * 255.0f);
  middleColor.a = emitterData.middleAlpha;

  NTempest::CImVector endColor;
  endColor.r = NTempest::CMath::ftol_0_256_(emitterData.endColor.r * 255.0f);
  endColor.g = NTempest::CMath::ftol_0_256_(emitterData.endColor.g * 255.0f);
  endColor.b = NTempest::CMath::ftol_0_256_(emitterData.endColor.b * 255.0f);
  endColor.a = emitterData.endAlpha;

  key1.SetColors(startColor, middleColor);
  key2.SetColors(middleColor, endColor);
  key1.SetHeadCells(emitterData.lifespanUVAnimStart, emitterData.lifespanUVAnimEnd);
  key2.SetHeadCells(emitterData.decayUVAnimStart, emitterData.decayUVAnimEnd);
  key1.SetTailCells(emitterData.tailUVAnimStart, emitterData.tailUVAnimEnd);
  key2.SetTailCells(emitterData.tailDecayUVAnimStart, emitterData.tailDecayUVAnimEnd);
  key1.SetScales(emitterData.startScale, emitterData.middleScale);
  key2.SetScales(emitterData.middleScale, emitterData.endScale);
  emitter->SetKey(0, key1);
  emitter->SetKey(1, key2);
}

HTEXTURE LoadModelTexture(LPCSTR texturePath, UINT modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status) {
  if (!(modelLoadFlags & 0x4000) || texturePath[1] == ':' || texturePath[0] == '\\') {
    return TextureCreate(texturePath, texLoadFlags, status, 0);
  }

  char path[MAX_PATH];
  SStrCopy(path, WOW_DATA_PATH, sizeof(path));
  SStrPack(path, texturePath, sizeof(path));
  return TextureCreate(path, texLoadFlags, status, 0);
}

static BYTE *CreateParticleMaterial(
    BYTE                    *emitterData,
    const MDLTEXTURESECTION *textures,
    UINT                     emitterFlags,
    UINT                     modelCreateFlags,
    CStatus                 *status,
    CParticleEmitter2       *emitter
) {
  CParticleMat newMat;

  SetMaterialBlendMode(*reinterpret_cast<UINT *>(emitterData), &newMat);
  emitterData += 4;

  const UINT               textureIndex = *reinterpret_cast<UINT *>(emitterData);
  const MDLTEXTURESECTION &texture = textures[textureIndex];
  HTEXTURE                 hTexture = LoadModelTexture(texture.image, modelCreateFlags, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), status);
  emitterData += 4;

  if (emitterFlags & 0x00008000) {
    newMat.enableLighting = 0;
  }
  if (emitterFlags & 0x00040000) {
    newMat.enableFog = 0;
  }
  if (emitterFlags & 0x00010000) {
    emitter->SetSortZ(1);
  }
  emitter->SetMaterial(newMat, hTexture);
  HandleClose(hTexture);
  return emitterData;
}

static void CreateParticleMaterial(
    const MDLPARTICLEEMITTER2                &emitterData,
    const TSGrowableArray<MDLTEXTURESECTION> &textures,
    UINT                                      flags,
    CStatus                                  *status,
    CParticleEmitter2                        *emitter
) {
  CParticleMat newMat;

  SetMaterialBlendMode(emitterData.blendMode, &newMat);
  const MDLTEXTURESECTION &texture = textures[emitterData.textureId];
  HTEXTURE                 hTexture = LoadModelTexture(texture.image, flags, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), status);
  if (emitterData.flags & 0x00008000) {
    newMat.enableLighting = 0;
  }
  if (emitterData.flags & 0x00040000) {
    newMat.enableFog = 0;
  }
  if (emitterData.flags & 0x00010000) {
    emitter->SetSortZ(1);
  }
  emitter->SetMaterial(newMat, hTexture);
  HandleClose(hTexture);
}

static BYTE *CreateChildEmitter(BYTE *emitterData, UINT flags, CStatus *status, CParticleEmitter2 *parent) {
  char *childPath = reinterpret_cast<char *>(emitterData);
  emitterData += 260;
  if (!SStrLen(childPath)) {
    return emitterData;
  }

  UINT  fileBytes = 0;
  BYTE *fileData = MDLFileBinaryLoad(childPath, &fileBytes, status);
  if (!fileData) {
    return emitterData;
  }

  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SXET');
  ASSERT(section);
  const MDLTEXTURESECTION *textures = reinterpret_cast<const MDLTEXTURESECTION *>(section + 4);

  section = MDLFileBinarySeek(fileData, fileBytes, '2ERP');
  ASSERT(section);
  UINT numEmitters = *reinterpret_cast<UINT *>(section + 4);
  if (numEmitters > 4) {
    numEmitters = 4;
  }

  BYTE *childData = section + 8;
  for (UINT i = 0; i < numEmitters; ++i) {
    const UINT         bytesThisEmitter = *reinterpret_cast<UINT *>(childData);
    CParticleEmitter2 *child = CreateEmitter(childData + 4, textures, flags, status);
    parent->AddChildEmitter(child);
    child->SetEnabled(1, 1);
    childData += bytesThisEmitter;
  }
  return emitterData;
}

static void CreateChildEmitter(const MDLPARTICLEEMITTER2 &emitterData, UINT flags, CStatus *status, CParticleEmitter2 *parent) {
  if (!static_cast<LPCSTR>(emitterData.recursionMdl)[0]) {
    return;
  }

  MDLDATA data;
  if (!MDLFileRead(emitterData.recursionMdl, &data, status) || !data.particleEmitters2.Count()) {
    return;
  }

  UINT numEmitters = data.particleEmitters2.Count();
  if (numEmitters > 4) {
    numEmitters = 4;
  }
  for (UINT i = 0; i < numEmitters; ++i) {
    CParticleEmitter2 *child = CreateEmitter(data.particleEmitters2[i], data.textures, flags, status);
    parent->AddChildEmitter(child);
    child->SetEnabled(1, 1);
  }
}

BYTE *SetParticleTumble(BYTE *emitterData, CParticleEmitter2 *emitter) {
  const float *tumble = reinterpret_cast<const float *>(emitterData);
  emitter->SetTumbleX(NTempest::C2Vector(tumble[0], tumble[1]));
  emitter->SetTumbleY(NTempest::C2Vector(tumble[2], tumble[3]));
  emitter->SetTumbleZ(NTempest::C2Vector(tumble[4], tumble[5]));
  return emitterData + 6 * sizeof(float);
}

static BYTE *LoadC3Vector(BYTE *emitterData, NTempest::C3Vector *vector) {
  const float *values = reinterpret_cast<const float *>(emitterData);
  vector->x = values[0];
  vector->y = values[1];
  vector->z = values[2];
  return emitterData + 3 * sizeof(float);
}

static CParticleEmitter2 *
CreateEmitter(const MDLPARTICLEEMITTER2 &emitterData, const TSGrowableArray<MDLTEXTURESECTION> &textures, UINT flags, CStatus *status) {
  CParticleEmitter2 *emitter = CreateEmitterObject(emitterData.emitterType);
  emitter->SetEnabled(0, 1);
  emitter->SetVelocity(emitterData.staticSpeed);
  emitter->SetVelocityVariation(emitterData.staticVariation);
  emitter->SetLatitude(emitterData.staticLatitude);
  emitter->SetLongitude(emitterData.staticLongitude);
  emitter->SetAcceleration(emitterData.staticGravity);
  emitter->SetZsource(emitterData.staticZsource);
  emitter->SetLifeSpan(emitterData.staticLife);
  emitter->SetEmissionRate(emitterData.staticEmissionRate);
  emitter->SetHeight(emitterData.staticLength);
  emitter->SetWidth(emitterData.staticWidth);
  emitter->SetTextureDimensions(emitterData.rows, emitterData.cols);

  SetParticleStyle(emitterData, emitter);
  SetParticleKeys(emitterData, emitter);
  CreateParticleMaterial(emitterData, textures, flags, status, emitter);
  emitter->SetInstantVelScale(emitterData.ivelScale);
  emitter->SetPriorityPlane(emitterData.priorityPlane);
  emitter->SetReplaceableId(emitterData.replaceableId);
  CreateChildEmitter(emitterData, flags, status, emitter);

  if (static_cast<LPCSTR>(emitterData.geometryMdl)[0]) {
    CModelCreate modelCreate;
    modelCreate.flags = flags & 0xFFFFFFB9;
    memset(&modelCreate.sequenceNames, 0, sizeof(modelCreate) - sizeof(modelCreate.flags));
    emitter->SetModel(ModelCreate(emitterData.geometryMdl, &modelCreate, status));
  }

  emitter->SetTumbleX(NTempest::C2Vector(emitterData.tumblexMin, emitterData.tumblexMax));
  emitter->SetTumbleY(NTempest::C2Vector(emitterData.tumbleyMin, emitterData.tumbleyMax));
  emitter->SetTumbleZ(NTempest::C2Vector(emitterData.tumblezMin, emitterData.tumblezMax));
  emitter->SetTwinkleFPS(emitterData.twinkleFPS);
  emitter->SetTwinkleOnOff(emitterData.twinkleOnOff);
  if (emitterData.twinkleOnOff == 0.0f) {
    status->Add(STATUS_WARNING, "Particle system with 0 twinkle\n");
  }
  emitter->SetTwinkleScale(emitterData.twinkleScaleMin, emitterData.twinkleScaleMax);
  emitter->SetDrag(emitterData.drag);
  emitter->SetAngularVelocity(emitterData.spin);
  emitter->SetWind(emitterData.windVector, emitterData.windTime);
  emitter->SetFollowParams(emitterData.followSpeed1, emitterData.followScale1, emitterData.followSpeed2, emitterData.followScale2);
  if (emitterData.spline.Count()) {
    static_cast<CSplineParticleEmitter *>(emitter)->SetSpline(emitterData.spline.Ptr(), emitterData.spline.Count());
  }
  return emitter;
}

CParticleEmitter2 *CreateEmitter(BYTE *emitterData, const MDLTEXTURESECTION *textures, UINT flags, CStatus *status) {
  const UINT emitterFlags = GetEmitterFlags(emitterData);
  BYTE      *cursor = emitterData;
  cursor += *reinterpret_cast<UINT *>(cursor) + 4;
  const UINT emitterType = *reinterpret_cast<UINT *>(cursor);
  cursor += 4;

  CParticleEmitter2 *emitter = CreateEmitterObject(emitterType);
  emitter->SetEnabled(0, 1);

  emitter->SetVelocity(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetVelocityVariation(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetLatitude(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetLongitude(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetAcceleration(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetZsource(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  const float lifeSpan = *reinterpret_cast<float *>(cursor);
  emitter->SetLifeSpan(lifeSpan);
  cursor += 4;
  emitter->SetEmissionRate(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetHeight(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetWidth(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  const UINT textureRows = *reinterpret_cast<UINT *>(cursor);
  cursor += 4;
  const UINT textureColumns = *reinterpret_cast<UINT *>(cursor);
  cursor += 4;
  emitter->SetTextureDimensions(textureRows, textureColumns);

  cursor += SetParticleStyle(cursor, emitterFlags, emitter);
  cursor = SetParticleKeys(cursor, lifeSpan, emitter);
  cursor = CreateParticleMaterial(cursor, textures, emitterFlags, flags, status, emitter);

  emitter->SetPriorityPlane(*reinterpret_cast<int *>(cursor));
  cursor += 4;
  emitter->SetReplaceableId(*reinterpret_cast<UINT *>(cursor));
  cursor += 4;

  LPCSTR modelPath = reinterpret_cast<LPCSTR>(cursor);
  cursor += 260;
  if (SStrLen(modelPath)) {
    CModelCreate modelCreate;
    modelCreate.flags = flags & 0xFFFFFFB9;
    memset(&modelCreate.sequenceNames, 0, sizeof(modelCreate) - sizeof(modelCreate.flags));
    emitter->SetModel(ModelCreate(modelPath, &modelCreate, status));
  }

  cursor = CreateChildEmitter(cursor, flags, status, emitter);

  const float *twinkle = reinterpret_cast<const float *>(cursor);
  emitter->SetTwinkleFPS(twinkle[0]);
  emitter->SetTwinkleOnOff(twinkle[1]);
  if (twinkle[1] == 0.0f) {
    status->Add(STATUS_WARNING, "Particle system with 0 twinkle\n");
  }
  emitter->SetTwinkleScale(twinkle[2], twinkle[3]);
  emitter->SetInstantVelScale(twinkle[4]);
  cursor += 5 * sizeof(float);

  cursor = SetParticleTumble(cursor, emitter);
  emitter->SetDrag(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  emitter->SetAngularVelocity(*reinterpret_cast<float *>(cursor));
  cursor += 4;
  NTempest::C3Vector windVector;
  cursor = LoadC3Vector(cursor, &windVector);
  emitter->SetWind(windVector, *reinterpret_cast<float *>(cursor));
  cursor += 4;

  const float *follow = reinterpret_cast<const float *>(cursor);
  emitter->SetFollowParams(follow[0], follow[1], follow[2], follow[3]);
  cursor += 4 * sizeof(float);

  const UINT numSplinePoints = *reinterpret_cast<UINT *>(cursor);
  cursor += 4;
  if (numSplinePoints) {
    static_cast<CSplineParticleEmitter *>(emitter)->SetSpline(reinterpret_cast<const NTempest::C3Vector *>(cursor), numSplinePoints);
  }
  return emitter;
}

BOOL MdlReadLoadEmitters2(const MDLDATA &data, CModelComplex *modelptr, CModelShared *shared, UINT flags, CStatus *status) {
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  UINT numEmitters = data.particleEmitters2.Count();
  modelptr->m_emitters2.SetCount(numEmitters);
  shared->emitter2Order.SetCount(numEmitters);

  for (UINT i = 0; i < numEmitters; ++i) {
    shared->emitter2Order[i] = data.particleEmitters2[i].objectId;
    modelptr->m_emitters2[i] = CreateEmitter(data.particleEmitters2[i], data.textures, flags, status);
  }
  return 1;
}

void MdxReadEmitters2(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  ASSERT(status);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, '2ERP');
  if (!section) {
    return;
  }

  BYTE *texData = MDLFileBinarySeek(data, fileBytes, 'SXET');
  ASSERT(texData);
  const MDLTEXTURESECTION *textures = reinterpret_cast<const MDLTEXTURESECTION *>(texData + 4);

  UINT  sectionBytes = *reinterpret_cast<UINT *>(section) - 4;
  UINT  numEmitters = *reinterpret_cast<UINT *>(section + 4);
  BYTE *emitterData = section + 8;

  modelptr->m_emitters2.SetCount(numEmitters);
  shared->emitter2Order.SetCount(numEmitters);
  for (UINT i = 0; i < numEmitters; ++i) {
    UINT bytesThisEmitter = *reinterpret_cast<UINT *>(emitterData);
    shared->emitter2Order[i] = *reinterpret_cast<UINT *>(emitterData + 0x58);
    modelptr->m_emitters2[i] = CreateEmitter(emitterData + 4, textures, flags, status);

    ASSERT(sectionBytes >= bytesThisEmitter);
    emitterData += bytesThisEmitter;
    sectionBytes -= bytesThisEmitter;
  }
  ASSERT(sectionBytes == 0);
}
