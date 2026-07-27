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

void TextureInitialize();
void TextureCacheFlush();
void TextureGxCacheFlush();
void TextureLogGxCache(HSLOG log);
void TextureLogTextures(HSLOG log);
void TextureCacheUpdate(unsigned long currentTime, CStatus *status);
void TextureDestroy();
const char *TextureGetFilename(HTEXTURE texture);
HTEXTURE TextureLoadImage(const char *filename);
HTEXTURE TextureAllocImage(EGxTexFormat format, unsigned int width, unsigned int height);
HTEXTURE
TextureCreate(const char *name, unsigned int width, unsigned int height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags);
HTEXTURE TextureCreate(const char *name, unsigned int width, unsigned int height, EGxTexFormat format, CGxTexFlags flags);
MipBits *TextureLoadImage(
    const char   *filename,
    unsigned int *width,
    unsigned int *height,
    unsigned int *gxTexFormat,
    int          *isOpaque,
    CStatus      *status,
    unsigned int *alphaBits
);
void TextureUnloadImage(MipBits *image);
HTEXTURE TextureCreate(const char *fileName, CGxTexFlags flags, CStatus *status, int dontCache);
HTEXTURE TextureCreate(CGxTex *gxTex);
HTEXTURE TextureCreateSolid(const NTempest::CImVector &color, CStatus *status);
CGxTex *TextureGetGxTex(HTEXTURE texture, int force, CStatus *status);
MipBits *TextureGetMips(HTEXTURE texture, int force);
int
TextureGetInfo(HTEXTURE texture, unsigned int &width, unsigned int &height, EGxTexFormat &format, int &opaque, unsigned int &alphaBits, int bForce);
TEXFILETYPE TextureDiscoverFileType(const char *path);
unsigned int TexturePickAlternateFilename(const char *path, TEXFILETYPE fileType, char *newpath, unsigned int size);
MipBits *TextureAllocMippedImg(EGxTexFormat format, unsigned int width, unsigned int height);
unsigned int TextureCalcMipCount(unsigned int width, unsigned int height);
void TextureFreeMippedImg(MipBits *image);

#endif
