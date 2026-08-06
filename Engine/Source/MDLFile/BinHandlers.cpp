#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Base/MsgBuffer.h"

namespace MDL {

  typedef int (*BINHANDLER)(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

  int ReadBinHelpers(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinTextureAnims(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinParticleEmitters(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinAttachments(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinModelGlobals(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinLights(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinCollision(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinBone(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinRibbonEmitters(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinParticleEmitters2(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinGeosetAnim(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinVersion(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinGeosets(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinSequences(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinCameras(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinGlobalSequences(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinMaterials(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinHitTests(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinEventObjects(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinTextures(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  int ReadBinPivotPoints(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);

  int WriteBinVersion(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinModelGlobals(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinGlobalSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinMaterials(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinTextures(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinTextureAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinGeosets(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinGeosetAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinBones(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinLights(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinHelpers(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinAttachments(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinPivotPoints(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinParticleEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinParticleEmitters2(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinRibbonEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinCameras(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinEventObjects(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinHitTests(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  int WriteBinCollision(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

  static BINHANDLER s_binHandlers[21] = {WriteBinVersion,        WriteBinModelGlobals, WriteBinSequences,        WriteBinGlobalSequences,
                                         WriteBinMaterials,      WriteBinTextures,     WriteBinTextureAnims,     WriteBinGeosets,
                                         WriteBinGeosetAnims,    WriteBinBones,        WriteBinLights,           WriteBinHelpers,
                                         WriteBinAttachments,    WriteBinPivotPoints,  WriteBinParticleEmitters, WriteBinParticleEmitters2,
                                         WriteBinRibbonEmitters, WriteBinCameras,      WriteBinEventObjects,     WriteBinHitTests,
                                         WriteBinCollision};

  int CallBinReadHandler(DWORD sectionTag, CMsgBuffer &buffer, UINT length, MDLDATA &data, CMDLStatus *status) {
    switch (sectionTag) {
      case 'PLEH':
        return ReadBinHelpers(buffer, length, data, status);
      case 'NAXT':
        return ReadBinTextureAnims(buffer, length, data, status);
      case 'MERP':
        return ReadBinParticleEmitters(buffer, length, data, status);
      case 'HCTA':
        return ReadBinAttachments(buffer, length, data, status);
      case 'LDOM':
        return ReadBinModelGlobals(buffer, length, data, status);
      case 'ETIL':
        return ReadBinLights(buffer, length, data, status);
      case 'DILC':
        return ReadBinCollision(buffer, length, data, status);
      case 'ENOB':
        return ReadBinBone(buffer, length, data, status);
      case 'BBIR':
        return ReadBinRibbonEmitters(buffer, length, data, status);
      case '2ERP':
        return ReadBinParticleEmitters2(buffer, length, data, status);
      case 'AOEG':
        return ReadBinGeosetAnim(buffer, length, data, status);
      case 'SREV':
        return ReadBinVersion(buffer, length, data, status);
      case 'SOEG':
        return ReadBinGeosets(buffer, length, data, status);
      case 'SQES':
        return ReadBinSequences(buffer, length, data, status);
      case 'SMAC':
        return ReadBinCameras(buffer, length, data, status);
      case 'SBLG':
        return ReadBinGlobalSequences(buffer, length, data, status);
      case 'SLTM':
        return ReadBinMaterials(buffer, length, data, status);
      case 'TSTH':
        return ReadBinHitTests(buffer, length, data, status);
      case 'STVE':
        return ReadBinEventObjects(buffer, length, data, status);
      case 'SXET':
        return ReadBinTextures(buffer, length, data, status);
      case 'TVIP':
        return ReadBinPivotPoints(buffer, length, data, status);
    }

    status->Add(STATUS_WARNING, "Warning: Unknown section tag found. Skipping section.\n");
    buffer.GetData(length);
    return 1;
  }

  int CallBinWriteHandlers(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *status) {
    for (UINT i = 0; i < 21; ++i) {
      if (!s_binHandlers[i](data, buffer, status)) {
        return 0;
      }
    }
    return 1;
  }

}  // namespace MDL
