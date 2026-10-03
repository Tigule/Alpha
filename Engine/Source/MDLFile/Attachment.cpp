#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadAttachment(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteAttachments(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         ReadBinAttachments(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
  BOOL         WriteBinAttachments(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
}  // namespace MDL

static void IAddAttachmentErrors(TSet &errors) {
  AddObjectErrors(errors);
  errors.Add(0x1A0, 0, 0);
  errors.Add(0x1D9, 0, 0);
  errors.Add(0x124, 0, 0);
}

static void IReadAttachment(Parser &parse, TSet &errors, NTempest::C3Vector *pivot, MDLATTACHMENTSECTION *attachment, CMDLStatus *status) {
  parse.Expect('{');
  LPCSTR tokentext;
  for (UINT token = parse.Token(&tokentext, 0); token != '}' && token; token = parse.Token(&tokentext, 0)) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, token, pivot, attachment, status)) {
      switch (token) {
        case 0x1A0:
          SStrCopy(attachment->path, parse.ExpectString(), 260);
          break;
        case 0x1D9:
          ReadObjectFloatKeyframes(parse, &attachment->visibilityKeys);
          continue;
        case 0x124:
          attachment->attachmentId = parse.ExpectInt();
          break;
        default:
          parse.FatalUnexpected(tokentext);
          continue;
      }
      parse.Expect(',');
    }
  }
  parse.Expect('}', token, tokentext);
}

BOOL MDL::ReadAttachment(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet                  errors;
  MDLATTACHMENTSECTION *attachment = data.attachments.New();
  NTempest::C3Vector   *pivot = data.version < 500 ? data.pivotPoints.New() : 0;

  IAddAttachmentErrors(errors);
  ReadObjectName(parse, attachment->name);
  IReadAttachment(parse, errors, pivot, attachment, status);
  if (errors.NotFound(0x124)) {
    attachment->attachmentId = data.attachments.Count() - 1;
  }
  ReadObjectEnd(errors, data, attachment, data.attachments.Count() - 1, 0x40000000);
  errors.Complete(status);
  return !parse.FoundError();
}

BOOL MDL::WriteAttachments(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  if (static_cast<LPCSTR>(data.model.animationFile)[0]) {
    return 1;
  }
  int                         needObjIds = data.attachments.Count() != data.objects.Count();
  const MDLATTACHMENTSECTION *attachment = data.attachments.Ptr();
  for (UINT n = 0; n < data.attachments.Count(); ++n, ++attachment) {
    WriteObjectHeader(data, *attachment, 0x110, needObjIds, buffer);
    if (SStrLen(attachment->path)) {
      MDL::WriteLine(buffer, "\t%s \"%s\",\n", MDL::TokenText(0x1A0), static_cast<LPCSTR>(attachment->path));
    }
    WriteFloatKeyFrames(0x1D9, "\t", attachment->visibilityKeys, buffer);
    if (attachment->attachmentId != n) {
      MDL::WriteLine(buffer, "\t%s %d,\n", MDL::TokenText(0x124), attachment->attachmentId);
    }
    WriteObjectTrailer(*attachment, buffer);
  }
  return 1;
}

static UINT GetBinAttachmentSize(const MDLATTACHMENTSECTION &section) {
  UINT size = GetBinGenObjectSize(section) + 269;
  if (section.visibilityKeys.keys.Count()) {
    UINT values = section.visibilityKeys.type > TRACK_LINEAR ? 3 : 1;
    size += 16 + section.visibilityKeys.keys.Count() * (4 + 4 * values);
  }
  return size;
}

static void IWriteBinAttachmentSection(const MDLATTACHMENTSECTION &section, UINT geosetAnimId, CMsgBuffer &buf, CMDLStatus *status) {
  buf.AddUint(GetBinAttachmentSize(section));
  WriteBinGenObject(section, buf, status);
  buf.AddUint(section.attachmentId);
  buf.AddByte(static_cast<BYTE>(geosetAnimId));
  buf.AddTcharArray(section.path, 260, 1);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buf);
}

static BOOL ReadBinAttachment(CMsgBuffer &buf, MDLATTACHMENTSECTION *pAtt, CMDLStatus *status, UINT &totalRead) {
  UINT sectionLength = buf.GetUint();
  UINT localBytesRead = 4;

  if (!ReadBinGenObject(*pAtt, buf, status, localBytesRead)) {
    status->Add(STATUS_ERROR, "Error reading gen object portion of attachment.\n");
    return 0;
  }

  pAtt->attachmentId = buf.GetUint();
  localBytesRead += 4;

  buf.GetByte();
  localBytesRead++;

  buf.GetTcharArray(pAtt->path, 260);
  localBytesRead += 260;

  while (localBytesRead < sectionLength) {
    DWORD tag = buf.GetDword();
    localBytesRead += 4;

    switch (tag) {
      case 'SIVK':
        if (!ReadBinFloatKeyFrames(pAtt->visibilityKeys, buf, localBytesRead)) {
          status->Add(STATUS_ERROR, "Error reading visibility keys in attachment.\n");
          return 0;
        }
        break;
      default:
        SkipUnknown(buf, localBytesRead);
        break;
    }

    if (localBytesRead > sectionLength) {
      status->FatalOverran("Attachment visKeys", -1);
      return 0;
    }
  }

  totalRead += localBytesRead;

  return 1;
}

BOOL MDL::ReadBinAttachments(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
  UINT totalRead = 8;
  UINT numAttached = buf.GetUint();
  buf.GetUint();
  data.attachments.SetCount(0);
  data.attachments.ReserveSpace(numAttached);
  MDLATTACHMENTSECTION *pAtt;
  while (totalRead < length) {
    pAtt = data.attachments.New();
    if (!pAtt) {
      status->FatalFlunked("Attachment", -1);
      return 0;
    }
    ReadBinAttachment(buf, pAtt, status, totalRead);
    if (totalRead > length) {
      status->FatalOverran("Attachment", -1);
      return 0;
    }
    ReadBinObjectEnd(data, pAtt, data.attachments.Count() - 1, 0x40000000);
  }
  return 1;
}

static UINT GetParentGeosetAnimId(const MDLDATA &data, const MDLATTACHMENTSECTION &attachmentData) {
  const MDLGENOBJECT *object = &attachmentData;
  do {
    if (object->parentId == static_cast<UINT>(-1)) {
      return static_cast<UINT>(-1);
    }
    object = data.objects[object->parentId];
  } while (!(static_cast<UINT>(reinterpret_cast<LPCSTR>(object) - reinterpret_cast<LPCSTR>(data.bones.Ptr())) < sizeof(MDLBONESECTION) * data.bones.Count()));
  return static_cast<const MDLBONESECTION *>(object)->geosetId == static_cast<UINT>(-1) ? static_cast<UINT>(-1) : static_cast<const MDLBONESECTION *>(object)->geosetAnimId;
}

BOOL MDL::WriteBinAttachments(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
  if (!static_cast<LPCSTR>(data.model.animationFile)[0] && data.attachments.Count()) {
    buf.AddDword('HCTA');
    UINT totalSize = 8;
    UINT numAttached = data.attachments.Count();
    UINT highestId = 0;
    UINT i;

    for (i = 0; i < numAttached; ++i) {
      totalSize += GetBinAttachmentSize(data.attachments[i]);
      if (highestId < data.attachments[i].attachmentId) {
        highestId = data.attachments[i].attachmentId;
      }
    }
    buf.AddUint(totalSize);
    buf.AddUint(numAttached);
    buf.AddUint(highestId);
    for (i = 0; i < numAttached; ++i) {
      UINT geosetAnimId = GetParentGeosetAnimId(data, data.attachments[i]);
      IWriteBinAttachmentSection(data.attachments[i], geosetAnimId, buf, status);
    }
  }
  return 1;
}
