#include "TextureCacheCore.h"

#include <Base/Status.h>
#include <Os/OsTime.h>

#include <new>
#include <stpl.h>

enum CACHEMODE {
  CACHEMODE_NONE = 0,
  CACHEMODE_SIZE = 1,
  CACHEMODE_ENTRIES = 2,
  CACHEMODE_TIME = 3,
  NUM_CACHEMODES = 4
};

struct CACHEENTRY : public TSHashObject<CACHEENTRY, HASHKEY_STRI>, public CHandleObject {
  CACHEENTRY() : m_texture(0), m_size(0), m_expireTime(0), m_uncached(0) {
  }

  virtual ~CACHEENTRY() {
    if (m_texture) {
      HandleClose(m_texture);
    }

    m_texture = 0;
  }

  BOOL IsInUse() const {
    return GetRefCount() > 1;
  }

  void LoadData(LPCSTR fileName) {
    char    altFileName[0x104];
    CStatus status;

    m_texture = TextureLoadImage(fileName);
    if (!m_texture) {
      TEXFILETYPE fileType = TextureDiscoverFileType(fileName);

      TexturePickAlternateFilename(fileName, fileType, altFileName, sizeof(altFileName));
      m_texture = TextureLoadImage(altFileName);
    }
  }

  LINKDECLEX(CACHEENTRY, m_cacheLink);
  HTEXTURE       m_texture;
  TEXTUREINFO    m_textureInfo;
  UINT           m_size;
  UINT           m_expireTime;
  int            m_uncached;
  HMIPPEDTEXTURE m_selfReference;
};

class CACHEOBJECT : public CHandleObject {
  friend HTEXTURECACHE TextureCacheCreateSizeCache(UINT cacheSize);
  friend HTEXTURECACHE TextureCacheCreateInstanceCache(UINT instances);
  friend HTEXTURECACHE TextureCacheCreatTimeCache(UINT milliSeconds);

 public:
  CACHEOBJECT();
  virtual ~CACHEOBJECT();
  void           PurgeTextureCache();
  HMIPPEDTEXTURE GetTexture(LPCSTR fileName, TEXTUREINFO *info);

 protected:
  LISTDECLEX(CACHEENTRY, m_cacheLink, m_LRUList);
  TSHashTable<CACHEENTRY, HASHKEY_STRI> m_cacheTable;
  UINT                                  m_cacheSize;
  UINT                                  m_currentCacheSize;
  CACHEMODE                             m_cacheMode;
  UINT                                  m_cacheEntries;
  UINT                                  m_cacheTime;
};

void CACHEOBJECT::PurgeTextureCache() {
  CACHEENTRY *curr;
  CACHEENTRY *next;

  switch (m_cacheMode) {
    case CACHEMODE_SIZE:
      curr = m_LRUList.Tail();
      while (curr && m_currentCacheSize > m_cacheSize) {
        next = m_LRUList.Prev(curr);

        if (!curr->IsInUse()) {
          m_currentCacheSize -= curr->m_size;
          ASSERT(curr->m_selfReference);
          HandleClose(curr->m_selfReference);
        }

        curr = next;
      }
      break;

    case CACHEMODE_ENTRIES: {
      UINT entriesFound = 0;

      curr = m_LRUList.Head();
      while (curr && m_currentCacheSize > m_cacheSize) {
        next = m_LRUList.Next(curr);

        if (!curr->IsInUse()) {
          if (entriesFound < m_cacheEntries) {
            ++entriesFound;
          } else {
            m_currentCacheSize -= curr->m_size;
            ASSERT(curr->m_selfReference);
            HandleClose(curr->m_selfReference);
          }
        }

        curr = next;
      }
      break;
    }

    case CACHEMODE_TIME: {
      UINT currentTime;

      curr = m_LRUList.Head();
      currentTime = OsGetAsyncTimeMs();
      while (curr && m_currentCacheSize > m_cacheSize) {
        next = m_LRUList.Next(curr);

        if (!curr->IsInUse() && currentTime >= curr->m_expireTime) {
          m_currentCacheSize -= curr->m_size;
          ASSERT(curr->m_selfReference);
          HandleClose(curr->m_selfReference);
        }

        curr = next;
      }
      break;
    }

    default:
      ASSERT(!"Error, unrecognized cache mode!");
      break;
  }
}

CACHEOBJECT::CACHEOBJECT() {
  m_currentCacheSize = 0;
  m_cacheMode = CACHEMODE_NONE;
  m_cacheSize = 0x00100000;
}

CACHEOBJECT::~CACHEOBJECT() {
  CACHEENTRY *curr = m_cacheTable.Head();

  while (curr) {
    CACHEENTRY *next = m_cacheTable.Next(curr);

    HandleClose(curr->m_selfReference);
    curr = next;
  }
}

HMIPPEDTEXTURE CACHEOBJECT::GetTexture(LPCSTR fileName, TEXTUREINFO *info) {
  CACHEENTRY *object;

  ASSERT(fileName);
  ASSERT(info);

  object = m_cacheTable.Ptr(fileName);
  if (object) {
    m_LRUList.LinkNode(object, LIST_HEAD, 0);
    object->m_expireTime = OsGetAsyncTimeMs() + m_cacheTime;
    *info = object->m_textureInfo;
    return reinterpret_cast<HMIPPEDTEXTURE>(HandleCreate(object, "HMIPPEDTEXTURE"));
  }

  object = m_cacheTable.New(fileName, 0, 0);

  object->LoadData(fileName);

  if (!object->m_texture) {
    m_cacheTable.Delete(fileName);
    return 0;
  }

  m_LRUList.LinkNode(object, LIST_HEAD, 0);
  object->m_expireTime = OsGetAsyncTimeMs() + m_cacheTime;
  object->m_size = 0x7FFF;
  m_currentCacheSize += object->m_size;
  object->m_selfReference = reinterpret_cast<HMIPPEDTEXTURE>(HandleCreate(object, "HMIPPEDTEXTURE"));
  ASSERT(object->m_selfReference);
  *info = object->m_textureInfo;
  return reinterpret_cast<HMIPPEDTEXTURE>(HandleDuplicate(object->m_selfReference));
}

HTEXTURECACHE TextureCacheCreateSizeCache(UINT cacheSize) {
  LPVOID       storage;
  CACHEOBJECT *cacheObject;

  if (!cacheSize) {
    return 0;
  }

  storage = SMemAlloc(sizeof(CACHEOBJECT), "HTEXTURECACHE", SERR_LINECODE_OBJECT, 0);
  cacheObject = storage ? new (storage) CACHEOBJECT : 0;
  if (!cacheObject) {
    return 0;
  }

  cacheObject->m_cacheSize = cacheSize;
  cacheObject->m_cacheMode = CACHEMODE_SIZE;
  return reinterpret_cast<HTEXTURECACHE>(HandleCreate(cacheObject, "HTEXTURECACHE"));
}

HTEXTURECACHE TextureCacheCreateInstanceCache(UINT instances) {
  LPVOID       storage;
  CACHEOBJECT *cacheObject;

  if (!instances) {
    return 0;
  }

  storage = SMemAlloc(sizeof(CACHEOBJECT), "HTEXTURECACHE", SERR_LINECODE_OBJECT, 0);
  cacheObject = storage ? new (storage) CACHEOBJECT : 0;
  if (!cacheObject) {
    return 0;
  }

  cacheObject->m_cacheEntries = instances;
  cacheObject->m_cacheMode = CACHEMODE_ENTRIES;
  return reinterpret_cast<HTEXTURECACHE>(HandleCreate(cacheObject, "HTEXTURECACHE"));
}

HTEXTURECACHE TextureCacheCreatTimeCache(UINT milliSeconds) {
  LPVOID       storage;
  CACHEOBJECT *cacheObject;

  if (!milliSeconds) {
    return 0;
  }

  storage = SMemAlloc(sizeof(CACHEOBJECT), "HTEXTURECACHE", SERR_LINECODE_OBJECT, 0);
  cacheObject = storage ? new (storage) CACHEOBJECT : 0;
  if (!cacheObject) {
    return 0;
  }

  cacheObject->m_cacheTime = milliSeconds;
  cacheObject->m_cacheMode = CACHEMODE_TIME;
  return reinterpret_cast<HTEXTURECACHE>(HandleCreate(cacheObject, "HTEXTURECACHE"));
}

HMIPPEDTEXTURE TextureCacheGetTexture(HTEXTURECACHE cache, LPCSTR fileName, TEXTUREINFO *info) {
  CACHEOBJECT *cacheObject = reinterpret_cast<CACHEOBJECT *>(cache);

  FATALASSERT(cache);

  FATALASSERT(fileName);

  FATALASSERT(info);

  cacheObject->PurgeTextureCache();
  return cacheObject->GetTexture(fileName, info);
}

const MipBits *TextureCacheGetImage(HMIPPEDTEXTURE texture) {
  CACHEENTRY *object = reinterpret_cast<CACHEENTRY *>(texture);

  if (!object) {
    return 0;
  }

  return TextureGetMips(object->m_texture, 1);
}

BOOL TextureCacheGetInfo(HMIPPEDTEXTURE texture, TEXTUREINFO &info, BOOL bForce) {
  CACHEENTRY *object = reinterpret_cast<CACHEENTRY *>(texture);

  if (!object) {
    return 0;
  }

  if (!object->m_textureInfo.width) {
    if (!TextureGetInfo(
            object->m_texture, object->m_textureInfo.width, object->m_textureInfo.height, object->m_textureInfo.format, object->m_textureInfo.opaque,
            object->m_textureInfo.alphaBits, bForce
        ))
    {
      return 0;
    }

    object->m_textureInfo.levels = TextureCalcMipCount(object->m_textureInfo.width, object->m_textureInfo.height);
  }

  info = object->m_textureInfo;
  return 1;
}

HMIPPEDTEXTURE TextureCacheAllocUncachedImage(EGxTexFormat format, UINT width, UINT height, TEXTUREINFO *textureInfo) {
  CACHEENTRY *newObj;

  ASSERT(!(width & (width - 1)));
  ASSERT(!(height & (height - 1)));
  ASSERT(format < GxTexFormats_Last);
  ASSERT(textureInfo);

  newObj = new (SMemAlloc(sizeof(CACHEENTRY), "HMIPPEDTEXTURE", SERR_LINECODE_OBJECT, 0)) CACHEENTRY;
  ASSERT(newObj);

  newObj->m_texture = TextureAllocImage(format, width, height);
  newObj->m_textureInfo.width = width;
  newObj->m_textureInfo.height = height;
  newObj->m_textureInfo.levels = TextureCalcMipCount(width, height);
  newObj->m_textureInfo.format = format;
  newObj->m_uncached = 1;
  *textureInfo = newObj->m_textureInfo;

  return reinterpret_cast<HMIPPEDTEXTURE>(HandleCreate(newObj, "HMIPPEDTEXTURE"));
}
