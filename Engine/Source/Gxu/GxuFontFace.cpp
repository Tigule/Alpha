#include <Base/Base.h>

#include "IGxuFont.h"

#include <Base/Handle.h>

#include <freetype/freetype.h>

DECLARE_DERIVED_HANDLE(HFACE, HOBJECT);

struct FACEDATA : public TSHashObject<FACEDATA, HASHKEY_STRI>, public CHandleObject {
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

  LPVOID   data;
  FT_Face  face;
  HFACE__ *selfReference;
};

static TSHashTable<FACEDATA, HASHKEY_STRI> s_faceHash;

HFACE__ *FontFaceGetHandle(LPCSTR fileName, FT_LibraryRec_ *library) {
  LPVOID    data = 0;
  DWORD     size;
  FT_Face   theFace;
  HFACE__  *handle = 0;
  FACEDATA *faceData;
  LPVOID    storage;

  if (!library || !fileName || !*fileName) {
    return 0;
  }

  faceData = s_faceHash.Ptr(fileName);
  if (faceData) {
    ASSERT(faceData->selfReference);
    return static_cast<HFACE>(HandleDuplicate(faceData->selfReference));
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
  faceData->selfReference = CREATEHANDLE(HFACE, faceData);
  handle = static_cast<HFACE>(HandleDuplicate(faceData->selfReference));

finallylabel:
  if (data) {
    SFile::Unload(data);
  }

  return handle;
}

FT_FaceRec_ *FontFaceGetFace(HFACE__ *handle) {
  FACEDATA *dataPtr;

  VALIDATEBEGIN;
  VALIDATE(handle);
  VALIDATEEND;

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  ASSERT(dataPtr->selfReference);
  return dataPtr->face;
}

void FontFaceCloseHandle(HFACE__ *handle) {
  FACEDATA *dataPtr;
  UINT      refCount;

  VALIDATEBEGIN;
  VALIDATE(handle);
  VALIDATEENDVOID;

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  HandleClose(handle);

  refCount = dataPtr->GetRefCount();
  ASSERT(refCount >= 1);
  ASSERT(dataPtr->selfReference);

  if (refCount <= 1) {
    HFACE__ *selfReference = dataPtr->selfReference;

    dataPtr->selfReference = 0;
    HandleClose(selfReference);
  }
}

LPCSTR FontFaceGetFontName(HFACE__ *handle) {
  FACEDATA *dataPtr;

  VALIDATEBEGIN;
  VALIDATE(handle);
  VALIDATEEND;

  dataPtr = reinterpret_cast<FACEDATA *>(handle);
  return dataPtr->GetString();
}
