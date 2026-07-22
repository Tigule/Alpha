#include "Anim/AnimInternal.h"

#include <stddef.h>
#include <malloc.h>
#include <stpl.h>

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

CAnim *__fastcall AnimCreate(unsigned int *const objectCounts, unsigned int numGeosets, unsigned int numCameras, unsigned int numMaterialLayers);
int __fastcall    AnimBuild(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, unsigned int flags);

void __fastcall AnimAddSequences(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared);
void __fastcall AnimAddCameras(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void __fastcall AnimAddGeosets(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void __fastcall AnimAddTextureAnims(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
void __fastcall AnimAddMaterialLayers(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
unsigned int __fastcall   GetGenObjectCount(unsigned char *fileData, unsigned int fileBytes);
unsigned char *__fastcall CreateBone(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateHitTestShape(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateLight(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateHelper(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateAttachmentPoint(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateParticleEmitter2(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateRibbonEmitter(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *__fastcall CreateEventObject(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);

void __fastcall BuildHierarchy(CAnimData *shared, const unsigned int *parentIds, const unsigned int *idConversion, unsigned int numObjects);
void __fastcall AnimInit(CAnim *unique, CAnimData *shared);

struct ANIMHASH : public TSHashObject<ANIMHASH, HASHKEY_STRI> {
  HANIM anim;
};

static TSHashTable<ANIMHASH, HASHKEY_STRI> s_animCache;

static unsigned int __fastcall GetObjectFlags(unsigned int mdlFlags) {
  unsigned int flags = 0;
  if (mdlFlags & 8) {
    flags = 0x38;
  } else if (mdlFlags & 0x10) {
    flags = 8;
  } else if (mdlFlags & 0x20) {
    flags = 0x10;
  } else if (mdlFlags & 0x40) {
    flags = 0x20;
  }
  if (mdlFlags & 1)
    flags |= 1;
  if (mdlFlags & 2)
    flags |= 2;
  if (mdlFlags & 4)
    flags |= 4;
  if (mdlFlags & 0x4000)
    flags |= 0x40;
  return flags;
}

static unsigned char *__fastcall GenericHandlerAnim(
    unsigned char      *fileData,
    CAnimData          *shared,
    CAnimObj           *currobj,
    const unsigned int *idConversion,
    unsigned int       *parentIds,
    MDLTRACKTYPE        forceType
) {
  FATALASSERT(shared);
  FATALASSERT(currobj);
  unsigned int   genObjBytes = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *dataDone = fileData + genObjBytes;
  SStrCopy(currobj->name, reinterpret_cast<const char *>(fileData + 4), sizeof(currobj->name));
  unsigned int oldObjectId = *reinterpret_cast<unsigned int *>(fileData + 84);
  parentIds[oldObjectId] = *reinterpret_cast<unsigned int *>(fileData + 88);
  unsigned int objectId = idConversion[oldObjectId];
  AnimObjectSetIndex(shared, currobj, objectId);
  currobj->flags = GetObjectFlags(*reinterpret_cast<unsigned int *>(fileData + 92));
  fileData += 96;
  fileData = AddKeyFramesType(fileData, dataDone - fileData, 0x5254474B, shared, &currobj->translation, forceType);
  fileData = AnimObjectSetRotation(fileData, dataDone - fileData, shared, currobj, forceType);
  fileData = AddKeyFramesType(fileData, dataDone - fileData, 0x4353474B, shared, &currobj->scale, forceType);
  FATALASSERT(fileData == dataDone);
  return fileData;
}

unsigned char *__fastcall
CreateBone(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  fileData = GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
  unsigned int geoset = *reinterpret_cast<unsigned int *>(fileData);
  currobj->geosetId = geoset == static_cast<unsigned int>(-1) ? 0xFF : *reinterpret_cast<unsigned int *>(fileData + 4);
  return fileData + 8;
}

unsigned char *__fastcall
CreateHitTestShape(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  unsigned int sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  currobj->geosetId = 0xFF;
  return fileData + sectionLength;
}

unsigned char *__fastcall
CreateLight(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimLightObj *currobj = AnimObjectCreateLight(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += 44;
  data = AnimObjectSetAttenuation(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetIntensity(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetAmbColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetAmbIntensity(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *__fastcall CreateParticleEmitter2(
    unsigned char      *fileData,
    CAnimData          *shared,
    const unsigned int *idConversion,
    unsigned int       *parentIds,
    MDLTRACKTYPE        forceType
) {
  FATALASSERT(shared);
  CAnimEmitter2Obj *currobj = AnimObjectCreateEmitter2(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);

  data += *reinterpret_cast<unsigned int *>(data);
  currobj->squirts = *reinterpret_cast<unsigned int *>(data);
  data += sizeof(unsigned int);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4532504B, shared, &currobj->emissionRate, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4732504B, shared, &currobj->gravity, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4E4C504B, shared, &currobj->longitude, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4C32504B, shared, &currobj->latitude, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5332504B, shared, &currobj->particleSpeed, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5232504B, shared, &currobj->variation, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4E32504B, shared, &currobj->length, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5732504B, shared, &currobj->width, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5A32504B, shared, &currobj->zsource, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5349564B, shared, &currobj->visibility, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x46494C4B, shared, &currobj->lifeSpan, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *__fastcall
CreateRibbonEmitter(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimRibbonObj *currobj = AnimObjectCreateRibbon(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += *reinterpret_cast<unsigned int *>(data);
  data = AnimObjectSetRibbonHeightAbove(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonHeightBelow(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonAlpha(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonSlot(data, fileData + sectionLength - data, shared, currobj);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *__fastcall
CreateEventObject(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimEventObj *currobj = AnimObjectCreateEvent(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data = AnimObjectSetEventTrack(data, fileData + sectionLength - data, shared, currobj);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *__fastcall
CreateHelper(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimObj *currobj = AnimObjectCreateHelper(shared);
  FATALASSERT(currobj);
  return GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
}

unsigned char *__fastcall
CreateAttachmentPoint(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimModelObj *currobj = AnimObjectCreateAttachment(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  currobj->geosetId = *(data + 4);
  data += 0x109;
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5349564B, shared, &currobj->visibility, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static void __fastcall SetObjectParent(CAnimData *shared, unsigned int object, unsigned int parent) {
  FATALASSERT(shared);
  CAnimObj *animobj = GetNodeByIndex(shared, object);
  FATALASSERT(animobj);
  int setObjParent = AnimObjectSetParent(shared, animobj, parent);
  FATALASSERT(setObjParent);
}

void __fastcall BuildHierarchy(CAnimData *shared, const unsigned int *parentIds, const unsigned int *idConversion, unsigned int numObjects) {
  for (unsigned int oldObjectId = 0; oldObjectId < numObjects; ++oldObjectId) {
    unsigned int objectId = idConversion[oldObjectId];
    if (objectId == static_cast<unsigned int>(-1)) {
      continue;
    }

    unsigned int parentId = parentIds[oldObjectId];
    if (parentId != static_cast<unsigned int>(-1)) {
      parentId = idConversion[parentId];
      FATALASSERT(parentId != static_cast<unsigned int>(-1));
    }

    SetObjectParent(shared, objectId, parentId);
  }
}

unsigned int __fastcall GetGenObjectCount(unsigned char *fileData, unsigned int fileBytes) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x4C444F4D);
  ASSERT(section);
  return *reinterpret_cast<unsigned int *>(section + 373);
}

unsigned int __fastcall AnimBuildObjectIdTranslation(
    unsigned char *fileData,
    unsigned int   fileBytes,
    unsigned int   flags,
    unsigned int  *idConversion,
    unsigned int   numObjects
) {
  ASSERT(idConversion);

  unsigned int index;
  for (index = 0; index < numObjects; ++index) {
    idConversion[index] = index;
  }

  unsigned int numRemoved = 0;
  if (!(flags & 1)) {
    unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x54535448);
    if (section) {
      unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned int firstObject = *reinterpret_cast<unsigned int *>(section + 96);
      if (firstObject < firstObject + count) {
        memset(idConversion + firstObject, 0xFF, count * sizeof(unsigned int));
      }
      for (index = firstObject + count; index < numObjects; ++index) {
        idConversion[index] = index - count;
      }
      numRemoved = count;
    }
  }

  if (!(flags & 2)) {
    return numRemoved;
  }

  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x4554494C);
  if (!section) {
    return numRemoved;
  }

  unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned int firstObject = *reinterpret_cast<unsigned int *>(section + 96);
  if (firstObject < firstObject + count) {
    memset(idConversion + firstObject, 0xFF, count * sizeof(unsigned int));
  }
  for (index = firstObject + count; index < numObjects; ++index) {
    if (idConversion[index] != static_cast<unsigned int>(-1)) {
      idConversion[index] = index - count;
    }
  }
  return numRemoved + count;
}

void __fastcall IAnimCreateObjects(
    unsigned char      *fileData,
    unsigned int        fileBytes,
    CAnimData          *shared,
    unsigned int        flags,
    const unsigned int *idConversion,
    unsigned int       *parentIds
) {
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x454E4F42);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateBone(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  if (flags & 1) {
    section = MDLFileBinarySeek(fileData, fileBytes, 0x54535448);
    if (section) {
      unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
      unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned char *data = section + 8;
      for (unsigned int index = 0; index < count; ++index) {
        data = CreateHitTestShape(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  if (!(flags & 2)) {
    section = MDLFileBinarySeek(fileData, fileBytes, 0x4554494C);
    if (section) {
      unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
      unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned char *data = section + 8;
      for (unsigned int index = 0; index < count; ++index) {
        data = CreateLight(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x504C4548);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateHelper(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x48435441);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 12;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateAttachmentPoint(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x32455250);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateParticleEmitter2(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x42424952);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateRibbonEmitter(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x53545645);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateEventObject(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }
}
int __fastcall AnimBuild(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, unsigned int flags) {
  if (!unique) {
    return 0;
  }

  if (flags & 8) {
    AnimEnableBlending(reinterpret_cast<HANIM>(unique), 1);
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  AnimAddSequences(fileData, fileBytes, unique, shared);
  AnimAddCameras(fileData, fileBytes, shared, forceType);
  AnimAddGeosets(fileData, fileBytes, shared, forceType);
  AnimAddTextureAnims(fileData, fileBytes, unique, shared, forceType);
  AnimAddMaterialLayers(fileData, fileBytes, unique, shared, forceType);

  unsigned int  numObjects = GetGenObjectCount(fileData, fileBytes);
  unsigned int *idConversion = static_cast<unsigned int *>(_alloca(numObjects * sizeof(unsigned int)));
  unsigned int *parentIds = static_cast<unsigned int *>(_alloca(numObjects * sizeof(unsigned int)));
  AnimBuildObjectIdTranslation(fileData, fileBytes, flags, idConversion, numObjects);
  IAnimCreateObjects(fileData, fileBytes, shared, flags, idConversion, parentIds);
  BuildHierarchy(shared, parentIds, idConversion, numObjects);
  AnimInit(unique, shared);
  return 1;
}

void __fastcall IAnimInitializeTime();

void __fastcall AnimInitialize() {
  IAnimInitializeTime();
}

static unsigned int __fastcall CountSectionEntries(unsigned char *fileData, unsigned int fileBytes, unsigned long tag) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, tag);
  return section ? *reinterpret_cast<unsigned int *>(section + 4) : 0;
}

HANIM __fastcall AnimCreate(unsigned char *fileData, unsigned int fileBytes, unsigned int flags) {
  unsigned int   animatedLayers = 0;
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (section) {
    animatedLayers = *reinterpret_cast<unsigned int *>(section + 8);
  }

  unsigned int objectCounts[7];
  objectCounts[0] = CountSectionEntries(fileData, fileBytes, 0x504C4548);
  objectCounts[1] = (flags & 2) ? 0 : CountSectionEntries(fileData, fileBytes, 0x4554494C);
  objectCounts[2] = CountSectionEntries(fileData, fileBytes, 0x48435441);
  objectCounts[3] = CountSectionEntries(fileData, fileBytes, 0x454E4F42);
  if (flags & 1) {
    objectCounts[3] += CountSectionEntries(fileData, fileBytes, 0x54535448);
  }
  objectCounts[4] = CountSectionEntries(fileData, fileBytes, 0x32455250);
  objectCounts[5] = CountSectionEntries(fileData, fileBytes, 0x42424952);
  objectCounts[6] = CountSectionEntries(fileData, fileBytes, 0x53545645);

  unsigned int numGeosets = CountSectionEntries(fileData, fileBytes, 0x414F4547);
  unsigned int numCameras = CountSectionEntries(fileData, fileBytes, 0x534D4143);
  CAnim       *unique = AnimCreate(objectCounts, numGeosets, numCameras, animatedLayers);
  if (!AnimBuild(fileData, fileBytes, unique, flags)) {
    return 0;
  }

  HANIM anim = static_cast<HANIM>(HandleCreate(unique, "HANIM"));
  if (!anim) {
    delete unique;
  }
  return anim;
}

void __fastcall AnimDestroy() {
  s_animCache.Clear();
}
