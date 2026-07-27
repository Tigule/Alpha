#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);

static void IAddBoneErrors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(0x150, 1, 0);
  errors.Add(0x151, 0, 0);
}

static void IReadGeoAnimId(Parser &parse, unsigned int *geosetAnimId) {
  const char *tokenText;
  UTokenData value;
  unsigned int token = parse.Token(&tokenText, &value);
  *geosetAnimId = token == 0x179
      ? static_cast<unsigned int>(-1)
      : parse.ExpectInt(token, tokenText, &value);
}

static void IReadGeosetId(Parser &parse, unsigned int *geosetId) {
  const char *tokenText;
  UTokenData value;
  unsigned int token = parse.Token(&tokenText, &value);
  *geosetId = token == 0x175
      ? static_cast<unsigned int>(-1)
      : parse.ExpectInt(token, tokenText, &value);
}

int ReadBone(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  FATALASSERT(status);
  TSet errors;
  MDLBONESECTION *bone = data.bones.New();
  NTempest::C3Vector *pivot =
      data.pivotPoints.Count() < 500 ? data.pivotPoints.New() : 0;

  IAddBoneErrors(errors);
  ReadObjectName(parse, bone->name);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, pivot, bone, status)) {
      if (token == 0x150) {
        IReadGeosetId(parse, &bone->geosetId);
      } else if (token == 0x151) {
        IReadGeoAnimId(parse, &bone->geosetAnimId);
      } else {
        parse.FatalUnexpected(tokenText);
      }
      parse.Expect(',');
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  ReadObjectEnd(
      errors,
      data,
      bone,
      data.bones.Count() - 1,
      0x30000000
  );
  if (errors.NotFound(0x151)) {
    bone->geosetAnimId = bone->geosetId;
  }
  errors.Complete(status);
  return !parse.FoundError();
}

static void IWriteBoneSection(
    const MDLDATA &data,
    const MDLBONESECTION &section,
    int needObjectIds,
    TSGrowableArray<char> &buffer
) {
  WriteObjectHeader(data, section, 0x10C, needObjectIds, buffer);
  WriteLine(buffer, "\t%s ", TokenText(0x150));
  if (section.geosetId == static_cast<unsigned int>(-1)) {
    WriteLine(buffer, "%s,\n", TokenText(0x175));
  } else {
    WriteLine(buffer, "%d,\n", section.geosetId);
  }
  WriteLine(buffer, "\t%s ", TokenText(0x151));
  if (section.geosetAnimId == static_cast<unsigned int>(-1)) {
    WriteLine(buffer, "%s,\n", TokenText(0x179));
  } else {
    WriteLine(buffer, "%d,\n", section.geosetAnimId);
  }
  WriteObjectTrailer(section, buffer);
}

int WriteBones(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int needObjectIds = data.bones.Count() != data.objects.Count();
    for (unsigned int i = 0; i < data.bones.Count(); ++i) {
      IWriteBoneSection(
          data,
          data.bones.Ptr()[i],
          needObjectIds,
          buffer
      );
    }
  }
  return 1;
}

static unsigned int GetBinBonesSize(const MDLBONESECTION &section) {
  return GetBinGenObjectSize(section) + 8;
}

static void IWriteBinBoneSection(
    const MDLBONESECTION &section,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  WriteBinGenObject(section, buffer, status);
  buffer.AddUint(section.geosetId);
  buffer.AddUint(section.geosetAnimId);
}

int WriteBinBones(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.bones.Count()) {
    buffer.AddDword('ENOB');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.bones.Count(); ++i) {
      totalSize += GetBinBonesSize(data.bones.Ptr()[i]);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.bones.Count());
    for (i = 0; i < data.bones.Count(); ++i) {
      IWriteBinBoneSection(data.bones.Ptr()[i], buffer, status);
    }
  }
  return 1;
}

int ReadBinBone(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int count = buffer.GetUint();
  unsigned int totalRead = 4;
  data.bones.SetCount(0);
  data.bones.Reserve(count);
  while (totalRead < length) {
    MDLBONESECTION *bone = data.bones.New();
    if (!bone) {
      status->FatalFlunked("Bone", -1);
      return 0;
    }
    if (!ReadBinGenObject(*bone, buffer, status, totalRead)) {
      status->Add(
          STATUS_ERROR,
          "Error reading gen object portion of bone.\n"
      );
      return 0;
    }
    bone->geosetId = buffer.GetUint();
    bone->geosetAnimId = buffer.GetUint();
    totalRead += 8;
    if (totalRead > length) {
      status->FatalOverran("Bone", -1);
      return 0;
    }
    ReadBinObjectEnd(
        data,
        bone,
        data.bones.Count() - 1,
        0x30000000
    );
  }
  return 1;
}

}
