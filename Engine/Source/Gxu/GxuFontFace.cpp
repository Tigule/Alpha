#include "IGxuFont.h"

#include <Base/Handle.h>

#include <freetype/freetype.h>

struct FACEDATA : public CHandleObject, public TSHashObject<FACEDATA, HASHKEY_STRI> {
  FACEDATA() : data(0), face(0) {
  }

  ~FACEDATA() {
    ASSERT(!selfReference);

    if (face) {
      FT_Done_Face(face);
    }

    if (data) {
      SFile::Unload(data);
    }
  }

  void    *data;
  FT_Face  face;
  HFACE__ *selfReference;
};

static TSHashTable<FACEDATA, HASHKEY_STRI> s_faceHash;

HFACE__ *FontFaceGetHandle(const char *fileName, FT_LibraryRec_ *library) {
  void         *data = 0;
  unsigned long size;
  FT_Face       theFace;
  HFACE__      *handle = 0;
  FACEDATA     *faceData;
  void         *storage;

  if (!library || !fileName || !*fileName) {
    return 0;
  }

  faceData = s_faceHash.Ptr(fileName);
  if (faceData) {
    ASSERT(faceData->selfReference);
    return reinterpret_cast<HFACE__ *>(HandleDuplicate(reinterpret_cast<HOBJECT>(faceData->selfReference)));
  }

  if (!SFile::Load(0, fileName, &data, &size, 0, 3, 0)) {
    goto finallylabel;
  }

  if (!data) {
    goto finallylabel;
  }

  if (FT_New_Memory_Face(library, static_cast<FT_Byte *>(data), size, 0, &theFace)) {
    goto finallylabel;
  }

  if (!theFace) {
    goto finallylabel;
  }

  if (FT_Select_Charmap(theFace, ft_encoding_unicode)) {
    goto finallylabel;
  }

  storage = SMemAlloc(sizeof(FACEDATA), "HFACE", SERR_LINECODE_OBJECT, 0);
  faceData = storage ? new (storage) FACEDATA : 0;
  ASSERT(faceData);

  faceData->data = data;
  faceData->face = theFace;
  s_faceHash.Insert(faceData, fileName);

  data = 0;
  theFace = 0;
  faceData->selfReference = reinterpret_cast<HFACE__ *>(HandleCreate(faceData, "HFACE"));
  handle = reinterpret_cast<HFACE__ *>(HandleDuplicate(reinterpret_cast<HOBJECT>(faceData->selfReference)));

finallylabel:
  if (data) {
    SFile::Unload(data);
  }

  return handle;
}

FT_FaceRec_ *FontFaceGetFace(HFACE__ *handle) {
  FACEDATA *dataPtr;

  FATALASSERT(handle);

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  ASSERT(dataPtr->selfReference);
  return dataPtr->face;
}

void FontFaceCloseHandle(HFACE__ *handle) {
  FACEDATA    *dataPtr;
  unsigned int refCount;

  FATALASSERT(handle);

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  HandleClose(reinterpret_cast<HOBJECT>(handle));

  refCount = dataPtr->GetRefCount();
  ASSERT(refCount >= 1);
  ASSERT(dataPtr->selfReference);

  if (refCount <= 1) {
    HFACE__ *selfReference = dataPtr->selfReference;

    dataPtr->selfReference = 0;
    HandleClose(reinterpret_cast<HOBJECT>(selfReference));
  }
}

const char *FontFaceGetFontName(HFACE__ *handle) {
  FACEDATA *dataPtr;

  FATALASSERT(handle);

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  return dataPtr->GetString();
}
