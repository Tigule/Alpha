#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadBone(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteBones(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinBones(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinBone(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

static void IAddBoneErrors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(MDLTOK_GEOSETID, 1, 0);
  errors.Add(MDLTOK_GEOSETANIMID, 0, 0);
}

static void IReadGeoAnimId(Parser &parse, UINT *geosetId) {
  LPCSTR     tokentext;
  UTokenData savedvalue;
  UINT       token = parse.Token(&tokentext, &savedvalue);
  *geosetId = token == MDLTOK_CAPNONE ? (UINT)-1 : parse.ExpectInt(token, tokentext, &savedvalue);
}

static void IReadGeosetId(Parser &parse, UINT *geosetId) {
  LPCSTR     tokentext;
  UTokenData savedvalue;
  UINT       token = parse.Token(&tokentext, &savedvalue);
  *geosetId = token == MDLTOK_MULTIPLE ? (UINT)-1 : parse.ExpectInt(token, tokentext, &savedvalue);
}

BOOL MDL::ReadBone(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  FATALASSERT(status);
  TSet                errors;
  MDLBONESECTION     *bone = data.bones.New();
  NTempest::C3Vector *pivot = data.version < 500 ? data.pivotPoints.New() : 0;

  IAddBoneErrors(errors);
  ReadObjectName(parse, bone->name);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token;
  for (token = parse.Token(&tokentext, 0); token != '}'; token = parse.Token(&tokentext, 0)) {
    if (!token) {
      break;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, token, pivot, bone, status)) {
      switch (token) {
        case MDLTOK_GEOSETID:
          IReadGeosetId(parse, &bone->geosetId);
          break;
        default:
          parse.FatalUnexpected(tokentext);
          break;
        case MDLTOK_GEOSETANIMID:
          IReadGeoAnimId(parse, &bone->geosetAnimId);
          break;
      }
      parse.Expect(',');
    }
  }
  parse.Expect('}', token, tokentext);
  ReadObjectEnd(errors, data, bone, data.bones.Count() - 1, 0x30000000);
  if (errors.NotFound(MDLTOK_GEOSETANIMID)) {
    bone->geosetAnimId = bone->geosetId;
  }
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteBoneSection(const MDLDATA &data, const MDLBONESECTION &section, int needObjectIds, TSGrowableArray<char> &buffer) {
  WriteObjectHeader(data, section, MDLTOK_BONE, needObjectIds, buffer);
  MDL::WriteLine(buffer, "\t%s ", MDL::TokenText(MDLTOK_GEOSETID));
  if (section.geosetId == (UINT)-1) {
    MDL::WriteLine(buffer, "%s,\n", MDL::TokenText(MDLTOK_MULTIPLE));
  } else {
    MDL::WriteLine(buffer, "%d,\n", section.geosetId);
  }
  MDL::WriteLine(buffer, "\t%s ", MDL::TokenText(MDLTOK_GEOSETANIMID));
  if (section.geosetAnimId == (UINT)-1) {
    MDL::WriteLine(buffer, "%s,\n", MDL::TokenText(MDLTOK_CAPNONE));
  } else {
    MDL::WriteLine(buffer, "%d,\n", section.geosetAnimId);
  }
  WriteObjectTrailer(section, buffer);
}

BOOL MDL::WriteBones(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (((LPCSTR)data.model.animationFile)[0]) {
    return 1;
  }
  int needObjIds = data.bones.Count() != data.objects.Count();
  for (UINT i = 0; i < data.bones.Count(); ++i) {
    IWriteBoneSection(data, data.bones[i], needObjIds, buffer);
  }
  return 1;
}

static UINT GetBinBonesSize(const MDLBONESECTION &section) {
  return GetBinGenObjectSize(section) + 8;
}

static void IWriteBinBoneSection(const MDLBONESECTION &section, CMsgBuffer &buf, CMDLStatus *status) {
  WriteBinGenObject(section, buf, status);
  buf.AddUint(section.geosetId);
  buf.AddUint(section.geosetAnimId);
}

BOOL MDL::WriteBinBones(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
  if (((LPCSTR)data.model.animationFile)[0]) {
    return 1;
  }
  if (!data.bones.Count()) {
    return 1;
  }
  buf.AddDword('ENOB');
  UINT totalSize = 4;
  UINT numBones = data.bones.Count();
  UINT i;
  for (i = 0; i < numBones; ++i) {
    totalSize += GetBinBonesSize(data.bones[i]);
  }
  buf.AddUint(totalSize);
  buf.AddUint(numBones);
  for (i = 0; i < numBones; ++i) {
    IWriteBinBoneSection(data.bones[i], buf, status);
  }
  return 1;
}

BOOL MDL::ReadBinBone(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT numBones = buf.GetUint();
  UINT totalRead = 4;
  data.bones.SetCount(0);
  data.bones.ReserveSpace(numBones);
  MDLBONESECTION *pBone;
  while (totalRead < length) {
    pBone = data.bones.New();
    if (!pBone) {
      status->FatalFlunked("Bone", -1);
      return 0;
    }
    if (!ReadBinGenObject(*pBone, buf, status, totalRead)) {
      status->Add(STATUS_ERROR, "Error reading gen object portion of bone.\n");
      return 0;
    }
    pBone->geosetId = buf.GetUint();
    pBone->geosetAnimId = buf.GetUint();
    totalRead += 8;
    if (totalRead > length) {
      status->FatalOverran("Bone", -1);
      return 0;
    }
    ReadBinObjectEnd(data, pBone, data.bones.Count() - 1, 0x30000000);
  }
  return 1;
}
