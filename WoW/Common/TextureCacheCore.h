#pragma once

#include <Services/Texture.h>

DECLARE_DERIVED_HANDLE(HTEXTURECACHE, HOBJECT);
DECLARE_DERIVED_HANDLE(HMIPPEDTEXTURE, HOBJECT);

struct TEXTUREINFO {
  TEXTUREINFO() : width(0), height(0), format(GxTex_Argb8888), levels(0), opaque(1), alphaBits(0) {
  }

  unsigned int width;
  unsigned int height;
  EGxTexFormat format;
  unsigned int levels;
  int          opaque;
  unsigned int alphaBits;
};

HTEXTURECACHE TextureCacheCreateSizeCache(unsigned int cacheSize);
HTEXTURECACHE TextureCacheCreateInstanceCache(unsigned int instances);
HTEXTURECACHE TextureCacheCreatTimeCache(unsigned int milliSeconds);
HMIPPEDTEXTURE TextureCacheGetTexture(HTEXTURECACHE cache, const char *fileName, TEXTUREINFO *info);
const MipBits *TextureCacheGetImage(HMIPPEDTEXTURE texture);
int TextureCacheGetInfo(HMIPPEDTEXTURE texture, TEXTUREINFO &info, int bForce);
HMIPPEDTEXTURE TextureCacheAllocUncachedImage(EGxTexFormat format, unsigned int width, unsigned int height, TEXTUREINFO *textureInfo);
