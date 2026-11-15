#include "Base/Status.h"
#include "Gx/Gx.h"
#include "MDLFile/MDLTypes.h"
#include "Model/IModel.h"
#include "Services/ParticleSystem2.h"
#include "Services/Texture.h"
#include "Tempest/cmath.h"

#include <storm.h>
#include <string.h>

static const char WOW_DATA_PATH[35] = "\\\\Guldan\\Drive2\\Projects\\WoW\\Data\\";

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);
BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status);
BOOL  MDLFileRead(LPCSTR path, MDLDATA *data, CStatus *status);

static CParticleEmitter2 *CreateEmitter(BYTE *emitterData, const MDLTEXTURESECTION *textures, UINT flags, CStatus *status);
static CParticleEmitter2 *
CreateEmitter(const MDLPARTICLEEMITTER2 &emitterData, const TSGrowableArray<MDLTEXTURESECTION> &textures, UINT flags, CStatus *status);
HTEXTURE LoadModelTexture(LPCSTR texturePath, UINT modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);

static CParticleEmitter2 *CreateEmitterObject(UINT type) {
  ParticleSystemManager *partMgr = ParticleSystemManager::GetInstance();
  ASSERT(partMgr);

  switch (type) {
    case CParticleEmitter2::PET_SPHERE_EMITTER:
      return partMgr->CreateSphereEmitter();
    case CParticleEmitter2::PET_SPLINE_EMITTER:
      return partMgr->CreateSplineEmitter();
    default:
      return partMgr->CreateQuadEmitter();
  }
}

static void SetMaterialBlendMode(UINT blendMode, CParticleMat *mat) {
  switch (blendMode) {
    case 1:
      mat->alpha = GxBlend_Add;
      mat->enableDepthWrites = 0;
      break;
    case 0:
      mat->alpha = GxBlend_Alpha;
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
  if (emitterData.flags & 0x00080000) {
    emitter->SetUseModelSpace(1);
  }
  if (emitterData.flags & 0x00200000) {
    emitter->SetInstantVel(1);
  }
  if (emitterData.flags & 0x00100000) {
    emitter->SetInheritScale(1);
  }
  if (emitterData.flags & 0x04000000) {
    emitter->SetExtrude(1);
  }
  if (emitterData.flags & 0x08000000) {
    emitter->SetXYQuads(1);
  }
  if (emitterData.emitterType == MDLPARTICLEEMITTER2::PET_SPHERE) {
    if (emitterData.flags & 0x00400000) {
      emitter->Set0XKill(1);
    }
    if (emitterData.flags & 0x00800000) {
      emitter->SetZVelOnly(1);
    }
  }
  if (emitterData.flags & 0x01000000) {
    emitter->SetTumbleReverse(1);
  }
  if (emitterData.flags & 0x10000000) {
    emitter->SetProject(1);
  }
  if (emitterData.flags & 0x20000000) {
    emitter->SetFollow(1);
  }

  bool tailGrows = (emitterData.flags & 0x02000000) != 0;
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
  return *(const UINT *)(emitterData + 0x5C);
}

static UINT SetParticleStyle(const BYTE *emitterData, UINT flags, CParticleEmitter2 *emitter) {
  const UINT  style = *(const UINT *)emitterData;
  const float tailLength = *(const float *)(emitterData + 4);

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

  bool tailGrows = (flags & 0x02000000) != 0;
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

static inline BYTE ColorToByte(float c) {
  return NTempest::CMath::ftol_0_256_(c * 255.0f);
}

static BYTE *SetKeyColors(BYTE *emitterData, CParticleKey *key1, CParticleKey *key2) {
  NTempest::CImVector startColor;
  NTempest::CImVector middleColor;
  NTempest::CImVector endColor;

  startColor.r = ColorToByte(((float *)emitterData)[0]);
  startColor.g = ColorToByte(((float *)emitterData)[1]);
  startColor.b = ColorToByte(((float *)emitterData)[2]);
  emitterData += 3 * sizeof(float);

  middleColor.r = ColorToByte(((float *)emitterData)[0]);
  middleColor.g = ColorToByte(((float *)emitterData)[1]);
  middleColor.b = ColorToByte(((float *)emitterData)[2]);
  emitterData += 3 * sizeof(float);

  endColor.r = ColorToByte(((float *)emitterData)[0]);
  endColor.g = ColorToByte(((float *)emitterData)[1]);
  endColor.b = ColorToByte(((float *)emitterData)[2]);
  emitterData += 3 * sizeof(float);

  startColor.a = emitterData[0];
  middleColor.a = emitterData[1];
  endColor.a = emitterData[2];

  key1->SetColors(startColor, middleColor);
  key2->SetColors(middleColor, endColor);
  return emitterData + 3;
}

static BYTE *SetParticleKeys(BYTE *emitterData, float lifeSpan, CParticleEmitter2 *emitter) {
  CParticleKey key2;
  CParticleKey key1;
  float        middleScale;
  float        startScale;
  float        middleTime;
  float        endScale;
  UINT         repeat;

  middleTime = *(float *)emitterData;
  emitterData = SetKeyColors(emitterData + sizeof(float), &key1, &key2);

  const float *scales = (const float *)emitterData;
  startScale = scales[0];
  middleScale = scales[1];
  endScale = scales[2];
  key1.SetScales(startScale, middleScale);
  key2.SetScales(middleScale, endScale);
  emitterData += 3 * sizeof(float);

  const int *cells = (const int *)emitterData;
  key1.SetHeadCells(cells[0], cells[1]);
  key1.SetSegment(0.0f, middleTime);
  repeat = cells[2];
  key1.SetRepeat((float)repeat);
  key1.SetLifeSpan(lifeSpan);

  cells += 3;
  key2.SetHeadCells(cells[0], cells[1]);
  key2.SetSegment(middleTime, 1.0f);
  repeat = (UINT)cells[2];
  key2.SetRepeat((float)repeat);
  key2.SetLifeSpan(lifeSpan);

  cells += 3;
  key1.SetTailCells(cells[0], cells[1]);
  cells += 3;
  key2.SetTailCells(cells[0], cells[1]);
  cells += 3;

  emitter->SetKey(0, key1);
  emitter->SetKey(1, key2);
  return (BYTE *)((int *)cells);
}

static void SetParticleKeys(const MDLPARTICLEEMITTER2 &emitterData, CParticleEmitter2 *emitter) {
  CParticleKey key1;
  CParticleKey key2;

  key1.SetSegment(0.0f, emitterData.middleTime);
  key1.SetRepeat((float)emitterData.lifespanUVAnimRepeat);
  key1.SetLifeSpan(emitterData.staticLife);
  key2.SetSegment(emitterData.middleTime, 1.0f);
  key2.SetRepeat((float)emitterData.decayUVAnimRepeat);
  key2.SetLifeSpan(emitterData.staticLife);

  NTempest::CImVector startColor;
  NTempest::CImVector middleColor;
  NTempest::CImVector endColor;
  startColor.r = ColorToByte(emitterData.startColor.r);
  startColor.g = ColorToByte(emitterData.startColor.g);
  startColor.b = ColorToByte(emitterData.startColor.b);
  startColor.a = emitterData.startAlpha;

  middleColor.r = ColorToByte(emitterData.middleColor.r);
  middleColor.g = ColorToByte(emitterData.middleColor.g);
  middleColor.b = ColorToByte(emitterData.middleColor.b);
  middleColor.a = emitterData.middleAlpha;

  endColor.r = ColorToByte(emitterData.endColor.r);
  endColor.g = ColorToByte(emitterData.endColor.g);
  endColor.b = ColorToByte(emitterData.endColor.b);
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
  UINT blendMode = *(UINT *)emitterData;
  emitterData += 4;

  CParticleMat newMat;
  SetMaterialBlendMode(blendMode, &newMat);

  HTEXTURE hTexture = LoadModelTexture(
      textures[*(UINT *)emitterData].image, modelCreateFlags, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), status
  );
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

static BYTE *CreateChildEmitter(BYTE *parentData, UINT flags, CStatus *status, CParticleEmitter2 *parent) {
  char *childPath = (char *)parentData;
  parentData += 260;
  if (!SStrLen(childPath)) {
    return parentData;
  }

  UINT  fileBytes;
  BYTE *fileData = MDLFileBinaryLoad(childPath, &fileBytes, status);
  if (!fileData) {
    return parentData;
  }

  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SXET');
  ASSERT(section);
  section += 4;
  const MDLTEXTURESECTION *textures = (const MDLTEXTURESECTION *)section;

  section = MDLFileBinarySeek(fileData, fileBytes, '2ERP');
  ASSERT(section);
  section += 4;
  UINT numEmitters = *(UINT *)section;
  section += 4;
  numEmitters = min(numEmitters, 4);

  for (UINT i = 0; i < numEmitters; ++i) {
    UINT bytesThisEmitter = *(UINT *)section;
    section += 4;
    CParticleEmitter2 *child = CreateEmitter(section, textures, flags, status);
    parent->AddChildEmitter(child);
    child->SetEnabled(1, 1);
    section += bytesThisEmitter - 4;
  }
  return parentData;
}

static void CreateChildEmitter(const MDLPARTICLEEMITTER2 &emitterData, UINT flags, CStatus *status, CParticleEmitter2 *parent) {
  if (!((LPCSTR)emitterData.recursionMdl)[0]) {
    return;
  }

  MDLDATA mdl;
  if (!MDLFileRead(emitterData.recursionMdl, &mdl, NULL) || !mdl.particleEmitters2.Count()) {
    return;
  }

  UINT numEmitters = min(mdl.particleEmitters2.Count(), 4);
  for (UINT i = 0; i < numEmitters; ++i) {
    CParticleEmitter2 *child = CreateEmitter(mdl.particleEmitters2[i], mdl.textures, flags, status);
    parent->AddChildEmitter(child);
    child->SetEnabled(1, 1);
  }
}

static BYTE *SetParticleTumble(BYTE *emitterData, CParticleEmitter2 *emitter) {
  float tumblexMin = *(float *)emitterData;
  emitterData += 4;
  float tumblexMax = *(float *)emitterData;
  emitterData += 4;
  float tumbleyMin = *(float *)emitterData;
  emitterData += 4;
  float tumbleyMax = *(float *)emitterData;
  emitterData += 4;
  float tumblezMin = *(float *)emitterData;
  emitterData += 4;
  float tumblezMax = *(float *)emitterData;
  emitterData += 4;
  emitter->SetTumbleX(NTempest::C2Vector(tumblexMin, tumblexMax));
  emitter->SetTumbleY(NTempest::C2Vector(tumbleyMin, tumbleyMax));
  emitter->SetTumbleZ(NTempest::C2Vector(tumblezMin, tumblezMax));
  return emitterData;
}

static BYTE *LoadC3Vector(BYTE *emitterData, NTempest::C3Vector *vector) {
  vector->x = *(float *)emitterData;
  emitterData += sizeof(float);
  vector->y = *(float *)emitterData;
  emitterData += sizeof(float);
  vector->z = *(float *)emitterData;
  emitterData += sizeof(float);
  return emitterData;
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

  if (SStrLen(emitterData.geometryMdl)) {
    CModelCreate modelCreate;
    modelCreate.flags = flags & 0xFFFFFFB9;
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
    ((CSplineParticleEmitter *)emitter)->SetSpline(emitterData.spline.Ptr(), emitterData.spline.Count());
  }
  return emitter;
}

static CParticleEmitter2 *CreateEmitter(BYTE *emitterData, const MDLTEXTURESECTION *textures, UINT flags, CStatus *status) {
  const UINT emitterBytes = *(UINT *)emitterData;
  const UINT emitterFlags = GetEmitterFlags(emitterData);
  BYTE      *cursor = emitterData + emitterBytes + 4;
  const UINT emitterType = *(UINT *)cursor;
  cursor += 4;

  CParticleEmitter2 *emitter = CreateEmitterObject(emitterType);
  emitter->SetEnabled(0, 1);

  emitter->SetVelocity(*(float *)cursor);
  cursor += 4;
  emitter->SetVelocityVariation(*(float *)cursor);
  cursor += 4;
  emitter->SetLatitude(*(float *)cursor);
  cursor += 4;
  emitter->SetLongitude(*(float *)cursor);
  cursor += 4;
  emitter->SetAcceleration(*(float *)cursor);
  cursor += 4;
  emitter->SetZsource(*(float *)cursor);
  cursor += 4;
  const float lifeSpan = *(float *)cursor;
  emitter->SetLifeSpan(lifeSpan);
  cursor += 4;
  emitter->SetEmissionRate(*(float *)cursor);
  cursor += 4;
  emitter->SetHeight(*(float *)cursor);
  cursor += 4;
  emitter->SetWidth(*(float *)cursor);
  cursor += 4;
  const UINT textureRows = *(UINT *)cursor;
  cursor += 4;
  const UINT textureColumns = *(UINT *)cursor;
  cursor += 4;
  emitter->SetTextureDimensions(textureRows, textureColumns);

  cursor += SetParticleStyle(cursor, emitterFlags, emitter);
  cursor = SetParticleKeys(cursor, lifeSpan, emitter);
  cursor = CreateParticleMaterial(cursor, textures, emitterFlags, flags, status, emitter);

  emitter->SetPriorityPlane(*(int *)cursor);
  cursor += 4;
  emitter->SetReplaceableId(*(UINT *)cursor);
  cursor += 4;

  LPCSTR modelPath = (LPCSTR)cursor;
  cursor += 260;
  if (SStrLen(modelPath)) {
    CModelCreate modelCreate;
    modelCreate.flags = flags & 0xFFFFFFB9;
    emitter->SetModel(ModelCreate(modelPath, &modelCreate, status));
  }

  cursor = CreateChildEmitter(cursor, flags, status, emitter);

  emitter->SetTwinkleFPS(*(float *)cursor);
  cursor += 4;
  float twinkleOnOff = *(float *)cursor;
  cursor += 4;
  emitter->SetTwinkleOnOff(twinkleOnOff);
  if (twinkleOnOff == 0.0f) {
    status->Add(STATUS_WARNING, "Particle system with 0 twinkle\n");
  }

  float twinkleScaleMin = *(float *)cursor;
  cursor += 4;
  float twinkleScaleMax = *(float *)cursor;
  cursor += 4;
  emitter->SetTwinkleScale(twinkleScaleMin, twinkleScaleMax);

  emitter->SetInstantVelScale(*(float *)cursor);
  cursor += 4;

  cursor = SetParticleTumble(cursor, emitter);
  emitter->SetDrag(*(float *)cursor);
  cursor += 4;
  emitter->SetAngularVelocity(*(float *)cursor);
  cursor += 4;
  NTempest::C3Vector windVector;
  cursor = LoadC3Vector(cursor, &windVector);
  emitter->SetWind(windVector, *(float *)cursor);
  cursor += 4;

  const float *follow = (const float *)cursor;
  emitter->SetFollowParams(follow[0], follow[1], follow[2], follow[3]);
  cursor += 4 * sizeof(float);

  const UINT numSplinePoints = *(UINT *)cursor;
  cursor += 4;
  if (numSplinePoints) {
    ((CSplineParticleEmitter *)emitter)->SetSpline((const NTempest::C3Vector *)cursor, numSplinePoints);
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
  UINT sectionBytes = *(UINT *)section;
  section += 4;
  UINT numEmitters = *(UINT *)section;
  section += 4;
  sectionBytes -= 4;
  texData += 4;
  MDLTEXTURESECTION *textures = (MDLTEXTURESECTION *)texData;
  BYTE *emitterData = section;

  modelptr->m_emitters2.SetCount(numEmitters);
  shared->emitter2Order.SetCount(numEmitters);
  for (UINT i = 0; i < numEmitters; ++i) {
    UINT bytesThisEmitter = *(UINT *)emitterData;
    UINT objectId = *(UINT *)(emitterData + 0x58);
    shared->emitter2Order[i] = objectId;
    modelptr->m_emitters2[i] = CreateEmitter(emitterData + 4, textures, flags, status);
    emitterData += bytesThisEmitter;

    ASSERT(sectionBytes >= bytesThisEmitter);
    sectionBytes -= bytesThisEmitter;
  }
  ASSERT(sectionBytes == 0);
}
