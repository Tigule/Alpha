#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <BLPFile/blp.h>
#include <Base/Coordinate.h>
#include <Frame/CSimpleRender.h>
#include <FrameScript/FrameScript.h>
#include <Gx/CGxDevice.h>
#include <Gx/Gx.h>
#include <Images/tga.h>
#include <Model/IModel.h>
#include <Services/Camera.h>
#include <Services/DataMgr.h>
#include <Services/Texture.h>
#include <Tempest/c34matrix.h>

#include <stdio.h>
#include <storm.h>

void Script_SendUnitSignal(const DWORDLONG &guid, int signal);

#define PORTRAIT_SIZE_SMALL 64

static int CCommand_PLightInfo(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightEnable(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightOmni(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightDir(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightAmbColor(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightDirColor(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightAmbIntens(LPCSTR command, LPCSTR arguments);
static int CCommand_PLightDirIntens(LPCSTR command, LPCSTR arguments);

struct PortraitData {
  HTEXTURE                             texture;
  TSGrowableArray<NTempest::CImVector> pixels;
};

struct PLAYERPORTRAIT : public TSHashObject<PLAYERPORTRAIT, CHashKeyGUID> {
  UINT         dirty;
  PortraitData portrait;
};

struct UNITPORTRAIT : public TSHashObject<UNITPORTRAIT, HASHKEY_NONE> {
  PortraitData portrait;
};

struct ITEMPORTRAIT : public TSHashObject<ITEMPORTRAIT, HASHKEY_STR> {
  PortraitData portrait;
};

NODEDECL(DIRTYFACE) {
  DWORDLONG guid;
};

static NTempest::C44Matrix                       identity;
static TSFixedArray<BYTE>                        alphaMasks[2];
static TSHashTable<PLAYERPORTRAIT, CHashKeyGUID> s_playerPortraits;
static TSHashTable<UNITPORTRAIT, HASHKEY_NONE>   s_unitPortraits;
static TSHashTable<ITEMPORTRAIT, HASHKEY_STR>    s_itemPortraits;
static HASHKEY_NONE                              s_nullHashKey;
static LISTDECL(DIRTYFACE, s_dirtyFaces);
static LISTDECL(DIRTYFACE, s_freeDirtyFaces);

static const TSFixedArray<BYTE> &GetAlphaMask(UINT size) {
  CBLPFile            image;
  CTgaFile            alpha;
  UINT                stride;
  int                 loaded = 0;
  BYTE               *imageData = 0;
  TSFixedArray<BYTE> &mask = alphaMasks[size == 64];

  if (!mask.Count()) {
    mask.SetCount(size * size);

    LPCSTR alphaFile = 0;
    if (size == 128) {
      alphaFile = "Interface\\CharacterFrame\\TempPortraitAlphaMask.tga";
    } else if (size == 64) {
      alphaFile = "Interface\\CharacterFrame\\TempPortraitAlphaMaskSmall.tga";
    } else {
      ASSERT(!"Unknown portrait size");
    }

    if (alpha.Open(alphaFile)) {
      ASSERT(alpha.Width() == size);
      ASSERT(alpha.Height() == size);
      if (alpha.LoadImageData(0)) {
        memcpy(mask.Ptr(), alpha.Image(), mask.Count());
        alpha.Close();
        return mask;
      }
    }

    LPCSTR imageFile = 0;
    if (size == 128) {
      imageFile = "Interface\\CharacterFrame\\TempPortraitAlphaMask.blp";
    } else if (size == 64) {
      imageFile = "Interface\\CharacterFrame\\TempPortraitAlphaMaskSmall.blp";
    } else {
      ASSERT(!"Unknown portrait size");
    }

    if (image.Open(imageFile)) {
      ASSERT(image.Width() == size);
      ASSERT(image.Height() == size);
      if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
        for (UINT i = 0; i < mask.Count(); ++i) {
          mask[i] = imageData[4 * i];
        }
        loaded = 1;
      }
      image.Unlock(0);
      image.Close();
    }

    if (!loaded) {
      memset(mask.Ptr(), 0xFF, mask.Count());
    }
    alpha.Close();
  }

  return mask;
}

static void TextureUpdate(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = static_cast<TSGrowableArray<NTempest::CImVector> *>(userArg)->Ptr();
  }
}

static struct {
  int   enable;
  int   omni;
  float dirx;
  float diry;
  float dirz;
  float ambColorr;
  float ambColorg;
  float ambColorb;
  float dirColorr;
  float dirColorg;
  float dirColorb;
  float ambIntens;
  float dirIntens;
} s_lightInfo[2] = {
    {1, 1,  1.0f,  0.5f, 1.0f, 0.25f, 0.75f, 1.0f, 0.99f, 0.66f, 0.33f, 0.33f, 3.0f},
    {1, 1, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f, 0.0f, 0.02f, 0.53f, 0.68f,  0.0f, 0.5f}
};

static int CCommand_PLightInfo(LPCSTR command, LPCSTR arguments) {
  UINT index = SStrToInt(arguments);

  if (index > 2) {
    ConsoleWriteA("Invalid index", ERROR_COLOR);
    return 1;
  }

  LPCSTR enabled = s_lightInfo[index].enable ? "on" : "off";
  LPCSTR type = s_lightInfo[index].omni ? "omni" : "directional";
  ConsoleWriteA("Portrait light %d(%s) is %s", DEFAULT_COLOR, index, type, enabled);
  ConsoleWriteA(
      "%s: (%.2f, %.2f, %.2f)", DEFAULT_COLOR, s_lightInfo[index].omni ? "Position" : "Direction", s_lightInfo[index].dirx, s_lightInfo[index].diry,
      s_lightInfo[index].dirz
  );
  ConsoleWriteA(
      "Ambient intensity: %.2f, RGB: (%.2f, %.2f, %.2f)", DEFAULT_COLOR, s_lightInfo[index].ambIntens, s_lightInfo[index].ambColorr,
      s_lightInfo[index].ambColorg, s_lightInfo[index].ambColorb
  );
  ConsoleWriteA(
      "Directional intensity: %.2f, RGB: (%.2f, %.2f, %.2f)", DEFAULT_COLOR, s_lightInfo[index].dirIntens, s_lightInfo[index].dirColorr,
      s_lightInfo[index].dirColorg, s_lightInfo[index].dirColorb
  );
  return 1;
}

static int CCommand_PLightEnable(LPCSTR command, LPCSTR arguments) {
  UINT enable;
  UINT index;

  if (sscanf(arguments, "%d %d", &index, &enable) && index < 2) {
    s_lightInfo[index].enable = enable != 0;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightOmni(LPCSTR command, LPCSTR arguments) {
  UINT omni;
  UINT index;

  if (sscanf(arguments, "%d %d", &index, &omni) && index < 2) {
    s_lightInfo[index].omni = omni != 0;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightDir(LPCSTR command, LPCSTR arguments) {
  float z;
  float y;
  float x;
  UINT  index;

  if (sscanf(arguments, "%d %f %f %f", &index, &x, &y, &z) && index < 2) {
    s_lightInfo[index].dirx = x;
    s_lightInfo[index].diry = y;
    s_lightInfo[index].dirz = z;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightAmbColor(LPCSTR command, LPCSTR arguments) {
  float b;
  float g;
  float r;
  UINT  index;

  if (sscanf(arguments, "%d %f %f %f", &index, &r, &g, &b) && index < 2) {
    s_lightInfo[index].ambColorr = r;
    s_lightInfo[index].ambColorg = g;
    s_lightInfo[index].ambColorb = b;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightDirColor(LPCSTR command, LPCSTR arguments) {
  float b;
  float g;
  float r;
  UINT  index;

  if (sscanf(arguments, "%d %f %f %f", &index, &r, &g, &b) && index < 2) {
    s_lightInfo[index].dirColorr = r;
    s_lightInfo[index].dirColorg = g;
    s_lightInfo[index].dirColorb = b;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightAmbIntens(LPCSTR command, LPCSTR arguments) {
  float intens;
  UINT  index;

  if (sscanf(arguments, "%d %f", &index, &intens) && index < 2) {
    s_lightInfo[index].ambIntens = intens;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static int CCommand_PLightDirIntens(LPCSTR command, LPCSTR arguments) {
  float intens;
  UINT  index;

  if (sscanf(arguments, "%d %f", &index, &intens) && index < 2) {
    s_lightInfo[index].dirIntens = intens;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

void PortraitInitialize() {
  ConsoleCommandRegister("PLightInfo", CCommand_PLightInfo, DEBUG, 0);
  ConsoleCommandRegister("PLightEnable", CCommand_PLightEnable, DEBUG, 0);
  ConsoleCommandRegister("PLightOmni", CCommand_PLightOmni, DEBUG, 0);
  ConsoleCommandRegister("PLightDirPos", CCommand_PLightDir, DEBUG, 0);
  ConsoleCommandRegister("PLightAmbColor", CCommand_PLightAmbColor, DEBUG, 0);
  ConsoleCommandRegister("PLightDirColor", CCommand_PLightDirColor, DEBUG, 0);
  ConsoleCommandRegister("PLightAmbIntens", CCommand_PLightAmbIntens, DEBUG, 0);
  ConsoleCommandRegister("PLightDirIntens", CCommand_PLightDirIntens, DEBUG, 0);
}

void PortraitShutdown() {
  s_playerPortraits.Clear();
  s_unitPortraits.Clear();
  s_itemPortraits.Clear();
  alphaMasks[0].Clear();
  alphaMasks[1].Clear();
  while (DIRTYFACE *dirty = s_dirtyFaces.Head()) {
    s_dirtyFaces.UnlinkNode(dirty);
    DEL(dirty);
  }
  while (DIRTYFACE *dirty = s_freeDirtyFaces.Head()) {
    s_freeDirtyFaces.UnlinkNode(dirty);
    DEL(dirty);
  }
}

void UpdatePortraits() {
  while (DIRTYFACE *dirty = s_dirtyFaces.Head()) {
    DWORDLONG       guid = dirty->guid;
    CHashKeyGUID    hashkey(guid);
    UINT            hashval = static_cast<UINT>(guid);
    PLAYERPORTRAIT *portrait = s_playerPortraits.Ptr(hashval, hashkey);
    if (portrait) {
      portrait->dirty = 1;
    }
    Script_SendUnitSignal(guid, 181);
    s_dirtyFaces.UnlinkNode(dirty);
    s_freeDirtyFaces.LinkNode(dirty, LIST_TAIL, 0);
  }
}

void UpdatePortraitTexture(const DWORDLONG &guid) {
  DIRTYFACE *dirty;
  ITERATELIST(DIRTYFACE, s_dirtyFaces, existingDirty) {
    if (existingDirty->guid == guid) {
      return;
    }
  }

  dirty = s_freeDirtyFaces.Head();
  if (dirty) {
    s_freeDirtyFaces.UnlinkNode(dirty);
  } else {
    dirty = NEW(DIRTYFACE);
  }
  s_dirtyFaces.LinkNode(dirty, LIST_TAIL, 0);
  dirty->guid = guid;
}

void SetPortraitTexture(CSimpleTexture *texture, UINT race, UINT sex, DWORDLONG guid) {
  char buf[64];

  if (!texture) {
    return;
  }

  if (guid) {
    CHashKeyGUID    hashkey(guid);
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(guid), hashkey);
    if (playerPortrait && playerPortrait->portrait.texture) {
      texture->SetTexture(playerPortrait->portrait.texture);
      return;
    }
  }

  ASSERT(race != 0);
  ASSERT(race <= static_cast<UINT>(g_chrRacesDB.GetMaxID()));
  ASSERT(sex < 3);

  SStrPrintf(
      buf, sizeof(buf), "Interface\\CharacterFrame\\TemporaryPortrait-%s-%s", sex ? "Female" : "Male",
      g_chrRacesDB.GetRecord(race)->m_clientFileString
  );
  texture->SetTexture(buf, 0);
}

void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit) {
  PortraitData       *portrait;
  NTempest::CRect     screenRect;
  NTempest::CRect     viewRect;
  NTempest::CRect     projectionRect;
  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  NTempest::CiRect    pixRect;
  NTempest::C3Vector  eye;
  NTempest::C3Vector  center;
  HMODEL              model = 0;
  HCAMERA             camera = 0;
  int                 gxFogEnable;

  if (!texture || !unit) {
    return;
  }

  if (unit->GetType() & TYPE_PLAYER) {
    CHashKeyGUID    hashkey(unit->GetGUID());
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(unit->GetGUID()), hashkey);
    if (playerPortrait) {
      if (playerPortrait->dirty) {
        portrait = &playerPortrait->portrait;
      } else {
        texture->SetTexture(playerPortrait->portrait.texture);
        return;
      }
    }
  } else {
    UNITPORTRAIT *unitPortrait = s_unitPortraits.Ptr(unit->GetUnitData()->displayID, s_nullHashKey);
    if (unitPortrait) {
      texture->SetTexture(unitPortrait->portrait.texture);
      return;
    }
  }

  GxCapsScreenSize(screenRect);
  viewRect.l = 0.0f;
  viewRect.b = 0.6f;
  viewRect.t = 0.6f - 38.400002f / screenRect.Height();
  viewRect.r = 51.200001f / screenRect.Width();

  if (!unit->IsObjectModelLoaded() || !(unit->m_flags & 0x100)) {
  portrait_fallback:
    if (unit->GetType() & TYPE_PLAYER) {
      SetPortraitTexture(texture, unit->GetUnitData()->race, unit->GetUnitData()->sex, unit->GetGUID());
    } else {
      texture->SetTexture("Interface\\CharacterFrame\\TempPortrait", 0);
    }
    if (model) {
      HandleClose(model);
    }
    return;
  }

  model = unit->DuplicateCharacterModel(0);
  camera = ModelGetCamera(model, 0);
  if (!camera) {
    goto portrait_fallback;
  }

  ClearSpecialEffects(model);
  ModelSetEmissiveColor(model, NTempest::CImVector(0ul), 1);

  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  float minX;
  float maxX;
  float minY;
  float maxY;
  float minZ;
  float maxZ;
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
  gxFogEnable = GxMasterEnable(GxMasterEnable_Fog);
  GxMasterEnableSet(GxMasterEnable_Fog, 0);

  float aspectRatio = viewRect.Width() / viewRect.Height();
  DDCToNDC(viewRect.l, viewRect.t, &viewRect.l, &viewRect.t);
  DDCToNDC(viewRect.r, viewRect.b, &viewRect.r, &viewRect.b);

  ModelSetSequence(model, 0, 13);
  ModelResetGlobalSequenceTimes(model, 0);
  ModelAnimateCameras(
      model, NTempest::C34Matrix(
                 identity.a0, identity.a1, identity.a2, identity.b0, identity.b1, identity.b2, identity.c0, identity.c1, identity.c2, identity.d0,
                 identity.d1, identity.d2
             )
  );
  DataMgrGetCoord(camera, 7, &eye);
  DataMgrGetCoord(camera, 8, &center);

  projectionRect.Set(0.0f, 0.0f, 1.0f, aspectRatio);
  CameraSetupWorldProjection(camera, projectionRect, 0);
  GxXformSetViewport(viewRect.l, viewRect.r, viewRect.t, viewRect.b, 0.0f, 1.0f);
  GxSceneClear(3);

  for (UINT lightIndex = 0; lightIndex < 2; ++lightIndex) {
    CGxLight light;
    light.m_enabled = s_lightInfo[lightIndex].enable;
    light.m_isOmni = s_lightInfo[lightIndex].omni;
    light.m_dir = NTempest::C3Vector(s_lightInfo[lightIndex].dirx, s_lightInfo[lightIndex].diry, s_lightInfo[lightIndex].dirz);
    light.m_ambColor.Set(1.0f, s_lightInfo[lightIndex].ambColorr, s_lightInfo[lightIndex].ambColorg, s_lightInfo[lightIndex].ambColorb);
    light.m_dirColor.Set(1.0f, s_lightInfo[lightIndex].dirColorr, s_lightInfo[lightIndex].dirColorg, s_lightInfo[lightIndex].dirColorb);
    light.m_specColor.Set(0.0f, 0.0f, 0.0f, 0.0f);
    light.m_ambIntensity = s_lightInfo[lightIndex].ambIntens;
    light.m_dirIntensity = s_lightInfo[lightIndex].dirIntens;
    GxLightSet(lightIndex, light, NTempest::C3Vector());
  }

  CGxLight noLight;
  noLight.m_enabled = 0;
  for (UINT disabledLightIndex = 2; disabledLightIndex < Gx_MaxLights; ++disabledLightIndex) {
    GxLightSet(disabledLightIndex, noLight, NTempest::C3Vector());
  }

  ModelSetLightSelectCallback(model, 0, 0, 1);
  NTempest::C3Vector cameraVector = center - eye;
  ModelAnimate(model, NTempest::C3Vector(), 0.0f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1.0f, eye, cameraVector);
  ModelRender(model, 0, 0);

  if (unit->GetType() & TYPE_PLAYER) {
    CHashKeyGUID    hashkey(unit->GetGUID());
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(unit->GetGUID()), hashkey);
    if (!playerPortrait) {
      playerPortrait = s_playerPortraits.New(static_cast<UINT>(unit->GetGUID()), hashkey, 0, 0);
    }
    playerPortrait->dirty = 0;
    portrait = &playerPortrait->portrait;
  } else {
    UNITPORTRAIT *unitPortrait = s_unitPortraits.New(unit->GetUnitData()->displayID, s_nullHashKey, 0, 0);
    portrait = &unitPortrait->portrait;
  }

  pixRect = NTempest::CiRect(0, 0, 64, 64);
  GxDevReadPixels(pixRect, portrait->pixels);
  GxSceneClear(3);

  const TSFixedArray<BYTE> &alphaMask = GetAlphaMask(64);
  ASSERT(portrait->pixels.Count() == alphaMask.Count());
  for (UINT i = 0; i < portrait->pixels.Count(); ++i) {
    portrait->pixels[i].a = alphaMask[i];
  }

  if (portrait->texture) {
    CGxTex *gxTex = TextureGetGxTex(portrait->texture, 1, 0);
    GxTexUpdate(gxTex, pixRect, 1);
  } else {
    CGxTex     *gxTex;
    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    GxTexCreate(64, 64, GxTex_Argb8888, flags, &portrait->pixels, TextureUpdate, gxTex);
    portrait->texture = TextureCreate(gxTex);
  }

  texture->SetTexture(portrait->texture);
  GxXformSetProjection(saved_proj);
  GxXformSetView(saved_view);
  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
  GxMasterEnableSet(GxMasterEnable_Fog, gxFogEnable);
  HandleClose(model);
  HandleClose(camera);
}

void SetPortraitTexture(CSimpleTexture *texture, LPCSTR textureFile) {
  if (!textureFile || !*textureFile) {
    return;
  }

  ITEMPORTRAIT *itemPortrait = s_itemPortraits.Ptr(textureFile);
  if (itemPortrait) {
    texture->SetTexture(itemPortrait->portrait.texture);
    return;
  }

  itemPortrait = s_itemPortraits.New(textureFile, 0, 0);

  CBLPFile texFile;
  if (texFile.Open(textureFile)) {
    ASSERT(texFile.Width() == PORTRAIT_SIZE_SMALL);
    ASSERT(texFile.Height() == PORTRAIT_SIZE_SMALL);

    TSGrowableArray<NTempest::CImVector> &pixels = itemPortrait->portrait.pixels;
    if (!pixels.Count()) {
      pixels.SetCount(PORTRAIT_SIZE_SMALL * PORTRAIT_SIZE_SMALL);
    }

    BYTE *texData;
    UINT  stride;
    if (texFile.Lock(PIXEL_ARGB8888, 0, texData, stride)) {
      for (UINT i = 0; i < pixels.Count(); ++i) {
        pixels[i] = NTempest::CImVector(texData[4 * i + 3], texData[4 * i + 2], texData[4 * i + 1], texData[4 * i]);
      }
    }
    texFile.Unlock(0);
    texFile.Close();

    const TSFixedArray<BYTE> &alphaMask = GetAlphaMask(PORTRAIT_SIZE_SMALL);
    ASSERT(pixels.Count() == alphaMask.Count());
    for (UINT i = 0; i < pixels.Count(); ++i) {
      pixels[i].a = alphaMask[i];
    }

    CGxTex     *gxTex;
    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    GxTexCreate(PORTRAIT_SIZE_SMALL, PORTRAIT_SIZE_SMALL, GxTex_Argb8888, flags, &pixels, TextureUpdate, gxTex);
    HTEXTURE portraitTexture = TextureCreate(gxTex);
    texture->SetTexture(portraitTexture);
    itemPortrait->portrait.texture = portraitTexture;
  }
}
