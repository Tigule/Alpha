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

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);
unsigned char *__fastcall MDLFileBinaryLoad(char *path, unsigned int *fileBytes, CStatus *status);

CParticleEmitter2 *__fastcall CreateEmitter(unsigned char *emitterData, const MDLTEXTURESECTION *textures, unsigned int flags, CStatus *status);
HTEXTURE __fastcall           LoadModelTexture(const char *texturePath, unsigned int modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);

static CParticleEmitter2 *__fastcall CreateEmitterObject(unsigned int type) {
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

static void __fastcall SetMaterialBlendMode(unsigned int blendMode, CParticleMat *mat) {
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

static unsigned int __fastcall GetEmitterFlags(const unsigned char *emitterData) {
  return *reinterpret_cast<const unsigned int *>(emitterData + 0x5C);
}

unsigned int __fastcall SetParticleStyle(const unsigned char *emitterData, unsigned int flags, CParticleEmitter2 *emitter) {
  const unsigned int style = *reinterpret_cast<const unsigned int *>(emitterData);
  const float        tailLength = *reinterpret_cast<const float *>(emitterData + 4);

  if (flags & 0x00080000) {
    emitter->m_useModelSpace = 1;
  }
  if (flags & 0x00200000) {
    emitter->m_instantVelLin = 1;
  }
  if (flags & 0x00100000) {
    emitter->m_inheritScale = 1;
  }
  if (flags & 0x04000000) {
    emitter->m_extrude = 1;
  }
  if (flags & 0x08000000) {
    emitter->m_xyQuads = 1;
  }
  if (emitter->m_emitterType == CParticleEmitter2::PET_SPHERE_EMITTER) {
    if (flags & 0x00400000) {
      emitter->m_0XKill = 1;
    }
    if (flags & 0x00800000) {
      emitter->m_zvelOnly = 1;
    }
  }
  if (flags & 0x01000000) {
    emitter->m_tumbler = 1;
  }
  if (flags & 0x10000000) {
    emitter->m_project = 1;
  }
  if (flags & 0x20000000) {
    emitter->m_follow = 1;
  }

  const unsigned int tailGrows = (flags & 0x02000000) != 0;
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

static unsigned char *__fastcall SetKeyColors(unsigned char *emitterData, CParticleKey *key1, CParticleKey *key2) {
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

  const unsigned char *alpha = emitterData + 9 * sizeof(float);
  startColor.a = alpha[0];
  middleColor.a = alpha[1];
  endColor.a = alpha[2];

  key1->SetColors(startColor, middleColor);
  key2->SetColors(middleColor, endColor);
  return const_cast<unsigned char *>(alpha + 3);
}

unsigned char *__fastcall SetParticleKeys(unsigned char *emitterData, float lifeSpan, CParticleEmitter2 *emitter) {
  CParticleKey key2;
  CParticleKey key1;
  float        middleScale;
  float        startScale;
  float        middleTime;
  float        endScale;
  unsigned int repeat;

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
  repeat = static_cast<unsigned int>(cells[2]);
  key1.SetRepeat(static_cast<float>(repeat));
  key1.SetLifeSpan(lifeSpan);

  cells += 3;
  key2.SetHeadCells(cells[0], cells[1]);
  key2.SetSegment(middleTime, 1.0f);
  repeat = static_cast<unsigned int>(cells[2]);
  key2.SetRepeat(static_cast<float>(repeat));
  key2.SetLifeSpan(lifeSpan);

  cells += 3;
  key1.SetTailCells(cells[0], cells[1]);
  cells += 3;
  key2.SetTailCells(cells[0], cells[1]);
  cells += 3;

  emitter->SetKey(0, key1);
  emitter->SetKey(1, key2);
  return reinterpret_cast<unsigned char *>(const_cast<int *>(cells));
}

HTEXTURE __fastcall LoadModelTexture(const char *texturePath, unsigned int modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status) {
  if (!(modelLoadFlags & 0x4000) || texturePath[1] == ':' || texturePath[0] == '\\') {
    return TextureCreate(texturePath, texLoadFlags, status, 0);
  }

  char path[260];
  SStrCopy(path, WOW_DATA_PATH, sizeof(path));
  SStrPack(path, texturePath, sizeof(path));
  return TextureCreate(path, texLoadFlags, status, 0);
}

static unsigned char *__fastcall CreateParticleMaterial(
    unsigned char           *emitterData,
    const MDLTEXTURESECTION *textures,
    unsigned int             emitterFlags,
    unsigned int             modelCreateFlags,
    CStatus                 *status,
    CParticleEmitter2       *emitter
) {
  CParticleMat newMat;
  newMat.alpha = GxBlend_Opaque;
  newMat.enableLighting = 1;
  newMat.enableFog = 1;
  newMat.enableDepthWrites = 1;

  SetMaterialBlendMode(*reinterpret_cast<unsigned int *>(emitterData), &newMat);
  emitterData += 4;

  const unsigned int       textureIndex = *reinterpret_cast<unsigned int *>(emitterData);
  const MDLTEXTURESECTION &texture = textures[textureIndex];
  CGxTexFlags              textureFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE                 hTexture = LoadModelTexture(texture.image, modelCreateFlags, textureFlags, status);
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

static unsigned char *__fastcall CreateChildEmitter(unsigned char *emitterData, unsigned int flags, CStatus *status, CParticleEmitter2 *parent) {
  char *childPath = reinterpret_cast<char *>(emitterData);
  emitterData += 260;
  if (!SStrLen(childPath)) {
    return emitterData;
  }

  unsigned int   fileBytes = 0;
  unsigned char *fileData = MDLFileBinaryLoad(childPath, &fileBytes, status);
  if (!fileData) {
    return emitterData;
  }

  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x53584554);
  ASSERT(section);
  const MDLTEXTURESECTION *textures = reinterpret_cast<const MDLTEXTURESECTION *>(section + 4);

  section = MDLFileBinarySeek(fileData, fileBytes, 0x32455250);
  ASSERT(section);
  unsigned int numEmitters = *reinterpret_cast<unsigned int *>(section + 4);
  if (numEmitters > 4) {
    numEmitters = 4;
  }

  unsigned char *childData = section + 8;
  for (unsigned int i = 0; i < numEmitters; ++i) {
    const unsigned int bytesThisEmitter = *reinterpret_cast<unsigned int *>(childData);
    CParticleEmitter2 *child = CreateEmitter(childData + 4, textures, flags, status);
    parent->AddChildEmitter(child);
    child->SetEnabled(1, 1);
    childData += bytesThisEmitter;
  }
  return emitterData;
}

unsigned char *__fastcall SetParticleTumble(unsigned char *emitterData, CParticleEmitter2 *emitter) {
  const float *tumble = reinterpret_cast<const float *>(emitterData);
  float        tumbleyMin = tumble[2];
  float        tumbleyMax = tumble[3];
  float        tumblezMin = tumble[4];
  float        tumblezMax = tumble[5];
  emitter->m_tumblex.x = tumble[0];
  emitter->m_tumblex.y = tumble[1] - tumble[0];
  emitter->m_tumbley.x = tumbleyMin;
  emitter->m_tumbley.y = tumbleyMax - tumbleyMin;
  emitter->m_tumblez.x = tumblezMin;
  emitter->m_tumblez.y = tumblezMax - tumblezMin;
  return emitterData + 6 * sizeof(float);
}

static unsigned char *__fastcall LoadC3Vector(unsigned char *emitterData, NTempest::C3Vector *vector) {
  const float *values = reinterpret_cast<const float *>(emitterData);
  vector->x = values[0];
  vector->y = values[1];
  vector->z = values[2];
  return emitterData + 3 * sizeof(float);
}

CParticleEmitter2 *__fastcall CreateEmitter(unsigned char *emitterData, const MDLTEXTURESECTION *textures, unsigned int flags, CStatus *status) {
  const unsigned int emitterFlags = GetEmitterFlags(emitterData);
  unsigned char     *cursor = emitterData;
  cursor += *reinterpret_cast<unsigned int *>(cursor) + 4;
  const unsigned int emitterType = *reinterpret_cast<unsigned int *>(cursor);
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
  const unsigned int textureRows = *reinterpret_cast<unsigned int *>(cursor);
  cursor += 4;
  const unsigned int textureColumns = *reinterpret_cast<unsigned int *>(cursor);
  cursor += 4;
  emitter->SetTextureDimensions(textureRows, textureColumns);

  cursor += SetParticleStyle(cursor, emitterFlags, emitter);
  cursor = SetParticleKeys(cursor, lifeSpan, emitter);
  cursor = CreateParticleMaterial(cursor, textures, emitterFlags, flags, status, emitter);

  emitter->m_priorityPlane = *reinterpret_cast<int *>(cursor);
  cursor += 4;
  emitter->SetReplaceableId(*reinterpret_cast<unsigned int *>(cursor));
  cursor += 4;

  const char *modelPath = reinterpret_cast<const char *>(cursor);
  cursor += 260;
  if (SStrLen(modelPath)) {
    CModelCreate modelCreate;
    modelCreate.flags = flags & 0xFFFFFFB9;
    memset(&modelCreate.sequenceNames, 0, sizeof(modelCreate) - sizeof(modelCreate.flags));
    emitter->SetModel(ModelCreate(modelPath, &modelCreate, status));
  }

  cursor = CreateChildEmitter(cursor, flags, status, emitter);

  const float *twinkle = reinterpret_cast<const float *>(cursor);
  emitter->m_twinkleFPS = twinkle[0];
  emitter->m_twinkleOnOff = twinkle[1];
  if (emitter->m_twinkleOnOff == 0.0f) {
    status->Add(STATUS_WARNING, "Particle system with 0 twinkle\n");
  }
  emitter->m_twinkleScaleMin = twinkle[2];
  emitter->m_twinkleScaleMax = twinkle[3];
  emitter->m_twinkleScaleRange = twinkle[3] - twinkle[2];
  emitter->m_ivelScale = twinkle[4];
  cursor += 5 * sizeof(float);

  cursor = SetParticleTumble(cursor, emitter);
  emitter->m_drag = *reinterpret_cast<float *>(cursor);
  cursor += 4;
  emitter->m_particleAngularVelocity = *reinterpret_cast<float *>(cursor);
  cursor += 4;
  NTempest::C3Vector windVector;
  cursor = LoadC3Vector(cursor, &windVector);
  emitter->m_windVector = windVector;
  emitter->m_windTime = *reinterpret_cast<float *>(cursor);
  cursor += 4;

  const float *follow = reinterpret_cast<const float *>(cursor);
  emitter->SetFollowParams(follow[0], follow[1], follow[2], follow[3]);
  cursor += 4 * sizeof(float);

  const unsigned int numSplinePoints = *reinterpret_cast<unsigned int *>(cursor);
  cursor += 4;
  if (numSplinePoints) {
    static_cast<CSplineParticleEmitter *>(emitter)->SetSpline(reinterpret_cast<const NTempest::C3Vector *>(cursor), numSplinePoints);
  }
  return emitter;
}

int __fastcall MdlReadLoadEmitters2(const MDLDATA& data, CModelComplex* modelptr, CModelShared* shared, unsigned int flags, CStatus* status) {
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  unsigned int numEmitters = data.particleEmitters2.Count();
  modelptr->m_emitters2.SetCount(numEmitters);
  shared->emitter2Order.SetCount(numEmitters);

  const unsigned char *emitterData =
      reinterpret_cast<const unsigned char *>(
          data.particleEmitters2.Ptr());
  unsigned int i;
  for (i = 0; i < numEmitters; ++i) {
    const unsigned char *emitter = emitterData + i * 1292;
    shared->emitter2Order[i] =
        *reinterpret_cast<const unsigned int *>(emitter + 0x50);
    modelptr->m_emitters2[i] = CreateEmitter(
        const_cast<unsigned char *>(emitter),
        data.textures.Ptr(),
        flags,
        status);
  }
  return 1;
}

void __fastcall
MdxReadEmitters2(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  ASSERT(status);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x32455250);
  if (!section) {
    return;
  }

  unsigned char *texData = MDLFileBinarySeek(data, fileBytes, 0x53584554);
  ASSERT(texData);
  const MDLTEXTURESECTION *textures = reinterpret_cast<const MDLTEXTURESECTION *>(texData + 4);

  unsigned int   sectionBytes = *reinterpret_cast<unsigned int *>(section) - 4;
  unsigned int   numEmitters = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *emitterData = section + 8;

  modelptr->m_emitters2.SetCount(numEmitters);
  shared->emitter2Order.SetCount(numEmitters);
  for (unsigned int i = 0; i < numEmitters; ++i) {
    unsigned int bytesThisEmitter = *reinterpret_cast<unsigned int *>(emitterData);
    shared->emitter2Order[i] = *reinterpret_cast<unsigned int *>(emitterData + 0x58);
    modelptr->m_emitters2[i] = CreateEmitter(emitterData + 4, textures, flags, status);

    ASSERT(sectionBytes >= bytesThisEmitter);
    emitterData += bytesThisEmitter;
    sectionBytes -= bytesThisEmitter;
  }
  ASSERT(sectionBytes == 0);
}
