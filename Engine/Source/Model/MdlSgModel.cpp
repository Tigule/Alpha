#include "Model/ModelInternal.h"
#include "Base/Status.h"
#include "MDLFile/MDLTypes.h"
#include "Tempest/cmath.h"

#include <storm.h>
#include <string.h>

unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

HTEXTURE LoadModelTexture(const char *texturePath, unsigned int modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);

static void GetTextureFlags(const MDLTEXTURESECTION &texdata, CGxTexFlags *flags) {
  if (texdata.flags & 0x1) {
    flags->m_wrapU = 1;
  }
  if (texdata.flags & 0x2) {
    flags->m_wrapV = 1;
  }
}

static void
ProcessTextures(const MDLTEXTURESECTION *texdata, unsigned int numTextures, unsigned int flags, CStatus *status, CModelTexture *textures) {
  static NTempest::CImVector s_uglyPink(0xFFFF00FFul);

  for (unsigned int textureIndex = 0; textureIndex < numTextures; ++textureIndex) {
    textures[textureIndex].replaceableId = texdata[textureIndex].replaceableId;

    if (((flags & 0x800) && texdata[textureIndex].replaceableId) || !static_cast<const char *>(texdata[textureIndex].image)[0]) {
      textures[textureIndex].handle = TextureCreateSolid(s_uglyPink, 0);
      continue;
    }

    EGxTexFilter filter = GxTex_LinearMipNearest;
    unsigned int maxAnisotropy = 1;
    if (flags & 0x10000) {
      filter = GxTex_Anisotropic;
      maxAnisotropy = 1 << ((flags >> 17) & 0x7);
    } else if (flags & 0x1000) {
      filter = GxTex_LinearMipLinear;
    }

    CGxTexFlags texFlags(filter, 0, 0, 0, 0, 0, maxAnisotropy);
    GetTextureFlags(texdata[textureIndex], &texFlags);
    textures[textureIndex].handle = LoadModelTexture(texdata[textureIndex].image, flags, texFlags, status);
  }
}

static unsigned int GetTmuPassFlags(unsigned int createFlags, unsigned int layerFlags) {
  unsigned int result = (layerFlags & 0x2) != 0;
  if (createFlags & 0x100000) {
    result |= 0x2;
  }
  return result;
}

static EGxTextureShader GetTextureShader(unsigned int transformId, unsigned int createFlags) {
  return transformId != static_cast<unsigned int>(-1) && !(createFlags & 0x100) ? GxTS_Affine : GxTS_PassThru;
}

static void
ProcessTexLayers(const MDLMATERIALSECTION &sectionData, CMaterial *unique, CMaterialShared *shared, unsigned int createFlags, unsigned int *layerId) {
  unsigned int numLayers = sectionData.texLayers.Count();
  unique->layers.SetCount(numLayers);
  shared->layers.SetCount(numLayers);

  for (unsigned int i = 0; i < numLayers; ++i) {
    const MDLTEXLAYER &source = sectionData.texLayers[i];
    CTexLayer         &uniqueLayer = unique->layers[i];
    CTexLayerShared   &sharedLayer = shared->layers[i];

    sharedLayer.tmuPass[0].transformId = source.transformId;
    sharedLayer.tmuPass[0].coordId = source.coordId;
    sharedLayer.tmuPass[0].textureShader = GetTextureShader(source.transformId, createFlags);
    sharedLayer.tmuPass[0].flags = GetTmuPassFlags(createFlags, source.flags);

    switch (source.blendMode) {
      case TEXOP_TRANSPARENT:
        sharedLayer.blendMode = GxBlend_AlphaKey;
        break;
      case TEXOP_BLEND:
        sharedLayer.blendMode = GxBlend_Alpha;
        break;
      case TEXOP_ADD:
      case TEXOP_ADD_ALPHA:
        sharedLayer.blendMode = GxBlend_Add;
        break;
      case TEXOP_MODULATE:
        sharedLayer.blendMode = GxBlend_Mod;
        break;
      case TEXOP_MODULATE2X:
        sharedLayer.blendMode = GxBlend_Mod2x;
        break;
      default:
        sharedLayer.blendMode = GxBlend_Opaque;
        break;
    }

    uniqueLayer.tmuPass[0].textureId = source.textureId;
    uniqueLayer.tmuPass[0].combiner = GxTexBlend_Mod;
    uniqueLayer.blendMode = sharedLayer.blendMode;
    uniqueLayer.vertexFormat = source.coordId == static_cast<unsigned int>(-1) ? GxVBF_PN : GxVBF_PNT0;

    if (source.blendMode >= TEXOP_BLEND && source.blendMode <= TEXOP_MODULATE2X) {
      uniqueLayer.disables |= 0x8;
    }
    if (source.flags & 0x1) {
      uniqueLayer.disables |= 0x1;
    }
    if (source.flags & 0x10) {
      uniqueLayer.disables |= 0x10;
    }
    if (source.flags & 0x20) {
      uniqueLayer.disables |= 0x2;
    }
    if (source.flags & 0x40) {
      uniqueLayer.disables |= 0x4;
    }
    if (source.flags & 0x80) {
      uniqueLayer.disables |= 0x8;
    }
  }

  *layerId += numLayers;
}

static unsigned int
ProcessMaterials(const TSGrowableArray<MDLMATERIALSECTION> &sectionData, unsigned int createFlags, HMATERIAL *materials) {
  unsigned int numMaterials = sectionData.Count();
  unsigned int layerId = 0;

  for (unsigned int i = 0; i < numMaterials; ++i) {
    CMaterial       *unique = NEW(CMaterial);
    CMaterialShared *shared = NEW(CMaterialShared);

    ProcessTexLayers(sectionData[i], unique, shared, createFlags, &layerId);
    unique->data = static_cast<HMATERIALSHARED>(HandleCreate(shared, "HMATERIALSHARED"));
    shared->priorityPlane = sectionData[i].priorityPlane;
    materials[i] = static_cast<HMATERIAL>(HandleCreate(unique, "HMATERIAL"));
  }

  return layerId;
}

static void ProcessLayerAlpha(const TSGrowableArray<MDLMATERIALSECTION> &sectionData, HMATERIAL const *materials) {
  unsigned int numMaterials = sectionData.Count();

  for (unsigned int i = 0; i < numMaterials; ++i) {
    CMaterial   *unique = static_cast<CMaterial *>(HandleDereference(reinterpret_cast<HOBJECT>(materials[i])));
    unsigned int numLayers = unique->layers.Count();
    for (unsigned int j = 0; j < numLayers; ++j) {
      unique->layers[j].layerAlpha = NTempest::CMath::ftol_0_256_(sectionData[i].texLayers[j].staticAlpha * 255.0f);
    }
  }
}

static const EGxPrim s_mdlToGxPrim[10] = {GxPrim_Points,        GxPrim_Lines,       GxPrims_Last, GxPrim_LineStrip, GxPrim_Triangles,
                                          GxPrim_TriangleStrip, GxPrim_TriangleFan, GxPrims_Last, GxPrims_Last,     GxPrims_Last};

static EGxPrim GetPrimitiveType(unsigned char type) {
  ASSERT(type < 10);
  EGxPrim gxPrim = s_mdlToGxPrim[type];
  ASSERT(gxPrim != GxPrims_Last);
  return gxPrim;
}

static const int s_multiPrimType[10] = {1, 0, 0, 0, 1, 0, 0, 0, 0, 0};

static unsigned int CountNumPrimLists(const unsigned char *primTypes, unsigned int numPrimTypes) {
  unsigned int  numPrimLists = 0;
  unsigned char lastType = 10;

  for (unsigned int i = 0; i < numPrimTypes; ++i) {
    if (primTypes[i] != lastType || !s_multiPrimType[primTypes[i]]) {
      ++numPrimLists;
    }
    lastType = primTypes[i];
  }

  return numPrimLists;
}

static void
LoadGeosetPrimitiveTypes(const unsigned char *primTypes, const unsigned int *primVertCounts, unsigned int numPrimTypes, CGeosetShared *geoShared) {
  unsigned int  primList = 0;
  unsigned char lastType = 10;

  for (unsigned int i = 0; i < numPrimTypes; ++i) {
    if (primTypes[i] != lastType || !s_multiPrimType[primTypes[i]]) {
      if (i) {
        ++primList;
      }
      geoShared->primitive.Ptr()[primList].type = GetPrimitiveType(primTypes[i]);
      geoShared->primitive.Ptr()[primList].vertexCount = 0;
    }
    geoShared->primitive.Ptr()[primList].vertexCount += primVertCounts[i];
    lastType = primTypes[i];
  }
}

static unsigned char *LoadGeosetPrimitiveData(unsigned char *data, CGeosetShared *geoShared) {
  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x50595450);
  unsigned int   numPrimTypes = *reinterpret_cast<const unsigned int *>(data + 4);
  unsigned char *primTypes = data + 8;
  data = primTypes + numPrimTypes;

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x544E4350);
  unsigned int numPrimCounts = *reinterpret_cast<const unsigned int *>(data + 4);
  ASSERT(numPrimCounts == numPrimTypes);
  const unsigned int *primVertCounts = reinterpret_cast<const unsigned int *>(data + 8);
  data += 8 + numPrimCounts * sizeof(unsigned int);

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x58545650);
  geoShared->primitiveVertices.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  if (geoShared->primitiveVertices.Count()) {
    memcpy(geoShared->primitiveVertices.Ptr(), data + 8, geoShared->primitiveVertices.Count() * sizeof(unsigned short));
  }
  data += 8 + geoShared->primitiveVertices.Count() * sizeof(unsigned short);

  geoShared->primitive.SetCount(CountNumPrimLists(primTypes, numPrimTypes));
  LoadGeosetPrimitiveTypes(primTypes, primVertCounts, numPrimTypes, geoShared);
  return data;
}

static unsigned char *LoadGeosetTransformGroups(unsigned char *data, unsigned int loadFlags, CGeosetShared *geoShared) {
  if (loadFlags & 0x100) {
    geoShared->boneWeights.SetCount(1);
    geoShared->boneWeights.Ptr()[0] = 0;
    geoShared->groupMatrixCounts.SetCount(1);
    geoShared->groupMatrixCounts.Ptr()[0] = 1;
    geoShared->matrices.SetCount(1);
    geoShared->matrices.Ptr()[0] = 0;

    unsigned int numVertices = geoShared->position.Count();
    geoShared->hwBoneIndices.SetCount(numVertices);
    geoShared->hwBoneWeights.SetCount(numVertices);
    for (unsigned int i = 0; i < numVertices; ++i) {
      geoShared->hwBoneIndices.Ptr()[i] = 0;
      geoShared->hwBoneWeights.Ptr()[i] = 0xFF000000;
    }

    data += 8 + *reinterpret_cast<const unsigned int *>(data + 4);
    for (unsigned int section = 0; section < 4; ++section) {
      data += 8 + 4 * *reinterpret_cast<const unsigned int *>(data + 4);
    }
    return data;
  }

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x58444E47);
  geoShared->boneWeights.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->boneWeights.Ptr(), data + 8, geoShared->boneWeights.Count());
  data += 8 + geoShared->boneWeights.Count();

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x4347544D);
  geoShared->groupMatrixCounts.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->groupMatrixCounts.Ptr(), data + 8, geoShared->groupMatrixCounts.Count() * sizeof(unsigned int));
  data += 8 + geoShared->groupMatrixCounts.Count() * sizeof(unsigned int);

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x5354414D);
  geoShared->matrices.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->matrices.Ptr(), data + 8, geoShared->matrices.Count() * sizeof(unsigned int));
  data += 8 + geoShared->matrices.Count() * sizeof(unsigned int);

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x58444942);
  geoShared->hwBoneIndices.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->hwBoneIndices.Ptr(), data + 8, geoShared->hwBoneIndices.Count() * sizeof(unsigned int));
  data += 8 + geoShared->hwBoneIndices.Count() * sizeof(unsigned int);

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x54475742);
  geoShared->hwBoneWeights.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->hwBoneWeights.Ptr(), data + 8, geoShared->hwBoneWeights.Count() * sizeof(unsigned int));
  data += 8 + geoShared->hwBoneWeights.Count() * sizeof(unsigned int);
  return data;
}

static void
LoadGeosetData(unsigned char *data, unsigned int sectionBytes, unsigned int loadFlags, unsigned int geosetId, CGeosetShared *geoShared) {
  unsigned char *sectionDone = data + sectionBytes;
  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x58545256);
  unsigned int numVertices = *reinterpret_cast<const unsigned int *>(data + 4);
  ASSERT(numVertices <= 0xFFFF);
  geoShared->position.SetCount(numVertices);
  memcpy(geoShared->position.Ptr(), data + 8, numVertices * sizeof(NTempest::C3Vector));
  data += 8 + numVertices * sizeof(NTempest::C3Vector);

  ASSERT(*reinterpret_cast<const unsigned int *>(data) == 0x534D524E);
  ASSERT(numVertices == *reinterpret_cast<const unsigned int *>(data + 4));
  geoShared->normal.SetCount(*reinterpret_cast<const unsigned int *>(data + 4));
  memcpy(geoShared->normal.Ptr(), data + 8, geoShared->normal.Count() * sizeof(NTempest::C3Vector));
  data += 8 + geoShared->normal.Count() * sizeof(NTempest::C3Vector);

  if (*reinterpret_cast<const unsigned int *>(data) == 0x53415655) {
    unsigned int numMappingChannels = *reinterpret_cast<const unsigned int *>(data + 4);
    data += 8;
    geoShared->texCoord.SetCount(numMappingChannels);
    for (unsigned int i = 0; i < numMappingChannels; ++i) {
      geoShared->texCoord[i].SetCount(numVertices);
      if (numVertices) {
        memcpy(geoShared->texCoord[i].Ptr(), data, numVertices * sizeof(NTempest::C2Vector));
      }
      data += numVertices * sizeof(NTempest::C2Vector);
    }
  }

  data = LoadGeosetPrimitiveData(data, geoShared);
  data = LoadGeosetTransformGroups(data, loadFlags, geoShared);

  geoShared->materialId = *reinterpret_cast<const unsigned int *>(data);
  geoShared->selectionGroup = *reinterpret_cast<const unsigned int *>(data + 4);
  geoShared->flags = *reinterpret_cast<const unsigned int *>(data + 8);
  geoShared->radius = *reinterpret_cast<const float *>(data + 12);
  data += 16;

  const NTempest::C3Vector &minimum = *reinterpret_cast<const NTempest::C3Vector *>(data);
  const NTempest::C3Vector &maximum = *reinterpret_cast<const NTempest::C3Vector *>(data + 12);
  geoShared->centroid = (minimum + maximum) * 0.5f;
  data += 24;

  unsigned int numSequenceBounds = *reinterpret_cast<const unsigned int *>(data);
  data += 4 + numSequenceBounds * 7 * sizeof(unsigned int);
  ASSERT(data == sectionDone);

  geoShared->geosetId = geosetId;
  geoShared->vertexShader = geoShared->groupMatrixCounts.Count() > 1 ? GxVS_Skin : GxVS_PassThru;
}

static void CreateGeoset(const MDLGEOSETSECTION &geosetdata, unsigned int geosetId, unsigned int loadFlags, CGeosetShared *geoShared) {
  ASSERT(geosetdata.vertices.Count() <= 0xFFFF);

  static_cast<TSFixedArray<NTempest::C3Vector> &>(geoShared->position) = static_cast<const TSFixedArray<NTempest::C3Vector> &>(geosetdata.vertices);
  static_cast<TSFixedArray<NTempest::C3Vector> &>(geoShared->normal) = static_cast<const TSFixedArray<NTempest::C3Vector> &>(geosetdata.normals);

  geoShared->texCoord.SetCount(geosetdata.texCoords.Count());
  for (unsigned int i = 0; i < geosetdata.texCoords.Count(); ++i) {
    static_cast<TSFixedArray<NTempest::C2Vector> &>(geoShared->texCoord.Ptr()[i]) =
        static_cast<const TSFixedArray<NTempest::C2Vector> &>(geosetdata.texCoords[i]);
  }

  unsigned int numPrimTypes = geosetdata.primitives.types.Count();
  unsigned int numPrimLists = CountNumPrimLists(geosetdata.primitives.types.Ptr(), numPrimTypes);
  geoShared->primitive.SetCount(numPrimLists);

  ASSERT(numPrimTypes == geosetdata.primitives.counts.Count());
  LoadGeosetPrimitiveTypes(geosetdata.primitives.types.Ptr(), geosetdata.primitives.counts.Ptr(), numPrimTypes, geoShared);
  static_cast<TSFixedArray<unsigned short> &>(geoShared->primitiveVertices) =
      static_cast<const TSFixedArray<unsigned short> &>(geosetdata.primitives.vertices);

  if (loadFlags & 0x100) {
    geoShared->groupMatrixCounts.SetCount(1);
    geoShared->groupMatrixCounts.Ptr()[0] = 1;
    geoShared->matrices.SetCount(1);
    geoShared->matrices.Ptr()[0] = 0;
    geoShared->boneWeights.SetCount(1);
    geoShared->boneWeights.Ptr()[0] = 0;

    unsigned int numVertices = geoShared->position.Count();
    geoShared->hwBoneIndices.SetCount(numVertices);
    memset(geoShared->hwBoneIndices.Ptr(), 0, geoShared->hwBoneIndices.Count() * sizeof(unsigned int));
    geoShared->hwBoneWeights.SetCount(numVertices);
    for (unsigned int i = 0; i < numVertices; ++i) {
      geoShared->hwBoneWeights.Ptr()[i] = 0xFF000000;
    }
  } else {
    static_cast<TSFixedArray<unsigned int> &>(geoShared->groupMatrixCounts) =
        static_cast<const TSFixedArray<unsigned int> &>(geosetdata.groupMatrixCounts);
    static_cast<TSFixedArray<unsigned int> &>(geoShared->matrices) = static_cast<const TSFixedArray<unsigned int> &>(geosetdata.matrices);
    static_cast<TSFixedArray<unsigned char> &>(geoShared->boneWeights) =
        static_cast<const TSFixedArray<unsigned char> &>(geosetdata.vertGroupIndices);
    static_cast<TSFixedArray<unsigned int> &>(geoShared->hwBoneIndices) = static_cast<const TSFixedArray<unsigned int> &>(geosetdata.boneIndices);
    static_cast<TSFixedArray<unsigned int> &>(geoShared->hwBoneWeights) = static_cast<const TSFixedArray<unsigned int> &>(geosetdata.boneWeights);
  }

  geoShared->materialId = geosetdata.materialId;
  geoShared->selectionGroup = geosetdata.selectionGroup;
  geoShared->centroid = (geosetdata.bounds.extent.b + geosetdata.bounds.extent.t) * 0.5f;
  geoShared->geosetId = geosetId;
  geoShared->radius = geosetdata.bounds.radius;
  geoShared->flags = geosetdata.flags;
  geoShared->vertexShader = geoShared->groupMatrixCounts.Count() > 1 ? GxVS_Skin : GxVS_PassThru;
}

static void
CreateGeosetWithNormals(const MDLGEOSETSECTION &geoset, unsigned int geosetId, unsigned int loadFlags, CGeosetShared *geoShared) {
  unsigned int numNormals = geoset.normals.Count();
  if (!(loadFlags & 0x1) || !numNormals) {
    CreateGeoset(geoset, geosetId, loadFlags, geoShared);
    return;
  }

  MDLGEOSETSECTION normalLines(geoset);
  unsigned int     numVertices = geoset.vertices.Count();
  normalLines.vertices.SetCount(numVertices * 2);
  normalLines.normals.SetCount(numVertices * 2);
  normalLines.vertGroupIndices.SetCount(numVertices * 2);

  unsigned int numTexCoords = normalLines.texCoords.Count();
  unsigned int i;
  for (i = 0; i < numTexCoords; ++i) {
    normalLines.texCoords[i].SetCount(numVertices * 2);
  }

  normalLines.primitives.SetCount(1, numVertices * 2);
  *normalLines.primitives.types.Top() = 1;
  *normalLines.primitives.counts.Top() = numVertices * 2;

  NTempest::C3Vector *normVert = normalLines.vertices.Ptr() + numVertices;
  unsigned char      *normGroupId = normalLines.vertGroupIndices.Ptr() + numVertices;
  for (i = 0; i < numVertices; ++i) {
    normVert[i] = geoset.vertices[i] + geoset.normals[i] * 0.12f;
    normGroupId[i] = geoset.vertGroupIndices[i];
    normalLines.primitives.vertices[i * 2] = static_cast<unsigned short>(i);
    normalLines.primitives.vertices[i * 2 + 1] = static_cast<unsigned short>(numVertices + i);
  }

  CreateGeoset(normalLines, geosetId, loadFlags, geoShared);
}

static void BuildAllGeosets(const MDLDATA &data, CGeosetShared *geosets, CGeosetColor *geosetColor, unsigned int loadFlags) {
  unsigned int numGeosets = data.geosets.Count();
  unsigned int i;
  for (i = 0; i < numGeosets; ++i) {
    CreateGeosetWithNormals(data.geosets[i], i, loadFlags, &geosets[i]);
  }

  unsigned int numGeosetAnims = data.geosetAnims.Count();
  for (i = 0; i < numGeosetAnims; ++i) {
    const MDLGEOSETANIMSECTION &source = data.geosetAnims[i];
    CGeosetColor               &color = geosetColor[source.geosetId];
    color.animatedAlpha = source.staticAlpha;
    color.animatedColor.Set(
        NTempest::CMath::ftol_0_256_(source.staticAlpha * 255.0f), NTempest::CMath::ftol_0_256_(source.staticColor.r * 255.0f),
        NTempest::CMath::ftol_0_256_(source.staticColor.g * 255.0f), NTempest::CMath::ftol_0_256_(source.staticColor.b * 255.0f)
    );
  }
}

static void ProcessAttachments(
    const TSGrowableArray<MDLATTACHMENTSECTION> &attachments,
    CModelComplex                               *modelptr,
    CModelShared                                *shared,
    unsigned int                                 loadFlags,
    CStatus                                     *status
) {
  unsigned int numAttachments = attachments.Count();
  if (!numAttachments) {
    return;
  }

  ASSERT(modelptr);
  modelptr->m_attached.SetCount(numAttachments);
  modelptr->m_attachmentFlags.SetCount(numAttachments);
  memset(modelptr->m_attachmentFlags.Ptr(), 0, numAttachments);

  unsigned int highestId = attachments.Ptr()[numAttachments - 1].attachmentId;
  shared->attachIdToIndex.SetCount(highestId + 1);
  memset(shared->attachIdToIndex.Ptr(), 0xFF, shared->attachIdToIndex.Count() * sizeof(unsigned int));

  CModelCreate                                createData;
  CStatus                                     subStatus;
  LISTPTR(LINKUNIQUE) instance = modelptr->m_attached.Ptr();
  for (unsigned int i = 0; i < numAttachments; ++i, ++instance) {
    unsigned int attachmentId = attachments.Ptr()[i].attachmentId;
    shared->attachIdToIndex.Ptr()[attachmentId] = i;
    if (!SStrLen(attachments.Ptr()[i].path)) {
      continue;
    }

    memset(&createData.sequenceNames, 0, sizeof(createData) - sizeof(createData.flags));
    createData.flags = loadFlags;
    HMODEL child = ModelCreate(attachments.Ptr()[i].path, &createData, &subStatus);
    if (child) {
      LINKUNIQUE *link = instance->NewNode(LIST_TAIL, 0, 8);
      link->child = child;
    }

    if (!subStatus.IsEmpty()) {
      status->Add(subStatus.GetHighestSeverity(), "%s:\n", static_cast<const char *>(attachments.Ptr()[i].path));
      status->Add(subStatus);
    }
  }
}

static void LoadLayerData(unsigned char *materialData, CTexLayer *unique, CTexLayerShared *shared, unsigned int createFlags) {
  switch (*reinterpret_cast<unsigned int *>(materialData)) {
    case TEXOP_TRANSPARENT:
      shared->blendMode = GxBlend_AlphaKey;
      break;
    case TEXOP_BLEND:
      shared->blendMode = GxBlend_Alpha;
      break;
    case TEXOP_ADD:
    case TEXOP_ADD_ALPHA:
      shared->blendMode = GxBlend_Add;
      break;
    case TEXOP_MODULATE:
      shared->blendMode = GxBlend_Mod;
      break;
    case TEXOP_MODULATE2X:
      shared->blendMode = GxBlend_Mod2x;
      break;
    default:
      shared->blendMode = GxBlend_Opaque;
      break;
  }
  unique->blendMode = shared->blendMode;

  unsigned int layerFlags = *reinterpret_cast<unsigned int *>(materialData + 4);
  shared->tmuPass[0].flags = GetTmuPassFlags(createFlags, layerFlags);
  unique->tmuPass[0].textureId = *reinterpret_cast<unsigned int *>(materialData + 8);
  shared->tmuPass[0].transformId = *reinterpret_cast<unsigned int *>(materialData + 12);
  shared->tmuPass[0].coordId = *reinterpret_cast<unsigned int *>(materialData + 16);

  float alpha = *reinterpret_cast<const float *>(materialData + 20);
  unique->layerAlpha = static_cast<unsigned char>(NTempest::CMath::fuint_n(alpha * 255.0f));
  shared->tmuPass[0].textureShader = GetTextureShader(shared->tmuPass[0].transformId, createFlags);

  unique->tmuPass[0].combiner = GxTexBlend_Mod;
  unique->vertexFormat = shared->tmuPass[0].coordId == static_cast<unsigned int>(-1) ? GxVBF_PN : GxVBF_PNT0;

  if (unique->blendMode >= GxBlend_Alpha && unique->blendMode <= GxBlend_ModAdd) {
    unique->disables |= 0x8;
  }
  if (layerFlags & 0x1) {
    unique->disables |= 0x1;
  }
  if (layerFlags & 0x10) {
    unique->disables |= 0x10;
  }
  if (layerFlags & 0x20) {
    unique->disables |= 0x2;
  }
  if (layerFlags & 0x40) {
    unique->disables |= 0x4;
  }
  if (layerFlags & 0x80) {
    unique->disables |= 0x8;
  }
}

static HMATERIAL LoadMaterialData(unsigned char *materialData, unsigned int createFlags, unsigned char *totalLayers) {
  CMaterial       *unique = NEW(CMaterial);
  CMaterialShared *shared = NEW(CMaterialShared);

  shared->priorityPlane = *reinterpret_cast<unsigned int *>(materialData);
  unsigned int numLayers = *reinterpret_cast<unsigned int *>(materialData + 4);
  materialData += 8;
  *totalLayers += static_cast<unsigned char>(numLayers);

  unique->layers.SetCount(numLayers);
  shared->layers.SetCount(numLayers);
  for (unsigned int i = 0; i < numLayers; ++i) {
    unsigned int bytesThisLayer = *reinterpret_cast<unsigned int *>(materialData);
    LoadLayerData(materialData + 4, &unique->layers.Ptr()[i], &shared->layers.Ptr()[i], createFlags);
    materialData += bytesThisLayer;
  }

  unique->data = static_cast<HMATERIALSHARED>(HandleCreate(shared, "HMATERIALSHARED"));
  return static_cast<HMATERIAL>(HandleCreate(unique, "HMATERIAL"));
}

static unsigned int
LoadAttachment(unsigned char *data, unsigned int loadFlags, LISTPTR(LINKUNIQUE) attachment, CStatus *status) {
  unsigned int   dataOffset = *reinterpret_cast<unsigned int *>(data);
  unsigned char *attachmentData = data + dataOffset;
  unsigned int   attachmentId = *reinterpret_cast<unsigned int *>(attachmentData);
  const char    *fileName = reinterpret_cast<const char *>(attachmentData + 5);

  if (!SStrLen(fileName)) {
    return attachmentId;
  }

  CModelCreate createData;
  memset(&createData.sequenceNames, 0, sizeof(createData) - sizeof(createData.flags));
  createData.flags = loadFlags;

  CStatus subStatus;
  HMODEL  child = ModelCreate(fileName, &createData, &subStatus);
  if (child) {
    LINKUNIQUE *link = attachment->NewNode(LIST_TAIL, 0, 8);
    link->child = child;
  }

  if (!subStatus.IsEmpty()) {
    status->Add(subStatus.GetHighestSeverity(), "%s:\n", fileName);
    status->Add(subStatus);
  }

  return attachmentId;
}

void MdlReadLoadGlobalProperties(const MDLDATA &data, CModelShared *shared, unsigned int *loadFlags) {
  ASSERT(shared);
  shared->groundTrack = static_cast<GROUND_TRACK>(data.model.flags & GROUND_TRACK_MASK);
  if (data.model.flags & 0x4) {
    *loadFlags &= ~0x100;
  }
}

int MdlReadLoadModel(const MDLDATA &data, CModelComplex *modelptr, CModelShared *shared, unsigned int flags, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned int numTextures = data.textures.Count();
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(data.textures.Ptr(), numTextures, flags, status, modelptr->m_textures.Ptr());

  unsigned int numMaterials = data.materials.Count();
  modelptr->m_materials.SetCount(numMaterials);
  unsigned int numLayers = ProcessMaterials(data.materials, flags, modelptr->m_materials.Ptr());
  ProcessLayerAlpha(data.materials, modelptr->m_materials.Ptr());

  unsigned int numGeosets = data.geosets.Count();
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);
  BuildAllGeosets(data, shared->geosets.Ptr(), modelptr->m_geosetColor.Ptr(), flags);
  ProcessAttachments(data.attachments, modelptr, shared, flags, status);

  ASSERT(numGeosets <= 0xFF);
  ASSERT(numLayers <= 0xFF);
  shared->numGeosets = static_cast<unsigned char>(numGeosets);
  shared->numLayers = static_cast<unsigned char>(numLayers);
  return 1;
}

int MdlReadLoadModel(const MDLDATA &data, CModelSimple *modelptr, CModelShared *shared, unsigned int flags, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned int numTextures = data.textures.Count();
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(data.textures.Ptr(), numTextures, flags, status, modelptr->m_textures.Ptr());

  unsigned int numMaterials = data.materials.Count();
  modelptr->m_materials.SetCount(numMaterials);
  unsigned int numLayers = ProcessMaterials(data.materials, flags, modelptr->m_materials.Ptr());
  ProcessLayerAlpha(data.materials, modelptr->m_materials.Ptr());

  unsigned int numGeosets = data.geosets.Count();
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);
  BuildAllGeosets(data, shared->geosets.Ptr(), modelptr->m_geosetColor.Ptr(), flags);

  ASSERT(numGeosets <= 0xFF);
  ASSERT(numLayers <= 0xFF);
  shared->numGeosets = static_cast<unsigned char>(numGeosets);
  shared->numLayers = static_cast<unsigned char>(numLayers);
  return 1;
}

void MdxLoadGlobalProperties(unsigned char *fileData, unsigned int fileBytes, unsigned int *loadFlags, CModelShared *modelShared) {
  ASSERT(modelShared);
  ASSERT(loadFlags);
  unsigned char *data = MDLFileBinarySeek(fileData, fileBytes, 0x4C444F4D);
  ASSERT(data != 0);

  unsigned char globalFlags = data[0x174];
  modelShared->groundTrack = static_cast<GROUND_TRACK>(globalFlags & GROUND_TRACK_MASK);
  if (globalFlags & 0x4) {
    *loadFlags &= ~0x100;
  }
}

void MdxReadTextures(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x53584554);
  if (!section) {
    return;
  }

  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(section);
  unsigned int numTextures = sectionBytes / sizeof(MDLTEXTURESECTION);
  ASSERT(sectionBytes == numTextures * sizeof(MDLTEXTURESECTION));
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(reinterpret_cast<MDLTEXTURESECTION *>(section + 4), numTextures, flags, status, modelptr->m_textures.Ptr());
}

void MdxReadTextures(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelSimple *modelptr, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x53584554);
  if (!section) {
    return;
  }

  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(section);
  unsigned int numTextures = sectionBytes / sizeof(MDLTEXTURESECTION);
  ASSERT(sectionBytes == numTextures * sizeof(MDLTEXTURESECTION));
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(reinterpret_cast<MDLTEXTURESECTION *>(section + 4), numTextures, flags, status, modelptr->m_textures.Ptr());
}

void MdxReadMaterials(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CModelShared *shared) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (!section) {
    return;
  }

  unsigned char *data = section + 12;
  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numMaterials = *reinterpret_cast<unsigned int *>(section + 4);
  shared->numLayers = 0;
  modelptr->m_materials.SetCount(numMaterials);
  for (unsigned int i = 0; i < numMaterials; ++i) {
    unsigned int bytesThisMaterial = *reinterpret_cast<unsigned int *>(data);
    modelptr->m_materials.Ptr()[i] = LoadMaterialData(data + 4, flags, &shared->numLayers);
    data += bytesThisMaterial;
    ASSERT(data <= dataDone);
  }
  ASSERT(data == dataDone);
}

void MdxReadMaterials(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelSimple *modelptr, CModelShared *shared) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (!section) {
    return;
  }

  unsigned char *data = section + 12;
  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numMaterials = *reinterpret_cast<unsigned int *>(section + 4);
  shared->numLayers = 0;
  modelptr->m_materials.SetCount(numMaterials);
  for (unsigned int i = 0; i < numMaterials; ++i) {
    unsigned int bytesThisMaterial = *reinterpret_cast<unsigned int *>(data);
    modelptr->m_materials[i] = LoadMaterialData(data + 4, flags, &shared->numLayers);
    data += bytesThisMaterial;
    ASSERT(data <= dataDone);
  }
  ASSERT(data == dataDone);
}

void MdxReadGeosets(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534F4547);
  if (!section) {
    return;
  }

  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(section);
  unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
  ASSERT(count <= 0xFF);
  shared->numGeosets = count;
  modelptr->m_geosets.SetCount(count);
  shared->geosets.SetCount(count);
  modelptr->m_geosetColor.SetCount(count);

  unsigned char *data = section + 8;
  unsigned char *done = section + 4 + sectionBytes;
  unsigned int   i;
  for (i = 0; i < count; ++i) {
    unsigned int bytesThisGeo = *reinterpret_cast<unsigned int *>(data);
    ASSERT(data + bytesThisGeo <= done);
    LoadGeosetData(data + 4, bytesThisGeo - 4, flags, i, &shared->geosets.Ptr()[i]);
    data += bytesThisGeo;
  }
  ASSERT(data == done);

  section = MDLFileBinarySeek(fileData, fileBytes, 0x414F4547);
  if (!section) {
    return;
  }
  sectionBytes = *reinterpret_cast<unsigned int *>(section);
  count = *reinterpret_cast<unsigned int *>(section + 4);
  data = section + 8;
  done = section + 4 + sectionBytes;
  for (i = 0; i < count; ++i) {
    unsigned int bytesThisAnim = *reinterpret_cast<unsigned int *>(data);
    unsigned int geosetId = *reinterpret_cast<unsigned int *>(data + 4);
    ASSERT(geosetId < modelptr->m_geosetColor.Count());
    CGeosetColor &color = modelptr->m_geosetColor.Ptr()[geosetId];
    color.animatedAlpha = *reinterpret_cast<float *>(data + 8);
    color.animatedColor.Set(
        NTempest::CMath::ftol_0_256_(color.animatedAlpha * 255.0f), NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 12) * 255.0f),
        NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 16) * 255.0f),
        NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 20) * 255.0f)
    );
    data += bytesThisAnim;
    ASSERT(data <= done);
  }
  ASSERT(data == done);
}

void MdxReadGeosets(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelSimple *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534F4547);
  if (!section) {
    return;
  }

  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(section);
  unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
  ASSERT(count <= 0xFF);
  shared->numGeosets = count;
  modelptr->m_geosets.SetCount(count);
  shared->geosets.SetCount(count);
  modelptr->m_geosetColor.SetCount(count);

  unsigned char *data = section + 8;
  unsigned char *done = section + 4 + sectionBytes;
  unsigned int   i;
  for (i = 0; i < count; ++i) {
    unsigned int bytesThisGeo = *reinterpret_cast<unsigned int *>(data);
    ASSERT(data + bytesThisGeo <= done);
    LoadGeosetData(data + 4, bytesThisGeo - 4, flags, i, &shared->geosets.Ptr()[i]);
    data += bytesThisGeo;
  }
  ASSERT(data == done);

  section = MDLFileBinarySeek(fileData, fileBytes, 0x414F4547);
  if (!section) {
    return;
  }
  sectionBytes = *reinterpret_cast<unsigned int *>(section);
  count = *reinterpret_cast<unsigned int *>(section + 4);
  data = section + 8;
  done = section + 4 + sectionBytes;
  for (i = 0; i < count; ++i) {
    unsigned int bytesThisAnim = *reinterpret_cast<unsigned int *>(data);
    unsigned int geosetId = *reinterpret_cast<unsigned int *>(data + 4);
    ASSERT(geosetId < modelptr->m_geosetColor.Count());
    CGeosetColor &color = modelptr->m_geosetColor[geosetId];
    color.animatedAlpha = *reinterpret_cast<float *>(data + 8);
    color.animatedColor.Set(
        NTempest::CMath::ftol_0_256_(color.animatedAlpha * 255.0f), NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 12) * 255.0f),
        NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 16) * 255.0f),
        NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(data + 20) * 255.0f)
    );
    data += bytesThisAnim;
    ASSERT(data <= done);
  }
  ASSERT(data == done);
}

void
MdxReadAttachments(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x48435441);
  if (!section) {
    return;
  }

  unsigned int   sectionBytes = *reinterpret_cast<unsigned int *>(section) - 8;
  unsigned int   numAttached = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned int   highestId = *reinterpret_cast<unsigned int *>(section + 8);
  unsigned char *attachmentData = section + 12;

  modelptr->m_attached.SetCount(numAttached);
  modelptr->m_attachmentFlags.SetCount(numAttached);
  memset(modelptr->m_attachmentFlags.Ptr(), 0, modelptr->m_attachmentFlags.Count());

  shared->attachIdToIndex.SetCount(highestId + 1);
  memset(shared->attachIdToIndex.Ptr(), 0xFF, shared->attachIdToIndex.Count() * sizeof(unsigned int));

  for (unsigned int i = 0; i < numAttached; ++i) {
    unsigned int bytesThisAttachment = *reinterpret_cast<unsigned int *>(attachmentData);
    unsigned int attachmentId = LoadAttachment(attachmentData + 4, flags, &modelptr->m_attached[i], status);
    shared->attachIdToIndex[attachmentId] = i;

    ASSERT(sectionBytes >= bytesThisAttachment);
    sectionBytes -= bytesThisAttachment;
    attachmentData += bytesThisAttachment;
  }

  ASSERT(sectionBytes == 0);
}
