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

void     TextureInitialize();
void     TextureCacheFlush();
void     TextureGxCacheFlush();
void     TextureLogGxCache(HSLOG log);
void     TextureLogTextures(HSLOG log);
void     TextureCacheUpdate(DWORD currentTime, CStatus *status);
void     TextureDestroy();
LPCSTR   TextureGetFilename(HTEXTURE texture);
HTEXTURE TextureLoadImage(LPCSTR filename);
HTEXTURE TextureAllocImage(EGxTexFormat format, UINT width, UINT height);
HTEXTURE
TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, EGxTexFormat dataFormat, CGxTexFlags flags);
HTEXTURE    TextureCreate(LPCSTR name, UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags);
MipBits    *TextureLoadImage(LPCSTR filename, UINT *width, UINT *height, UINT *gxTexFormat, int *isOpaque, CStatus *status, UINT *alphaBits);
MipBits    *TextureLoadImage(HTEXTURE texture, UINT *width, UINT *height, UINT *gxTexFormat, CStatus *status, UINT *alphaBits);
void        TextureUnloadImage(MipBits *image);
HTEXTURE    TextureCreate(UINT width, UINT height, EGxTexFormat format, CGxTexFlags flags);
HTEXTURE    TextureCreate(LPCSTR fileName, CGxTexFlags flags, CStatus *status, int dontCache);
HTEXTURE    TextureCreate(CGxTex *gxTex);
HTEXTURE    TextureCreateSolid(const NTempest::CImVector &color, CStatus *status);
CGxTex     *TextureGetGxTex(HTEXTURE texture, int force, CStatus *status);
MipBits    *TextureGetMips(HTEXTURE texture, int force);
int         TextureGetInfo(HTEXTURE texture, UINT &width, UINT &height, EGxTexFormat &format, int &opaque, UINT &alphaBits, int bForce);
TEXFILETYPE TextureDiscoverFileType(LPCSTR path);
UINT        TexturePickAlternateFilename(LPCSTR path, TEXFILETYPE fileType, char *newpath, UINT size);
MipBits    *TextureAllocMippedImg(EGxTexFormat format, UINT width, UINT height);
UINT        TextureCalcMipCount(UINT width, UINT height);
void        TextureFreeMippedImg(MipBits *image);

#endif
