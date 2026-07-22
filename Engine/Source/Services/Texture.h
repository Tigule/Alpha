#ifndef ENGINE_SOURCE_SERVICES_TEXTURE_H
#define ENGINE_SOURCE_SERVICES_TEXTURE_H

#include "Base/Handle.h"
#include "Gx/Gx.h"
#include "Images/dxt.h"

class CStatus;
namespace NTempest {
  class CImVector;
}

DECLARE_DERIVED_HANDLE(HTEXTURE, HOBJECT);

enum TEXFILETYPE {
  TEXFILETYPE_UNKNOWN = 0,
  TEXFILETYPE_TGA = 1,
  TEXFILETYPE_BLP = 2,
  NUM_TEXFILETYPES = 3
};

void __fastcall        TextureInitialize();
void __fastcall        TextureCacheFlush();
void __fastcall        TextureGxCacheFlush();
void __fastcall        TextureLogGxCache(HSLOG log);
void __fastcall        TextureLogTextures(HSLOG log);
void __fastcall        TextureCacheUpdate(unsigned long currentTime, CStatus *status);
void __fastcall        TextureDestroy();
const char *__fastcall TextureGetFilename(HTEXTURE texture);
HTEXTURE __fastcall    TextureLoadImage(const char *filename);
HTEXTURE __fastcall    TextureAllocImage(EGxTexFormat format, unsigned int width, unsigned int height);
HTEXTURE __fastcall
TextureCreate(const char *name, unsigned int width, unsigned int height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags);
HTEXTURE __fastcall TextureCreate(const char *name, unsigned int width, unsigned int height, EGxTexFormat format, CGxTexFlags flags);
MipBits *__fastcall TextureLoadImage(
    const char   *filename,
    unsigned int *width,
    unsigned int *height,
    unsigned int *gxTexFormat,
    int          *isOpaque,
    CStatus      *status,
    unsigned int *alphaBits
);
HTEXTURE __fastcall TextureCreate(const char *fileName, CGxTexFlags flags, CStatus *status, int dontCache);
HTEXTURE __fastcall TextureCreate(CGxTex *gxTex);
HTEXTURE __fastcall TextureCreateSolid(const NTempest::CImVector &color, CStatus *status);
CGxTex *__fastcall  TextureGetGxTex(HTEXTURE texture, int force, CStatus *status);
MipBits *__fastcall TextureGetMips(HTEXTURE texture, int force);
int __fastcall
TextureGetInfo(HTEXTURE texture, unsigned int &width, unsigned int &height, EGxTexFormat &format, int &opaque, unsigned int &alphaBits, int bForce);
TEXFILETYPE __fastcall  TextureDiscoverFileType(const char *path);
unsigned int __fastcall TexturePickAlternateFilename(const char *path, TEXFILETYPE fileType, char *newpath, unsigned int size);
MipBits *__fastcall     TextureAllocMippedImg(EGxTexFormat format, unsigned int width, unsigned int height);
unsigned int __fastcall TextureCalcMipCount(unsigned int width, unsigned int height);
void __fastcall         TextureFreeMippedImg(MipBits *image);

#endif
