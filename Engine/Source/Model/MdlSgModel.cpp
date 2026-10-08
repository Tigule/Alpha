#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Base/Status.h"
#include "MDLFile/MDLTypes.h"
#include "Tempest/cmath.h"

#include <storm.h>
#include <string.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

HTEXTURE LoadModelTexture(LPCSTR texturePath, UINT modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);

static void GetTextureFlags(const MDLTEXTURESECTION &texdata, CGxTexFlags *flags) {
  if (texdata.flags & 0x1) {
    flags->m_wrapU = 1;
  }
  if (texdata.flags & 0x2) {
    flags->m_wrapV = 1;
  }
}

static void ProcessTextures(const MDLTEXTURESECTION *texdata, UINT numTextures, UINT flags, CStatus *status, CModelTexture *textures) {
  static NTempest::CImVector s_uglyPink(0xFFFF00FFul);
  for (UINT textureIndex = 0; textureIndex < numTextures; ++textureIndex) {
    textures[textureIndex].replaceableId = texdata[textureIndex].replaceableId;

    if (((flags & 0x800) && texdata[textureIndex].replaceableId) || !static_cast<LPCSTR>(texdata[textureIndex].image)[0]) {
      textures[textureIndex].handle = TextureCreateSolid(s_uglyPink, 0);
      continue;
    }

    EGxTexFilter filter = GxTex_LinearMipNearest;
    UINT         maxAnisotropy = 1;
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

static UINT GetTmuPassFlags(UINT createFlags, UINT layerFlags) {
  UINT result = 0;
  if (layerFlags & 0x2) {
    result = 1;
  }
  if (createFlags & 0x100000) {
    result |= 0x2;
  }
  return result;
}

static EGxTextureShader GetTextureShader(UINT transformId, UINT createFlags) {
  return transformId != static_cast<UINT>(-1) && !(createFlags & 0x100) ? GxTS_Affine : GxTS_PassThru;
}

static void ProcessTexLayers(const MDLMATERIALSECTION &sectionData, CMaterial *unique, CMaterialShared *shared, UINT createFlags, UINT *layerId) {
  UINT numLayers = sectionData.texLayers.Count();
  unique->layers.SetCount(numLayers);
  shared->layers.SetCount(numLayers);

  for (UINT i = 0; i < numLayers; ++i) {
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
    uniqueLayer.vertexFormat = source.coordId == static_cast<UINT>(-1) ? GxVBF_PN : GxVBF_PNT0;

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

static UINT ProcessMaterials(const TSGrowableArray<MDLMATERIALSECTION> &sectionData, UINT createFlags, HMATERIAL *materials) {
  UINT layerId = 0;
  UINT numMaterials = sectionData.Count();

  for (UINT i = 0; i < numMaterials; ++i) {
    CMaterial       *unique = NEWHANDLE(HMATERIAL, CMaterial);
    CMaterialShared *shared = NEWHANDLE(HMATERIALSHARED, CMaterialShared);

    ProcessTexLayers(sectionData[i], unique, shared, createFlags, &layerId);
    unique->data = CREATEHANDLE(HMATERIALSHARED, shared);
    shared->priorityPlane = sectionData[i].priorityPlane;
    materials[i] = CREATEHANDLE(HMATERIAL, unique);
  }

  return layerId;
}

static void ProcessLayerAlpha(const TSGrowableArray<MDLMATERIALSECTION> &sectionData, HMATERIAL const *materials) {
  UINT numMaterials = sectionData.Count();

  for (UINT j = 0; j < numMaterials; ++j) {
    CMaterial *unique = reinterpret_cast<CMaterial *>(materials[j]);
    UINT       numLayers = unique->layers.Count();
    for (UINT i = 0; i < numLayers; ++i) {
      unique->layers[i].layerAlpha = NTempest::CMath::ftol_0_256_(sectionData[j].texLayers[i].staticAlpha * 255.0f);
    }
  }
}

#define NUMPRIMITIVES 10

static EGxPrim s_mdlToGxPrim[NUMPRIMITIVES] = {GxPrim_Points,        GxPrim_Lines,       GxPrims_Last, GxPrim_LineStrip, GxPrim_Triangles,
                                    GxPrim_TriangleStrip, GxPrim_TriangleFan, GxPrims_Last, GxPrims_Last,     GxPrims_Last};

static EGxPrim GetPrimitiveType(BYTE type) {
  ASSERT(type < NUMPRIMITIVES);
  EGxPrim gxPrim = s_mdlToGxPrim[type];
  ASSERT(gxPrim != GxPrims_Last);
  return gxPrim;
}

static int s_multiPrimType[10] = {1, 1, 0, 0, 1, 0, 0, 1, 0, 0};

static UINT CountNumPrimLists(const BYTE *primTypes, UINT numPrimTypes) {
  UINT numPrimLists = 0;
  BYTE lastType = 10;

  for (UINT i = 0; i < numPrimTypes; ++i) {
    if (primTypes[i] != lastType || !s_multiPrimType[primTypes[i]]) {
      ++numPrimLists;
    }
    lastType = primTypes[i];
  }

  return numPrimLists;
}

static void LoadGeosetPrimitiveTypes(const BYTE *primTypes, const UINT *primVertCounts, UINT numPrimTypes, CGeosetShared *geoShared) {
  CPrimitive *primitive = geoShared->primitive.Ptr();
  while (numPrimTypes) {
    --numPrimTypes;
    primitive->type = GetPrimitiveType(*primTypes);
    primitive->vertexCount += *primVertCounts;
    BYTE lastType = *primTypes;
    ++primTypes;
    if (numPrimTypes && (*primTypes != lastType || !s_multiPrimType[*primTypes])) {
      ++primitive;
    }
    ++primVertCounts;
  }
}

static BYTE *LoadGeosetPrimitiveData(BYTE *geosetData, CGeosetShared *geoShared) {
  ASSERT(*((ULONG *) (geosetData)) == 'PYTP');
  geosetData += 4;
  UINT numPrimTypes = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  BYTE *primTypes = geosetData;
  geosetData += numPrimTypes;

  ASSERT(*((ULONG *) (geosetData)) == 'TNCP');
  geosetData += 4;
  UINT numPrimCounts = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  ASSERT(numPrimCounts == numPrimTypes);
  UINT *primVertCounts = reinterpret_cast<UINT *>(geosetData);
  geosetData += numPrimCounts * sizeof(UINT);

  ASSERT(*((ULONG *) (geosetData)) == 'XTVP');
  geosetData += 4;
  UINT numPrimVertices = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geoShared->primitiveVertices.SetCount(numPrimVertices);
  memcpy(geoShared->primitiveVertices.Ptr(), geosetData, numPrimVertices * sizeof(WORD));
  geosetData += numPrimVertices * sizeof(WORD);

  geoShared->primitive.SetCount(CountNumPrimLists(primTypes, numPrimTypes));
  LoadGeosetPrimitiveTypes(primTypes, primVertCounts, numPrimTypes, geoShared);
  return geosetData;
}

static BYTE *LoadGeosetTransformGroups(BYTE *geosetData, UINT loadFlags, CGeosetShared *geoShared) {
  if (loadFlags & 0x100) {
    geoShared->boneWeights.SetCount(1);
    geoShared->groupMatrixCounts.SetCount(1);
    geoShared->matrices.SetCount(1);
    geoShared->boneWeights[0] = 0;
    geoShared->groupMatrixCounts[0] = 1;
    geoShared->matrices[0] = 0;

    geoShared->hwBoneWeights.SetCount(geoShared->position.Count());
    geoShared->hwBoneIndices.SetCount(geoShared->position.Count());
    geoShared->hwBoneIndices.Zero();
    UINT numVertices = geoShared->position.Count();
    for (UINT i = 0; i < numVertices; ++i) {
      geoShared->hwBoneWeights.Ptr()[i] = 0xFF000000;
    }

    geosetData += 4;
    geosetData += *reinterpret_cast<UINT *>(geosetData) + 4;
    for (UINT section = 0; section < 4; ++section) {
      geosetData += 4;
      geosetData += *reinterpret_cast<UINT *>(geosetData) * sizeof(UINT) + 4;
    }
    return geosetData;
  }

  ASSERT(*((ULONG *) (geosetData)) == 'XDNG');
  geosetData += 4;
  UINT numBoneWeights = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geoShared->boneWeights.Set(numBoneWeights, geosetData);
  geosetData += numBoneWeights;

  ASSERT(*((ULONG *) (geosetData)) == 'CGTM');
  geosetData += 4;
  geoShared->groupMatrixCounts.SetCount(*reinterpret_cast<UINT *>(geosetData));
  geosetData += 4;
  memcpy(geoShared->groupMatrixCounts.Ptr(), geosetData, geoShared->groupMatrixCounts.Count() * sizeof(UINT));
  geosetData += geoShared->groupMatrixCounts.Count() * sizeof(UINT);

  ASSERT(*((ULONG *) (geosetData)) == 'STAM');
  geosetData += 4;
  geoShared->matrices.SetCount(*reinterpret_cast<UINT *>(geosetData));
  geosetData += 4;
  memcpy(geoShared->matrices.Ptr(), geosetData, geoShared->matrices.Count() * sizeof(UINT));
  geosetData += geoShared->matrices.Count() * sizeof(UINT);

  ASSERT(*((ULONG *) (geosetData)) == 'XDIB');
  geosetData += 4;
  geoShared->hwBoneIndices.SetCount(*reinterpret_cast<UINT *>(geosetData));
  geosetData += 4;
  memcpy(geoShared->hwBoneIndices.Ptr(), geosetData, geoShared->hwBoneIndices.Count() * sizeof(UINT));
  geosetData += geoShared->hwBoneIndices.Count() * sizeof(UINT);

  ASSERT(*((ULONG *) (geosetData)) == 'TGWB');
  geosetData += 4;
  geoShared->hwBoneWeights.SetCount(*reinterpret_cast<UINT *>(geosetData));
  geosetData += 4;
  memcpy(geoShared->hwBoneWeights.Ptr(), geosetData, geoShared->hwBoneWeights.Count() * sizeof(UINT));
  geosetData += geoShared->hwBoneWeights.Count() * sizeof(UINT);
  return geosetData;
}

static void LoadGeosetData(BYTE *geosetData, UINT bytesLeft, UINT loadFlags, UINT geosetId, CGeosetShared *geoShared) {
  BYTE *sectionDone = geosetData + bytesLeft;
  ASSERT(*((ULONG *) (geosetData)) == 'XTRV');
  geosetData += 4;
  UINT numVertices = *reinterpret_cast<UINT *>(geosetData);
  ASSERT(numVertices <= 0xffff);
  geosetData += 4;
  geoShared->position.SetCount(numVertices);
  memcpy(geoShared->position.Ptr(), geosetData, numVertices * sizeof(NTempest::C3Vector));
  geosetData += geoShared->position.Count() * sizeof(NTempest::C3Vector);

  ASSERT(*((ULONG *) (geosetData)) == 'SMRN');
  geosetData += 4;
  UINT numNormals = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  ASSERT(numVertices == numNormals);
  geoShared->normal.SetCount(numNormals);
  memcpy(geoShared->normal.Ptr(), geosetData, numNormals * sizeof(NTempest::C3Vector));
  geosetData += geoShared->normal.Count() * sizeof(NTempest::C3Vector);

  if (*((ULONG *) (geosetData)) == 'SAVU') {
    geosetData += 4;
    UINT numMappingChannels = *reinterpret_cast<UINT *>(geosetData);
    geosetData += 4;
    geoShared->texCoord.SetCount(numMappingChannels);
    for (UINT i = 0; i < numMappingChannels; ++i) {
      geoShared->texCoord[i].SetCount(numVertices);
      memcpy(geoShared->texCoord[i].Ptr(), geosetData, numVertices * sizeof(NTempest::C2Vector));
      geosetData += numVertices * sizeof(NTempest::C2Vector);
    }
  }

  geosetData = LoadGeosetPrimitiveData(geosetData, geoShared);
  geosetData = LoadGeosetTransformGroups(geosetData, loadFlags, geoShared);

  geoShared->materialId = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geoShared->selectionGroup = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geoShared->flags = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geoShared->radius = *reinterpret_cast<float *>(geosetData);
  geosetData += 4;

  const NTempest::C3Vector &minimum = *reinterpret_cast<const NTempest::C3Vector *>(geosetData);
  const NTempest::C3Vector &maximum = *reinterpret_cast<const NTempest::C3Vector *>(geosetData + 12);
  geosetData += 24;
  geoShared->centroid = (minimum + maximum) * 0.5f;
  geoShared->geosetId = geosetId;

  UINT numSequenceBounds = *reinterpret_cast<UINT *>(geosetData);
  geosetData += 4;
  geosetData += numSequenceBounds * 7 * sizeof(UINT);
  ASSERT(geosetData == sectionDone);

  geoShared->vertexShader = geoShared->groupMatrixCounts.Count() > 1 ? GxVS_Skin : GxVS_PassThru;
}

static void CreateGeoset(const MDLGEOSETSECTION &geosetdata, UINT geosetId, UINT loadFlags, CGeosetShared *geoShared) {
  if (geosetdata.vertices.Count() > 0xFFFF) {
    return;
  }

  geoShared->position = geosetdata.vertices;
  geoShared->normal = geosetdata.normals;

  geoShared->texCoord.SetCount(geosetdata.texCoords.Count());
  for (UINT i = 0; i < geoShared->texCoord.Count(); ++i) {
    geoShared->texCoord[i] = geosetdata.texCoords[i];
  }

  UINT numPrimTypes = geosetdata.primitives.types.Count();
  UINT numPrimLists = CountNumPrimLists(geosetdata.primitives.types.Ptr(), numPrimTypes);
  geoShared->primitive.SetCount(numPrimLists);

  ASSERT(numPrimTypes == geosetdata.primitives.counts.Count());
  LoadGeosetPrimitiveTypes(geosetdata.primitives.types.Ptr(), geosetdata.primitives.counts.Ptr(), numPrimTypes, geoShared);
  geoShared->primitiveVertices = geosetdata.primitives.vertices;

  if (loadFlags & 0x100) {
    geoShared->groupMatrixCounts.SetCount(1);
    geoShared->matrices.SetCount(1);
    geoShared->boneWeights.SetCount(1);
    geoShared->groupMatrixCounts[0] = 1;
    geoShared->matrices[0] = 0;
    geoShared->boneWeights[0] = 0;

    geoShared->hwBoneWeights.SetCount(geoShared->position.Count());
    geoShared->hwBoneIndices.SetCount(geoShared->position.Count());
    geoShared->hwBoneIndices.Zero();
    UINT numVertices = geoShared->position.Count();
    for (UINT i = 0; i < numVertices; ++i) {
      geoShared->hwBoneWeights.Ptr()[i] = 0xFF000000;
    }
  } else {
    geoShared->groupMatrixCounts = geosetdata.groupMatrixCounts;
    geoShared->matrices = geosetdata.matrices;
    geoShared->boneWeights = geosetdata.vertGroupIndices;
    geoShared->hwBoneWeights = geosetdata.boneWeights;
    geoShared->hwBoneIndices = geosetdata.boneIndices;
  }

  geoShared->materialId = geosetdata.materialId;
  geoShared->selectionGroup = geosetdata.selectionGroup;
  geoShared->geosetId = geosetId;
  geoShared->centroid = (geosetdata.bounds.extent.b + geosetdata.bounds.extent.t) * 0.5f;
  geoShared->radius = geosetdata.bounds.radius;
  geoShared->flags = geosetdata.flags;
  geoShared->vertexShader = geoShared->groupMatrixCounts.Count() > 1 ? GxVS_Skin : GxVS_PassThru;
}

static void CreateGeosetWithNormals(const MDLGEOSETSECTION &geoset, UINT geosetId, UINT loadFlags, CGeosetShared *geoShared) {
  UINT numNormals = geoset.normals.Count();
  if (!(loadFlags & 0x1) || !numNormals) {
    CreateGeoset(geoset, geosetId, loadFlags, geoShared);
    return;
  }

  MDLGEOSETSECTION normalLines(geoset);
  normalLines.vertices.SetCount(numNormals * 2);
  normalLines.normals.SetCount(numNormals * 2);
  normalLines.vertGroupIndices.SetCount(numNormals * 2);

  UINT numTexCoords = normalLines.texCoords.Count();
  UINT i;
  for (i = 0; i < numTexCoords; ++i) {
    normalLines.texCoords[i].GrowToFit(numNormals * 2 - 1, 1);
  }

  UINT numPrimVertices = normalLines.primitives.vertices.Count();
  normalLines.primitives.SetCount(normalLines.primitives.types.Count() + 1, numPrimVertices + numNormals * 2);
  *normalLines.primitives.types.Top() = 1;
  *normalLines.primitives.counts.Top() = numNormals * 2;

  for (i = 0; i < numNormals; ++i) {
    normalLines.primitives.vertices[numPrimVertices + i * 2] = static_cast<WORD>(i);
    normalLines.primitives.vertices[numPrimVertices + i * 2 + 1] = static_cast<WORD>(numNormals + i);
  }

  NTempest::C3Vector *normVert = &normalLines.vertices[numNormals];
  BYTE               *normGroupId = &normalLines.vertGroupIndices[numNormals];
  UINT                numVertices = geoset.vertices.Count();
  for (i = 0; i < numVertices; ++i) {
    normVert[i] = geoset.vertices[i] + 0.12f * geoset.normals[i];
    normGroupId[i] = geoset.vertGroupIndices[i];
  }

  CreateGeoset(normalLines, geosetId, loadFlags, geoShared);
}

static void BuildAllGeosets(const MDLDATA &data, CGeosetShared *geosets, CGeosetColor *geosetColor, UINT loadFlags) {
  UINT numGeosets = data.geosets.Count();
  UINT i;
  for (i = 0; i < numGeosets; ++i) {
    CreateGeosetWithNormals(data.geosets[i], i, loadFlags, &geosets[i]);
  }

  UINT numGeosetAnims = data.geosetAnims.Count();
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
    UINT                                         loadFlags,
    CStatus                                     *status
) {
  UINT numAttachments = attachments.Count();
  if (!numAttachments) {
    return;
  }

  ASSERT(modelptr);
  modelptr->m_attached.SetCount(numAttachments);
  modelptr->m_attachmentFlags.SetCount(numAttachments);
  memset(modelptr->m_attachmentFlags.Ptr(), 0, numAttachments);

  UINT highestId = attachments.Top()->attachmentId;
  shared->attachIdToIndex.SetCount(highestId + 1);
  memset(shared->attachIdToIndex.Ptr(), 0xFF, shared->attachIdToIndex.Count() * sizeof(UINT));

  CStatus      subStatus;
  LISTPTR(LINKUNIQUE) instance = modelptr->m_attached.Ptr();
  for (UINT i = 0; i < numAttachments; ++i, ++instance) {
    UINT attachmentId = attachments.Ptr()[i].attachmentId;
    shared->attachIdToIndex[attachmentId] = i;
    if (!SStrLen(attachments.Ptr()[i].path)) {
      continue;
    }

    CModelCreate createData;
    createData.flags = loadFlags;
    HMODEL child = ModelCreate(attachments.Ptr()[i].path, &createData, &subStatus);
    if (child) {
      LINKUNIQUE *link = instance->NewNode(LIST_TAIL, 0, 8);
      link->child = child;
    }

    if (!subStatus.IsEmpty()) {
      status->Add(subStatus.GetHighestSeverity(), "%s:\n", static_cast<LPCSTR>(attachments.Ptr()[i].path));
      status->Add(subStatus);
      subStatus.Clear();
    }
  }
}

static void LoadLayerData(BYTE *materialData, CTexLayer *unique, CTexLayerShared *shared, UINT createFlags) {
  EGxBlend blendMode;
  switch (*reinterpret_cast<UINT *>(materialData)) {
    case TEXOP_TRANSPARENT:
      blendMode = GxBlend_AlphaKey;
      break;
    case TEXOP_BLEND:
      blendMode = GxBlend_Alpha;
      break;
    case TEXOP_ADD:
    case TEXOP_ADD_ALPHA:
      blendMode = GxBlend_Add;
      break;
    case TEXOP_MODULATE:
      blendMode = GxBlend_Mod;
      break;
    case TEXOP_MODULATE2X:
      blendMode = GxBlend_Mod2x;
      break;
    case TEXOP_LOAD:
      blendMode = GxBlend_Opaque;
      break;
    default:
      blendMode = GxBlend_Opaque;
      break;
  }
  shared->blendMode = blendMode;
  materialData += 4;
  unique->blendMode = blendMode;

  UINT layerFlags = *reinterpret_cast<UINT *>(materialData);
  shared->tmuPass[0].flags = GetTmuPassFlags(createFlags, layerFlags);
  materialData += 4;
  unique->tmuPass[0].textureId = *reinterpret_cast<UINT *>(materialData);
  materialData += 4;
  shared->tmuPass[0].transformId = *reinterpret_cast<UINT *>(materialData);
  materialData += 4;
  shared->tmuPass[0].coordId = *reinterpret_cast<UINT *>(materialData);
  materialData += 4;

  unique->layerAlpha = NTempest::CMath::ftol_0_256_(*reinterpret_cast<float *>(materialData) * 255.0f);
  shared->tmuPass[0].textureShader = GetTextureShader(shared->tmuPass[0].transformId, createFlags);

  unique->tmuPass[0].combiner = GxTexBlend_Mod;
  unique->vertexFormat = shared->tmuPass[0].coordId == static_cast<UINT>(-1) ? GxVBF_PN : GxVBF_PNT0;

  if (static_cast<UINT>(unique->blendMode) >= GxBlend_Alpha && static_cast<UINT>(unique->blendMode) <= GxBlend_ModAdd) {
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

static HMATERIAL LoadMaterialData(BYTE *materialData, UINT createFlags, BYTE *totalLayers) {
  CMaterial       *unique = NEWHANDLE(HMATERIAL, CMaterial);
  CMaterialShared *shared = NEWHANDLE(HMATERIALSHARED, CMaterialShared);

  shared->priorityPlane = *reinterpret_cast<UINT *>(materialData);
  materialData += 4;
  UINT numLayers = *reinterpret_cast<UINT *>(materialData);
  materialData += 4;
  *totalLayers += static_cast<BYTE>(numLayers);

  unique->layers.SetCount(numLayers);
  shared->layers.SetCount(numLayers);
  for (UINT i = 0; i < numLayers; ++i) {
    UINT bytesThisLayer = *reinterpret_cast<UINT *>(materialData);
    LoadLayerData(materialData + 4, &unique->layers[i], &shared->layers[i], createFlags);
    materialData += bytesThisLayer;
  }

  unique->data = CREATEHANDLE(HMATERIALSHARED, shared);
  return CREATEHANDLE(HMATERIAL, unique);
}

static UINT LoadAttachment(BYTE *data, UINT loadFlags, LISTPTR(LINKUNIQUE) attachment, CStatus *status) {
  UINT  dataOffset = *reinterpret_cast<UINT *>(data);
  BYTE *attachmentData = data + dataOffset;
  UINT  attachmentId = *reinterpret_cast<UINT *>(attachmentData);
  if (!SStrLen(reinterpret_cast<LPCSTR>(attachmentData + 5))) {
    return attachmentId;
  }

  CModelCreate createData;
  createData.flags = loadFlags;

  CStatus subStatus;
  HMODEL  child = ModelCreate(reinterpret_cast<LPCSTR>(attachmentData + 5), &createData, &subStatus);
  if (child) {
    LINKUNIQUE *link = attachment->NewNode(LIST_TAIL, 0, 8);
    link->child = child;
  }

  if (!subStatus.IsEmpty()) {
    status->Add(subStatus.GetHighestSeverity(), "%s:\n", reinterpret_cast<LPCSTR>(attachmentData + 5));
    status->Add(subStatus);
  }

  return attachmentId;
}

void MdlReadLoadGlobalProperties(const MDLDATA &data, CModelShared *shared, UINT *loadFlags) {
  ASSERT(shared);
  shared->groundTrack = static_cast<GROUND_TRACK>(data.model.flags & GROUND_TRACK_MASK);
  if (data.model.flags & 0x4) {
    *loadFlags &= ~0x100;
  }
}

BOOL MdlReadLoadModel(const MDLDATA &data, CModelComplex *modelptr, CModelShared *shared, UINT flags, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  UINT numTextures = data.textures.Count();
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(data.textures.Ptr(), numTextures, flags, status, modelptr->m_textures.Ptr());

  UINT numMaterials = data.materials.Count();
  modelptr->m_materials.SetCount(numMaterials);
  UINT numLayers = ProcessMaterials(data.materials, flags, modelptr->m_materials.Ptr());
  ProcessLayerAlpha(data.materials, modelptr->m_materials.Ptr());

  UINT numGeosets = data.geosets.Count();
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);
  BuildAllGeosets(data, shared->geosets.Ptr(), modelptr->m_geosetColor.Ptr(), flags);
  ProcessAttachments(data.attachments, modelptr, shared, flags, status);

  ASSERT(numGeosets <= 0xff);
  ASSERT(numLayers <= 0xff);
  shared->numGeosets = static_cast<BYTE>(numGeosets);
  shared->numLayers = static_cast<BYTE>(numLayers);
  return 1;
}

BOOL MdlReadLoadModel(const MDLDATA &data, CModelSimple *modelptr, CModelShared *shared, UINT flags, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  UINT numTextures = data.textures.Count();
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(data.textures.Ptr(), numTextures, flags, status, modelptr->m_textures.Ptr());

  UINT numMaterials = data.materials.Count();
  modelptr->m_materials.SetCount(numMaterials);
  UINT numLayers = ProcessMaterials(data.materials, flags, modelptr->m_materials.Ptr());
  ProcessLayerAlpha(data.materials, modelptr->m_materials.Ptr());

  UINT numGeosets = data.geosets.Count();
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);
  BuildAllGeosets(data, shared->geosets.Ptr(), modelptr->m_geosetColor.Ptr(), flags);

  ASSERT(numGeosets <= 0xff);
  ASSERT(numLayers <= 0xff);
  shared->numGeosets = static_cast<BYTE>(numGeosets);
  shared->numLayers = static_cast<BYTE>(numLayers);
  return 1;
}

void MdxLoadGlobalProperties(BYTE *data, UINT fileBytes, UINT *loadFlags, CModelShared *modelShared) {
  ASSERT(modelShared);
  ASSERT(loadFlags);
  data = MDLFileBinarySeek(data, fileBytes, 'LDOM');
  ASSERT(data != 0);

  data += 0x174;
  modelShared->groundTrack = static_cast<GROUND_TRACK>(*data & GROUND_TRACK_MASK);
  if (*data & 0x4) {
    *loadFlags &= ~0x100;
  }
}

void MdxReadTextures(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  data = MDLFileBinarySeek(data, fileBytes, 'SXET');
  if (!data) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(data);
  data += 4;
  UINT numTextures = sectionBytes / sizeof(MDLTEXTURESECTION);
  ASSERT(sectionBytes == (numTextures * sizeof(MDLTEXTURESECTION)));
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(reinterpret_cast<MDLTEXTURESECTION *>(data), numTextures, flags, status, modelptr->m_textures.Ptr());
}

void MdxReadTextures(BYTE *data, UINT fileBytes, UINT flags, CModelSimple *modelptr, CStatus *status) {
  ASSERT(data);
  ASSERT(modelptr);
  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'SXET');
  if (!section) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(section);
  section += 4;
  UINT numTextures = sectionBytes / sizeof(MDLTEXTURESECTION);
  ASSERT(sectionBytes == (numTextures * sizeof(MDLTEXTURESECTION)));
  modelptr->m_textures.SetCount(numTextures);
  ProcessTextures(reinterpret_cast<MDLTEXTURESECTION *>(section), numTextures, flags, status, modelptr->m_textures.Ptr());
}

void MdxReadMaterials(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CModelShared *shared) {
  data = MDLFileBinarySeek(data, fileBytes, 'SLTM');
  if (!data) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(data);
  data += 4;
  BYTE *dataDone = data + sectionBytes;
  UINT  numMaterials = *reinterpret_cast<UINT *>(data);
  data += 4;
  data += 4;
  shared->numLayers = 0;
  modelptr->m_materials.SetCount(numMaterials);
  for (UINT i = 0; i < numMaterials; ++i) {
    UINT bytesThisMaterial = *reinterpret_cast<UINT *>(data);
    modelptr->m_materials[i] = LoadMaterialData(data + 4, flags, &shared->numLayers);
    data += bytesThisMaterial;
    ASSERT(data <= dataDone);
  }
  ASSERT(data == dataDone);
}

void MdxReadMaterials(BYTE *data, UINT fileBytes, UINT flags, CModelSimple *modelptr, CModelShared *shared) {
  data = MDLFileBinarySeek(data, fileBytes, 'SLTM');
  if (!data) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(data);
  data += 4;
  BYTE *dataDone = data + sectionBytes;
  UINT  numMaterials = *reinterpret_cast<UINT *>(data);
  data += 4;
  data += 4;
  shared->numLayers = 0;
  modelptr->m_materials.SetCount(numMaterials);
  for (UINT i = 0; i < numMaterials; ++i) {
    UINT bytesThisMaterial = *reinterpret_cast<UINT *>(data);
    modelptr->m_materials[i] = LoadMaterialData(data + 4, flags, &shared->numLayers);
    data += bytesThisMaterial;
    ASSERT(data <= dataDone);
  }
  ASSERT(data == dataDone);
}

void MdxReadGeosets(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);
  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'SOEG');
  if (!section) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(section);
  section += 4;
  BYTE *dataDone = section + sectionBytes;
  UINT  numGeosets = *reinterpret_cast<UINT *>(section);
  section += 4;
  ASSERT(numGeosets <= 0xff);
  shared->numGeosets = static_cast<BYTE>(numGeosets);
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);

  UINT i;
  for (i = 0; i < numGeosets; ++i) {
    UINT bytesThisGeo = *reinterpret_cast<UINT *>(section);
    ASSERT(dataDone >= (section + bytesThisGeo));
    LoadGeosetData(section + 4, bytesThisGeo - 4, flags, i, &shared->geosets[i]);
    section += bytesThisGeo;
  }
  ASSERT(section == dataDone);

  section = MDLFileBinarySeek(section, fileBytes - (section - data), 'AOEG');
  if (!section) {
    return;
  }

  sectionBytes = *reinterpret_cast<UINT *>(section) - 4;
  section += 4;
  UINT numGeosetAnims = *reinterpret_cast<UINT *>(section);
  section += 4;
  for (i = 0; i < numGeosetAnims; ++i) {
    BYTE *animData = section;
    UINT  bytesThisAnim = *reinterpret_cast<UINT *>(animData);
    animData += 4;
    UINT geosetId = *reinterpret_cast<UINT *>(animData);
    animData += 4;
    CGeosetColor &color = modelptr->m_geosetColor[geosetId];
    color.animatedAlpha = *reinterpret_cast<float *>(animData);
    animData += 4;
    color.animatedColor.a = NTempest::CMath::ftol_0_256_(color.animatedAlpha * 255.0f);
    color.animatedColor.r = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[0] * 255.0f);
    color.animatedColor.g = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[1] * 255.0f);
    color.animatedColor.b = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[2] * 255.0f);
    section += bytesThisAnim;
    ASSERT(sectionBytes >= bytesThisAnim);
    sectionBytes -= bytesThisAnim;
  }
  ASSERT(sectionBytes == 0);
}

void MdxReadGeosets(BYTE *data, UINT fileBytes, UINT flags, CModelSimple *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);
  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'SOEG');
  if (!section) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(section);
  section += 4;
  BYTE *dataDone = section + sectionBytes;
  UINT  numGeosets = *reinterpret_cast<UINT *>(section);
  section += 4;
  ASSERT(numGeosets <= 0xff);
  shared->numGeosets = static_cast<BYTE>(numGeosets);
  modelptr->m_geosets.SetCount(numGeosets);
  shared->geosets.SetCount(numGeosets);
  modelptr->m_geosetColor.SetCount(numGeosets);

  UINT i;
  for (i = 0; i < numGeosets; ++i) {
    UINT bytesThisGeo = *reinterpret_cast<UINT *>(section);
    ASSERT(dataDone >= (section + bytesThisGeo));
    LoadGeosetData(section + 4, bytesThisGeo - 4, flags, i, &shared->geosets[i]);
    section += bytesThisGeo;
  }
  ASSERT(section == dataDone);

  section = MDLFileBinarySeek(section, fileBytes - (section - data), 'AOEG');
  if (!section) {
    return;
  }

  sectionBytes = *reinterpret_cast<UINT *>(section) - 4;
  section += 4;
  UINT numGeosetAnims = *reinterpret_cast<UINT *>(section);
  section += 4;
  for (i = 0; i < numGeosetAnims; ++i) {
    BYTE *animData = section;
    UINT  bytesThisAnim = *reinterpret_cast<UINT *>(animData);
    animData += 4;
    UINT geosetId = *reinterpret_cast<UINT *>(animData);
    animData += 4;
    CGeosetColor &color = modelptr->m_geosetColor[geosetId];
    color.animatedAlpha = *reinterpret_cast<float *>(animData);
    animData += 4;
    color.animatedColor.a = NTempest::CMath::ftol_0_256_(color.animatedAlpha * 255.0f);
    color.animatedColor.r = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[0] * 255.0f);
    color.animatedColor.g = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[1] * 255.0f);
    color.animatedColor.b = NTempest::CMath::ftol_0_256_(reinterpret_cast<float *>(animData)[2] * 255.0f);
    section += bytesThisAnim;
    ASSERT(sectionBytes >= bytesThisAnim);
    sectionBytes -= bytesThisAnim;
  }
  ASSERT(sectionBytes == 0);
}

void MdxReadAttachments(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(shared);

  data = MDLFileBinarySeek(data, fileBytes, 'HCTA');
  if (!data) {
    return;
  }

  UINT sectionBytes = *reinterpret_cast<UINT *>(data) - 8;
  UINT numAttached = *reinterpret_cast<UINT *>(data + 4);
  UINT highestId = *reinterpret_cast<UINT *>(data + 8);
  data += 12;

  modelptr->m_attached.SetCount(numAttached);
  modelptr->m_attachmentFlags.SetCount(numAttached);
  memset(modelptr->m_attachmentFlags.Ptr(), 0, modelptr->m_attachmentFlags.Count());

  shared->attachIdToIndex.SetCount(highestId + 1);
  memset(shared->attachIdToIndex.Ptr(), 0xFF, shared->attachIdToIndex.Count() * sizeof(UINT));

  for (UINT i = 0; i < numAttached; ++i) {
    UINT bytesThisAttachment = *reinterpret_cast<UINT *>(data);
    UINT attachmentId = LoadAttachment(data + 4, flags, &modelptr->m_attached[i], status);
    shared->attachIdToIndex[attachmentId] = i;

    ASSERT(sectionBytes >= bytesThisAttachment);
    sectionBytes -= bytesThisAttachment;
    data += bytesThisAttachment;
  }

  ASSERT(sectionBytes == 0);
}
