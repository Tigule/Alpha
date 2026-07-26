#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "lex.h"

#include <stpl.h>
#include <stdio.h>
#include <stdarg.h>

namespace MDL {

typedef int (__fastcall *TEXTHANDLER)(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

int __fastcall ReadVersion(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadModelGlobals(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadSequences(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadGlobalSequences(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadTextureAnims(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadTextures(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadMaterials(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadGeoset(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadGeosetAnim(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadBone(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadLight(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadHelper(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadAttachment(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadPivotPoints(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadParticleEmitter(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadParticleEmitter2(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadCamera(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadEventObject(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadHitTest(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadRibbonEmitter(Parser &, MDLDATA &, CMDLStatus *);
int __fastcall ReadCollision(Parser &, MDLDATA &, CMDLStatus *);

int __fastcall WriteHeaderComment(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteVersion(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteModelGlobals(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteGlobalSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteTextures(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteMaterials(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteTextureAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteGeosets(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteGeosetAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteBones(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteLights(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteHelpers(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteAttachments(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WritePivotPoints(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteParticleEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteParticleEmitters2(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteRibbonEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteCameras(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteEventObjects(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteHitTests(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
int __fastcall WriteCollision(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

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

int __fastcall CallTextReadHandler(
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

int __fastcall CallTextWriteHandlers(
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
