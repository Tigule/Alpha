#include <Base/Base.h>

#include "IGxuFont.h"

#include <Base/Handle.h>

#include <freetype/freetype.h>

DECLARE_DERIVED_HANDLE(HFACE, HOBJECT);

struct FACEDATA : public TSHashObject<FACEDATA, HASHKEY_STRI>, public CHandleObject {
  FACEDATA() : data(NULL), face(NULL) {
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

  LPVOID   data;
  FT_Face  face;
  HFACE    selfReference;
};

static TSHashTable<FACEDATA, HASHKEY_STRI> s_faceHash;

HFACE FontFaceGetHandle(LPCSTR fileName, FT_LibraryRec_ *library) {
  LPVOID    data;
  DWORD     size;
  FT_Face   theFace;
  HFACE     handle;
  FACEDATA *faceData;
  LPVOID    storage;

  if (!library || !fileName || !*fileName) {
    return NULL;
  }

  faceData = s_faceHash.Ptr(fileName);
  if (faceData) {
    ASSERT(faceData->selfReference);
    return (HFACE)HandleDuplicate(faceData->selfReference);
  }

  data   = NULL;
  handle = NULL;
  if (!SFile::Load(0, fileName, &data, &size, 0, 3, 0)) {
    goto finallylabel;
  }

  if (!data) {
    goto finallylabel;
  }

  if (FT_New_Memory_Face(library, (FT_Byte *)data, size, 0, &theFace)) {
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

  data = NULL;
  theFace = NULL;
  faceData->selfReference = CREATEHANDLE(HFACE, faceData);
  handle = (HFACE)HandleDuplicate(faceData->selfReference);

finallylabel:
  if (data) {
    SFile::Unload(data);
  }

  return handle;
}

FT_FaceRec_ *FontFaceGetFace(HFACE handle) {
  FACEDATA *dataPtr;

  VALIDATE2(handle);

  dataPtr = (FACEDATA *)handle;
  ASSERT(dataPtr->selfReference);
  return dataPtr->face;
}

void FontFaceCloseHandle(HFACE handle) {
  FACEDATA *dataPtr;
  UINT      refCount;

  VALIDATE2VOID(handle);

  dataPtr = (FACEDATA *)handle;
  HandleClose(handle);

  refCount = dataPtr->GetRefCount();
  ASSERT(refCount >= 1);
  ASSERT(dataPtr->selfReference);

  if (refCount <= 1) {
    HFACE selfReference = dataPtr->selfReference;

    dataPtr->selfReference = NULL;
    HandleClose(selfReference);
  }
}

LPCSTR FontFaceGetFontName(HFACE handle) {
  FACEDATA *dataPtr;

  VALIDATE2(handle);

  dataPtr = (FACEDATA *)handle;
  return dataPtr->GetString();
}
