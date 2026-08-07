#include "MDLTypes.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "lex.h"

#include <stpl.h>
#include <stdio.h>
#include <stdarg.h>

namespace MDL {

  typedef BOOL (*TEXTHANDLER)(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

  BOOL ReadVersion(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadModelGlobals(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadSequences(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadGlobalSequences(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadTextureAnims(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadTextures(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadMaterials(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadGeoset(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadGeosetAnim(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadBone(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadLight(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadHelper(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadAttachment(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadPivotPoints(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadParticleEmitter(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadParticleEmitter2(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadCamera(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadEventObject(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadHitTest(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadRibbonEmitter(Parser &, MDLDATA &, CMDLStatus *);
  BOOL ReadCollision(Parser &, MDLDATA &, CMDLStatus *);

  BOOL WriteHeaderComment(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteVersion(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteModelGlobals(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteGlobalSequences(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteTextures(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteMaterials(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteTextureAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteGeosets(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteGeosetAnims(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteBones(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteLights(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteHelpers(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteAttachments(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WritePivotPoints(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteParticleEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteParticleEmitters2(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteRibbonEmitters(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteCameras(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteEventObjects(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteHitTests(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL WriteCollision(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);

  static TEXTHANDLER s_handlers[22] = {WriteHeaderComment,     WriteVersion,        WriteModelGlobals, WriteSequences,
                                       WriteGlobalSequences,   WriteTextures,       WriteMaterials,    WriteTextureAnims,
                                       WriteGeosets,           WriteGeosetAnims,    WriteBones,        WriteLights,
                                       WriteHelpers,           WriteAttachments,    WritePivotPoints,  WriteParticleEmitters,
                                       WriteParticleEmitters2, WriteRibbonEmitters, WriteCameras,      WriteEventObjects,
                                       WriteHitTests,          WriteCollision};

  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...) {
    static char line[1024];
    va_list     args;
    va_start(args, format);
    int numchars = _vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    if (numchars == sizeof(line)) {
      numchars = sizeof(line) - 1;
      line[numchars] = 0;
    } else if (numchars <= 0) {
      return;
    }
    buffer.Add(numchars, line);
  }

  int CallTextReadHandler(UINT token, mdl_scan &scanner, MDLDATA &data, CMDLStatus *status) {
    Parser parse(status, scanner);
    switch (token) {
      case 0x103:
        return ReadVersion(parse, data, status);
      case 0x104:
        return ReadModelGlobals(parse, data, status);
      case 0x105:
        return ReadSequences(parse, data, status);
      case 0x106:
        return ReadGlobalSequences(parse, data, status);
      case 0x107:
        return ReadTextureAnims(parse, data, status);
      case 0x108:
        return ReadTextures(parse, data, status);
      case 0x109:
        return ReadMaterials(parse, data, status);
      case 0x10A:
        return ReadGeoset(parse, data, status);
      case 0x10B:
        return ReadGeosetAnim(parse, data, status);
      case 0x10C:
        return ReadBone(parse, data, status);
      case 0x10E:
        return ReadLight(parse, data, status);
      case 0x10F:
        return ReadHelper(parse, data, status);
      case 0x110:
        return ReadAttachment(parse, data, status);
      case 0x111:
        return ReadPivotPoints(parse, data, status);
      case 0x112:
        return ReadParticleEmitter(parse, data, status);
      case 0x113:
        return ReadParticleEmitter2(parse, data, status);
      case 0x114:
        return ReadCamera(parse, data, status);
      case 0x115:
        return ReadEventObject(parse, data, status);
      case 0x116:
      case 0x117:
        return ReadHitTest(parse, data, status);
      case 0x118:
        return ReadRibbonEmitter(parse, data, status);
      case 0x119:
        return ReadCollision(parse, data, status);
    }
    parse.FatalUnexpected(scanner.mdltext);
    return 0;
  }

  BOOL CallTextWriteHandlers(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *status) {
    for (UINT i = 0; i < 22; ++i) {
      if (!s_handlers[i](data, buffer, status)) {
        return 0;
      }
    }
    return 1;
  }

}  // namespace MDL
