#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Base/MsgBuffer.h"

namespace MDL {

  typedef BOOL (*BINHANDLER)(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

  BOOL ReadBinHelpers(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinTextureAnims(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinParticleEmitters(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinAttachments(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinModelGlobals(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinLights(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinCollision(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinBone(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinRibbonEmitters(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinParticleEmitters2(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinGeosetAnim(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinVersion(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinGeosets(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinSequences(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinCameras(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinGlobalSequences(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinMaterials(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinHitTests(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinEventObjects(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinTextures(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL ReadBinPivotPoints(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);

  BOOL WriteBinVersion(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinModelGlobals(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinGlobalSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinMaterials(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinTextures(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinTextureAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinGeosets(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinGeosetAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinBones(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinLights(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinHelpers(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinAttachments(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinPivotPoints(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinParticleEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinParticleEmitters2(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinRibbonEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinCameras(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinEventObjects(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinHitTests(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL WriteBinCollision(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

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

  BOOL CallBinWriteHandlers(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *status) {
    for (UINT i = 0; i < 21; ++i) {
      if (!s_binHandlers[i](data, buffer, status)) {
        return 0;
      }
    }
    return 1;
  }

}  // namespace MDL
