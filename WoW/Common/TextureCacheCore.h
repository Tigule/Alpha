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

HTEXTURECACHE __fastcall  TextureCacheCreateSizeCache(unsigned int cacheSize);
HTEXTURECACHE __fastcall  TextureCacheCreateInstanceCache(unsigned int instances);
HTEXTURECACHE __fastcall  TextureCacheCreatTimeCache(unsigned int milliSeconds);
HMIPPEDTEXTURE __fastcall TextureCacheGetTexture(HTEXTURECACHE cache, const char *fileName, TEXTUREINFO *info);
const MipBits *__fastcall TextureCacheGetImage(HMIPPEDTEXTURE texture);
int __fastcall            TextureCacheGetInfo(HMIPPEDTEXTURE texture, TEXTUREINFO &info, int bForce);
HMIPPEDTEXTURE __fastcall TextureCacheAllocUncachedImage(EGxTexFormat format, unsigned int width, unsigned int height, TEXTUREINFO *textureInfo);
