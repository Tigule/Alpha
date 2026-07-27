#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Base/MsgBuffer.h"

namespace MDL {

typedef int (*BINHANDLER)(const MDLDATA &, CMsgBuffer &, CMDLStatus *);

int ReadBinHelpers(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinTextureAnims(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinParticleEmitters(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinAttachments(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinModelGlobals(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinLights(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinCollision(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinBone(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinRibbonEmitters(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinParticleEmitters2(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinGeosetAnim(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinVersion(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinGeosets(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinSequences(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinCameras(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinGlobalSequences(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinMaterials(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinHitTests(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinEventObjects(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinTextures(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);
int ReadBinPivotPoints(CMsgBuffer &, unsigned int, MDLDATA &, CMDLStatus *);

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

static BINHANDLER s_binHandlers[21] = {
    WriteBinVersion, WriteBinModelGlobals, WriteBinSequences, WriteBinGlobalSequences,
    WriteBinMaterials, WriteBinTextures, WriteBinTextureAnims, WriteBinGeosets,
    WriteBinGeosetAnims, WriteBinBones, WriteBinLights, WriteBinHelpers,
    WriteBinAttachments, WriteBinPivotPoints, WriteBinParticleEmitters,
    WriteBinParticleEmitters2, WriteBinRibbonEmitters, WriteBinCameras,
    WriteBinEventObjects, WriteBinHitTests, WriteBinCollision
};

int CallBinReadHandler(
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

int CallBinWriteHandlers(const MDLDATA &data, CMsgBuffer &buffer, CMDLStatus *status) {
  for (unsigned int i = 0; i < 21; ++i) {
    if (!s_binHandlers[i](data, buffer, status)) {
      return 0;
    }
  }
  return 1;
}

} // namespace MDL
