#pragma once

#include <Services/Texture.h>

DECLARE_DERIVED_HANDLE(HTEXTURECACHE, HOBJECT);
DECLARE_DERIVED_HANDLE(HMIPPEDTEXTURE, HOBJECT);

struct TEXTUREINFO {
  TEXTUREINFO() : width(0), height(0), format(GxTex_Argb8888), levels(0), opaque(1), alphaBits(0) {
  }

  UINT         width;
  UINT         height;
  EGxTexFormat format;
  UINT         levels;
  int          opaque;
  UINT         alphaBits;
};

HTEXTURECACHE  TextureCacheCreateSizeCache(UINT cacheSize);
HTEXTURECACHE  TextureCacheCreateInstanceCache(UINT instances);
HTEXTURECACHE  TextureCacheCreatTimeCache(UINT milliSeconds);
HMIPPEDTEXTURE TextureCacheGetTexture(HTEXTURECACHE cache, LPCSTR fileName, TEXTUREINFO *info);
const MipBits *TextureCacheGetImage(HMIPPEDTEXTURE texture);
BOOL           TextureCacheGetInfo(HMIPPEDTEXTURE texture, TEXTUREINFO &info, BOOL bForce);
HMIPPEDTEXTURE TextureCacheAllocUncachedImage(EGxTexFormat format, UINT width, UINT height, TEXTUREINFO *textureInfo);
