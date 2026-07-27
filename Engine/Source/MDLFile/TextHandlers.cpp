#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "lex.h"

#include <stpl.h>
#include <stdio.h>
#include <stdarg.h>

namespace MDL {

typedef int (*TEXTHANDLER)(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

int ReadVersion(Parser &, MDLDATA &, CMDLStatus *);
int ReadModelGlobals(Parser &, MDLDATA &, CMDLStatus *);
int ReadSequences(Parser &, MDLDATA &, CMDLStatus *);
int ReadGlobalSequences(Parser &, MDLDATA &, CMDLStatus *);
int ReadTextureAnims(Parser &, MDLDATA &, CMDLStatus *);
int ReadTextures(Parser &, MDLDATA &, CMDLStatus *);
int ReadMaterials(Parser &, MDLDATA &, CMDLStatus *);
int ReadGeoset(Parser &, MDLDATA &, CMDLStatus *);
int ReadGeosetAnim(Parser &, MDLDATA &, CMDLStatus *);
int ReadBone(Parser &, MDLDATA &, CMDLStatus *);
int ReadLight(Parser &, MDLDATA &, CMDLStatus *);
int ReadHelper(Parser &, MDLDATA &, CMDLStatus *);
int ReadAttachment(Parser &, MDLDATA &, CMDLStatus *);
int ReadPivotPoints(Parser &, MDLDATA &, CMDLStatus *);
int ReadParticleEmitter(Parser &, MDLDATA &, CMDLStatus *);
int ReadParticleEmitter2(Parser &, MDLDATA &, CMDLStatus *);
int ReadCamera(Parser &, MDLDATA &, CMDLStatus *);
int ReadEventObject(Parser &, MDLDATA &, CMDLStatus *);
int ReadHitTest(Parser &, MDLDATA &, CMDLStatus *);
int ReadRibbonEmitter(Parser &, MDLDATA &, CMDLStatus *);
int ReadCollision(Parser &, MDLDATA &, CMDLStatus *);

int WriteHeaderComment(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteVersion(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteModelGlobals(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteGlobalSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteTextures(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteMaterials(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteTextureAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteGeosets(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteGeosetAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteBones(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteLights(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteHelpers(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteAttachments(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WritePivotPoints(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteParticleEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteParticleEmitters2(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteRibbonEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteCameras(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteEventObjects(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteHitTests(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int WriteCollision(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

static TEXTHANDLER s_handlers[22] = {
    WriteHeaderComment, WriteVersion, WriteModelGlobals, WriteSequences,
    WriteGlobalSequences, WriteTextures, WriteMaterials, WriteTextureAnims,
    WriteGeosets, WriteGeosetAnims, WriteBones, WriteLights, WriteHelpers,
    WriteAttachments, WritePivotPoints, WriteParticleEmitters,
    WriteParticleEmitters2, WriteRibbonEmitters, WriteCameras,
    WriteEventObjects, WriteHitTests, WriteCollision
};

void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...) {
  static char line[1024];
  va_list args;
  va_start(args, format);
  int count = _vsnprintf(line, sizeof(line), format, args);
  va_end(args);

  if (count == sizeof(line)) {
    count = sizeof(line) - 1;
    line[count] = 0;
  } else if (count <= 0) {
    return;
  }
  buffer.Add(count, line);
}

int CallTextReadHandler(
    unsigned int token,
    mdl_scan &scanner,
    MDLDATA &data,
    CMDLStatus *status
) {
  Parser parse(status, scanner);
  switch (token) {
    case 0x103: return ReadVersion(parse, data, status);
    case 0x104: return ReadModelGlobals(parse, data, status);
    case 0x105: return ReadSequences(parse, data, status);
    case 0x106: return ReadGlobalSequences(parse, data, status);
    case 0x107: return ReadTextureAnims(parse, data, status);
    case 0x108: return ReadTextures(parse, data, status);
    case 0x109: return ReadMaterials(parse, data, status);
    case 0x10A: return ReadGeoset(parse, data, status);
    case 0x10B: return ReadGeosetAnim(parse, data, status);
    case 0x10C: return ReadBone(parse, data, status);
    case 0x10E: return ReadLight(parse, data, status);
    case 0x10F: return ReadHelper(parse, data, status);
    case 0x110: return ReadAttachment(parse, data, status);
    case 0x111: return ReadPivotPoints(parse, data, status);
    case 0x112: return ReadParticleEmitter(parse, data, status);
    case 0x113: return ReadParticleEmitter2(parse, data, status);
    case 0x114: return ReadCamera(parse, data, status);
    case 0x115: return ReadEventObject(parse, data, status);
    case 0x116:
    case 0x117: return ReadHitTest(parse, data, status);
    case 0x118: return ReadRibbonEmitter(parse, data, status);
    case 0x119: return ReadCollision(parse, data, status);
  }
  parse.FatalUnexpected(scanner.mdltext);
  return 0;
}

int CallTextWriteHandlers(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *status
) {
  for (unsigned int i = 0; i < 22; ++i) {
    if (!s_handlers[i](data, buffer, status)) {
      return 0;
    }
  }
  return 1;
}

} // namespace MDL
