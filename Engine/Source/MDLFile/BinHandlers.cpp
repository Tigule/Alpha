#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Base/MsgBuffer.h"

namespace MDL {

typedef int (__fastcall *BINHANDLER)(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

int __fastcall ReadBinHelpers(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinTextureAnims(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinParticleEmitters(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinAttachments(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinModelGlobals(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinLights(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinCollision(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinBone(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinRibbonEmitters(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinParticleEmitters2(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinGeosetAnim(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinVersion(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinGeosets(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinSequences(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinCameras(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinGlobalSequences(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinMaterials(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinHitTests(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinEventObjects(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinTextures(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int __fastcall ReadBinPivotPoints(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);

int __fastcall WriteBinVersion(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinModelGlobals(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinGlobalSequences(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinMaterials(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinTextures(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinTextureAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinGeosets(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinGeosetAnims(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinBones(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinLights(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinHelpers(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinAttachments(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinPivotPoints(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinParticleEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinParticleEmitters2(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinRibbonEmitters(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinCameras(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinEventObjects(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinHitTests(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
int __fastcall WriteBinCollision(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

static BINHANDLER s_binHandlers[21] = {
    WriteBinVersion, WriteBinModelGlobals, WriteBinSequences, WriteBinGlobalSequences,
    WriteBinMaterials, WriteBinTextures, WriteBinTextureAnims, WriteBinGeosets,
    WriteBinGeosetAnims, WriteBinBones, WriteBinLights, WriteBinHelpers,
    WriteBinAttachments, WriteBinPivotPoints, WriteBinParticleEmitters,
    WriteBinParticleEmitters2, WriteBinRibbonEmitters, WriteBinCameras,
    WriteBinEventObjects, WriteBinHitTests, WriteBinCollision
};

int __fastcall CallBinReadHandler(
    unsigned long sectionTag,
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  switch (sectionTag) {
    case 'PLEH': return ReadBinHelpers(buffer, length, data, status);
    case 'NAXT': return ReadBinTextureAnims(buffer, length, data, status);
    case 'MERP': return ReadBinParticleEmitters(buffer, length, data, status);
    case 'HCTA': return ReadBinAttachments(buffer, length, data, status);
    case 'LDOM': return ReadBinModelGlobals(buffer, length, data, status);
    case 'ETIL': return ReadBinLights(buffer, length, data, status);
    case 'DILC': return ReadBinCollision(buffer, length, data, status);
    case 'ENOB': return ReadBinBone(buffer, length, data, status);
    case 'BBIR': return ReadBinRibbonEmitters(buffer, length, data, status);
    case '2ERP': return ReadBinParticleEmitters2(buffer, length, data, status);
    case 'AOEG': return ReadBinGeosetAnim(buffer, length, data, status);
    case 'SREV': return ReadBinVersion(buffer, length, data, status);
    case 'SOEG': return ReadBinGeosets(buffer, length, data, status);
    case 'SQES': return ReadBinSequences(buffer, length, data, status);
    case 'SMAC': return ReadBinCameras(buffer, length, data, status);
    case 'SBLG': return ReadBinGlobalSequences(buffer, length, data, status);
    case 'SLTM': return ReadBinMaterials(buffer, length, data, status);
    case 'TSTH': return ReadBinHitTests(buffer, length, data, status);
    case 'STVE': return ReadBinEventObjects(buffer, length, data, status);
    case 'SXET': return ReadBinTextures(buffer, length, data, status);
    case 'TVIP': return ReadBinPivotPoints(buffer, length, data, status);
  }

  status->Add(STATUS_WARNING, "Warning: Unknown section tag found. Skipping section.\n");
  buffer.GetData(length);
  return 1;
}

int __fastcall CallBinWriteHandlers(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *status) {
  for (unsigned int i = 0; i < 21; ++i) {
    if (!s_binHandlers[i](data, buffer, status)) {
      return 0;
    }
  }
  return 1;
}

} // namespace MDL
