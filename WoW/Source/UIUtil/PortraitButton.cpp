#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Ui/GameUI.h"

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include <BLPFile/blp.h>
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <Base/Coordinate.h>
#include <Frame/CSimpleRender.h>
#include <FrameScript/FrameScript.h>
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
void SetPortraitTexture(CSimpleTexture *texture, UINT race, UINT sex, DWORDLONG guid);

#define PORTRAIT_SIZE_SMALL 64

static BOOL CCommand_PLightInfo(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightEnable(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightOmni(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightDir(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightAmbColor(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightDirColor(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightAmbIntens(LPCSTR command, LPCSTR arguments);
static BOOL CCommand_PLightDirIntens(LPCSTR command, LPCSTR arguments);

struct PortraitData {
  PortraitData() : texture(0) {
  }

  ~PortraitData() {
    if (texture) {
      HandleClose(texture);
    }
  }

  HTEXTURE                             texture;
  TSGrowableArray<NTempest::CImVector> pixels;
};

struct PLAYERPORTRAIT : public TSHashObject<PLAYERPORTRAIT, CHashKeyGUID> {
  PLAYERPORTRAIT() : dirty(1) {
  }

  BYTE         dirty;
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

LISTDECL(DIRTYFACE, s_dirtyList);
LISTDECL(DIRTYFACE, s_freeList);
static TSHashTable<PLAYERPORTRAIT, CHashKeyGUID> s_playerPortraits;
static TSHashTable<UNITPORTRAIT, HASHKEY_NONE>   s_unitPortraits;
static TSHashTable<ITEMPORTRAIT, HASHKEY_STR>    s_itemPortraits;

#include "Object/ObjectClient/Unit_C.h"

static const TSFixedArray<BYTE> &GetAlphaMask(UINT size) {
  static TSFixedArray<BYTE> alphaMasks[2];
  TSFixedArray<BYTE> &mask = alphaMasks[size == 64];

  if (!mask.Count()) {
    mask.SetCount(size * size);

    int      loaded = 0;
    CTgaFile alpha;
    LPCSTR   alphaFile = 0;
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
        loaded = 1;
      }
    }

    if (!loaded) {
      CBLPFile image;
      LPCSTR   imageFile = 0;
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
        BYTE *imageData;
        UINT  stride;
        if (image.Lock(PIXEL_ARGB8888, 0, imageData, stride)) {
          for (UINT i = 0; i < mask.Count(); ++i) {
            mask[i] = imageData[4 * i];
          }
          loaded = 1;
        }
        image.Unlock(0);
        image.Close();
      }
    }

    if (!loaded) {
      memset(mask.Ptr(), mask.Count(), 0xFF);
    }
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

static BOOL CCommand_PLightInfo(LPCSTR command, LPCSTR arguments) {
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

static BOOL CCommand_PLightEnable(LPCSTR command, LPCSTR arguments) {
  UINT enable;
  UINT index;

  if (sscanf(arguments, "%d %d", &index, &enable) && index < 2) {
    s_lightInfo[index].enable = enable != 0;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static BOOL CCommand_PLightOmni(LPCSTR command, LPCSTR arguments) {
  UINT omni;
  UINT index;

  if (sscanf(arguments, "%d %d", &index, &omni) && index < 2) {
    s_lightInfo[index].omni = omni != 0;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static BOOL CCommand_PLightDir(LPCSTR command, LPCSTR arguments) {
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

static BOOL CCommand_PLightAmbColor(LPCSTR command, LPCSTR arguments) {
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

static BOOL CCommand_PLightDirColor(LPCSTR command, LPCSTR arguments) {
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

static BOOL CCommand_PLightAmbIntens(LPCSTR command, LPCSTR arguments) {
  float intens;
  UINT  index;

  if (sscanf(arguments, "%d %f", &index, &intens) && index < 2) {
    s_lightInfo[index].ambIntens = intens;
    return 1;
  }

  ConsoleWriteA("Invalid syntax", ERROR_COLOR);
  return 1;
}

static BOOL CCommand_PLightDirIntens(LPCSTR command, LPCSTR arguments) {
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
  s_dirtyList.Clear();
  s_freeList.Clear();
}

void UpdatePortraits() {
  while (DIRTYFACE *dirty = s_dirtyList.Head()) {
    DWORDLONG       guid = dirty->guid;
    UINT            hashval = static_cast<UINT>(guid);
    CHashKeyGUID    hashkey(guid);
    PLAYERPORTRAIT *portrait = s_playerPortraits.Ptr(hashval, hashkey);
    if (portrait) {
      portrait->dirty = 1;
    }
    Script_SendUnitSignal(guid, 181);
    s_freeList.LinkNode(dirty, LIST_TAIL, 0);
  }
}

void UpdatePortraitTexture(const DWORDLONG &guid) {
  DIRTYFACE *dirty;
  ITERATELIST(DIRTYFACE, s_dirtyList, existingDirty) {
    if (existingDirty->guid == guid) {
      return;
    }
  }

  dirty = s_freeList.Head();
  if (dirty) {
    s_dirtyList.LinkNode(dirty, LIST_TAIL, 0);
  } else {
    dirty = s_dirtyList.NewNode(LIST_TAIL, 0, 0);
  }
  dirty->guid = guid;
}

void SetPortraitTexture(CSimpleTexture *texture, const CGUnit_C *unit) {
  if (!texture || !unit) {
    return;
  }

  if (unit->IsA(ID_PLAYER)) {
    CHashKeyGUID    hashkey(unit->GetGUID());
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(unit->GetGUID()), hashkey);
    if (playerPortrait && !playerPortrait->dirty) {
      texture->SetTexture(playerPortrait->portrait.texture);
      return;
    }
  } else {
    UNITPORTRAIT *unitPortrait = s_unitPortraits.Ptr(unit->GetDisplayID(), HASHKEY_NONE());
    if (unitPortrait) {
      texture->SetTexture(unitPortrait->portrait.texture);
      return;
    }
  }

  NTempest::CRect screenRect;
  GxCapsScreenSize(screenRect);
  NTempest::CRect viewRect;
  viewRect.l = 0.0f;
  viewRect.b = 0.6f;
  viewRect.t = 0.6f - 38.400002f / screenRect.Height();
  viewRect.r = 51.200001f / screenRect.Width();

  HMODEL  model = 0;
  HCAMERA camera;
  if (!unit->IsObjectModelLoaded() || !(unit->m_flags & 0x100) || (model = unit->DuplicateCharacterModel(0), (camera = ModelGetCamera(model, 0)) == 0)) {
    if (unit->IsA(ID_PLAYER)) {
      SetPortraitTexture(texture, unit->GetRace(), unit->GetSex(), unit->GetGUID());
    } else {
      texture->SetTexture("Interface\\CharacterFrame\\TempPortrait", 0);
    }
    if (model) {
      HandleClose(model);
    }
    return;
  }

  ClearSpecialEffects(model);
  ModelSetEmissiveColor(model, NTempest::CImVector(0ul), 1);

  NTempest::C44Matrix saved_proj;
  NTempest::C44Matrix saved_view;
  GxXformProjection(saved_proj);
  GxXformView(saved_view);
  float minX;
  float maxX;
  float minY;
  float maxY;
  float minZ;
  float maxZ;
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
  int gxFogEnable = GxMasterEnable(GxMasterEnable_Fog);
  GxMasterEnableSet(GxMasterEnable_Fog, 0);

  float aspectRatio = viewRect.Width() / viewRect.Height();
  DDCToNDC(viewRect.l, viewRect.t, &viewRect.l, &viewRect.t);
  DDCToNDC(viewRect.r, viewRect.b, &viewRect.r, &viewRect.b);

  static NTempest::C44Matrix identity;
  NTempest::C3Vector         eye;
  NTempest::C3Vector         center;
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

  NTempest::CRect projectionRect(0.0f, 0.0f, 1.0f, aspectRatio);
  CameraSetupWorldProjection(camera, projectionRect, 0);
  GxXformSetViewport(viewRect.l, viewRect.r, viewRect.t, viewRect.b, 0.0f, 1.0f);
  GxSceneClear(3);

  CGxLight light;
  light.m_enabled = s_lightInfo[0].enable;
  light.m_isOmni = s_lightInfo[0].omni;
  light.m_dir = NTempest::C3Vector(s_lightInfo[0].dirx, s_lightInfo[0].diry, s_lightInfo[0].dirz);
  light.m_ambColor.Set(1.0f, s_lightInfo[0].ambColorr, s_lightInfo[0].ambColorg, s_lightInfo[0].ambColorb);
  light.m_dirColor.Set(1.0f, s_lightInfo[0].dirColorr, s_lightInfo[0].dirColorg, s_lightInfo[0].dirColorb);
  light.m_ambIntensity = s_lightInfo[0].ambIntens;
  light.m_dirIntensity = s_lightInfo[0].dirIntens;
  GxLightSet(0, light, NTempest::C3Vector());

  light.m_enabled = s_lightInfo[1].enable;
  light.m_isOmni = s_lightInfo[1].omni;
  light.m_dir = NTempest::C3Vector(s_lightInfo[1].dirx, s_lightInfo[1].diry, s_lightInfo[1].dirz);
  light.m_ambColor.Set(1.0f, s_lightInfo[1].ambColorr, s_lightInfo[1].ambColorg, s_lightInfo[1].ambColorb);
  light.m_dirColor.Set(1.0f, s_lightInfo[1].dirColorr, s_lightInfo[1].dirColorg, s_lightInfo[1].dirColorb);
  light.m_ambIntensity = s_lightInfo[1].ambIntens;
  light.m_dirIntensity = s_lightInfo[1].dirIntens;
  GxLightSet(1, light, NTempest::C3Vector());

  CGxLight noLight;
  noLight.m_enabled = 0;
  for (UINT lightIndex = 2; lightIndex < Gx_MaxLights; ++lightIndex) {
    GxLightSet(lightIndex, noLight, NTempest::C3Vector());
  }

  ModelSetLightSelectCallback(model, 0, 0, 1);
  ModelAnimate(model, NTempest::C3Vector(), 0.0f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1.0f, eye, center - eye);
  ModelRender(model, 0, 0);

  PortraitData *portrait;
  if (unit->IsA(ID_PLAYER)) {
    CHashKeyGUID    hashkey(unit->GetGUID());
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(unit->GetGUID()), hashkey);
    if (playerPortrait) {
      playerPortrait->dirty = 0;
      portrait = &playerPortrait->portrait;
    } else {
      portrait = &s_playerPortraits.New(static_cast<UINT>(unit->GetGUID()), hashkey, 0, 0)->portrait;
    }
  } else {
    portrait = &s_unitPortraits.New(unit->GetDisplayID(), HASHKEY_NONE(), 0, 0)->portrait;
  }

  TSGrowableArray<NTempest::CImVector> &pixels = portrait->pixels;
  NTempest::CiRect                      pixRect(0, 0, 64, 64);
  GxDevReadPixels(pixRect, pixels);
  GxSceneClear(3);

  const TSFixedArray<BYTE> &alphaMask = GetAlphaMask(64);
  ASSERT(pixels.Count() == alphaMask.Count());
  for (UINT i = 0; i < pixels.Count(); ++i) {
    pixels[i].a = alphaMask[i];
  }

  if (portrait->texture) {
    GxTexUpdate(TextureGetGxTex(portrait->texture, 1, 0), pixRect, 1);
  } else {
    CGxTex *gxTex;
    GxTexCreate(64, 64, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, TextureUpdate, gxTex);
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

void SetPortraitTexture(CSimpleTexture *texture, UINT race, UINT sex, DWORDLONG guid) {
  if (guid) {
    PLAYERPORTRAIT *playerPortrait = s_playerPortraits.Ptr(static_cast<UINT>(guid), guid);
    if (playerPortrait && playerPortrait->portrait.texture) {
      texture->SetTexture(playerPortrait->portrait.texture);
      return;
    }
  }

  ASSERT(race != 0);
  ASSERT(race <= (uint)g_chrRacesDB.GetMaxID());
  ASSERT(sex < UNITSEX_LAST);

  char               buf[64];
  const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(race);
  SStrPrintf(
      buf, sizeof(buf), "Interface\\CharacterFrame\\TemporaryPortrait-%s-%s", sex == UNITSEX_MALE ? "Male" : "Female", raceRec->m_clientFileString
  );
  texture->SetTexture(buf, 0);
}

void SetPortraitTexture(CSimpleTexture *texture, LPCSTR textureFile) {
  if (!textureFile || !*textureFile) {
    return;
  }

  ITEMPORTRAIT *hash = s_itemPortraits.Ptr(textureFile);
  if (hash) {
    texture->SetTexture(hash->portrait.texture);
    return;
  }

  hash = s_itemPortraits.New(textureFile, 0, 0);

  CBLPFile texFile;
  if (texFile.Open(textureFile)) {
    ASSERT(texFile.Width() == PORTRAIT_SIZE_SMALL);
    ASSERT(texFile.Height() == PORTRAIT_SIZE_SMALL);

    TSGrowableArray<NTempest::CImVector> &pixels = hash->portrait.pixels;
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

    CGxTex *gxTex;
    GxTexCreate(PORTRAIT_SIZE_SMALL, PORTRAIT_SIZE_SMALL, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &pixels, TextureUpdate, gxTex);
    HTEXTURE portraitTexture = TextureCreate(gxTex);
    texture->SetTexture(portraitTexture);
    hash->portrait.texture = portraitTexture;
  }
}
