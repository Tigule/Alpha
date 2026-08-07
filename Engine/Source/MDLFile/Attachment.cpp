#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

#include <storm.h>

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  static void IAddAttachmentErrors(TSet &errors) {
    AddObjectErrors(errors);
    errors.Add(0x1A0, 0, 0);
    errors.Add(0x1D9, 0, 0);
    errors.Add(0x124, 0, 0);
  }

  static void IReadAttachment(Parser &parse, TSet &errors, NTempest::C3Vector *pivot, MDLATTACHMENTSECTION *attachment, CMDLStatus *status) {
    parse.Expect('{');
    LPCSTR tokenText;
    UINT   token = parse.Token(&tokenText, 0);
    while (token && token != '}') {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokenText);
      }
      if (!ReadObjectBody(parse, token, pivot, attachment, status)) {
        if (token == 0x124) {
          attachment->attachmentId = parse.ExpectInt();
          parse.Expect(',');
        } else if (token == 0x1A0) {
          SStrCopy(attachment->path, parse.ExpectString(), 260);
          parse.Expect(',');
        } else if (token == 0x1D9) {
          MDLKEYTRACK<float> &track = attachment->visibilityKeys;
          UTokenData          tokenData;
          long                expected = parse.GetOptionalInt(&token, &tokenText, &tokenData);
          if (expected > 0) {
            track.keys.ReserveSpace(expected);
          }
          parse.Expect('{', token, tokenText);
          token = parse.Token(&tokenText, &tokenData);
          for (;;) {
            if (token == 0x128) {
              track.type = TRACK_BEZIER;
            } else if (token == 0x140) {
              track.type = TRACK_NO_INTERP;
            } else if (token == 0x152) {
              track.globalSeqId = parse.ExpectInt();
            } else if (token == 0x15B) {
              track.type = TRACK_HERMITE;
            } else if (token == 0x167) {
              track.type = TRACK_LINEAR;
            } else {
              break;
            }
            parse.Expect(',');
            token = parse.Token(&tokenText, &tokenData);
          }

          long actual = 0;
          while (token == 0x100) {
            MDLKEYFRAME<float> *key = track.keys.New();
            key->time = tokenData.lVal;
            parse.Expect(':');
            ReadFloatKeyData(parse, &key->value, 1);
            parse.Expect(',');
            ++actual;
            if (track.type > TRACK_LINEAR) {
              parse.Expect(0x15E);
              ReadFloatKeyData(parse, &key->inTan, 1);
              parse.Expect(',');
              parse.Expect(0x18A);
              ReadFloatKeyData(parse, &key->outTan, 1);
              parse.Expect(',');
            }
            token = parse.Token(&tokenText, &tokenData);
          }
          parse.Expect('}', token, tokenText);
          if (expected >= 0 && actual != expected) {
            parse.WarningCount("key frames", expected, actual);
          }
        } else {
          parse.FatalUnexpected(tokenText);
        }
      }
      token = parse.Token(&tokenText, 0);
    }
    parse.Expect('}', token, tokenText);
  }

  BOOL ReadAttachment(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet                  errors;
    MDLATTACHMENTSECTION *attachment = data.attachments.New();
    IAddAttachmentErrors(errors);
    ReadObjectName(parse, attachment->name);
    IReadAttachment(parse, errors, data.pivotPoints.Count() < 500 ? data.pivotPoints.New() : 0, attachment, status);
    if (errors.NotFound(0x124)) {
      attachment->attachmentId = data.attachments.Count() - 1;
    }
    ReadObjectEnd(errors, data, attachment, data.attachments.Count() - 1, 0x40000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

  BOOL WriteAttachments(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (!static_cast<LPCSTR>(data.model.animationFile)[0]) {
      int                         needObjIds = data.attachments.Count() != data.objects.Count();
      const MDLATTACHMENTSECTION *attachment = data.attachments.Ptr();
      for (; attachment < data.attachments.Ptr() + data.attachments.Count(); ++attachment) {
        WriteObjectHeader(data, *attachment, 0x110, needObjIds, buffer);
        if (SStrLen(attachment->path)) {
          WriteLine(buffer, "\t%s \"%s\",\n", TokenText(0x1A0), static_cast<LPCSTR>(attachment->path));
        }
        const MDLKEYTRACK<float> &track = attachment->visibilityKeys;
        if (track.keys.Count()) {
          WriteLine(buffer, "\t%s %d {\n", TokenText(0x1D9), track.keys.Count());
          UINT typeToken = 0;
          switch (track.type) {
            case TRACK_NO_INTERP:
              typeToken = 0x140;
              break;
            case TRACK_LINEAR:
              typeToken = 0x167;
              break;
            case TRACK_HERMITE:
              typeToken = 0x15B;
              break;
            case TRACK_BEZIER:
              typeToken = 0x128;
              break;
            default:
              break;
          }
          if (typeToken) {
            WriteLine(buffer, "\t\t%s,\n", TokenText(typeToken));
          }
          if (track.globalSeqId != static_cast<UINT>(-1)) {
            WriteLine(buffer, "\t\t%s %d,\n", TokenText(0x152), track.globalSeqId);
          }
          for (UINT n = 0; n < track.keys.Count(); ++n) {
            const MDLKEYFRAME<float> &key = track.keys.Ptr()[n];
            WriteLine(buffer, "\t\t%d: ", key.time);
            WriteKeyData(buffer, &key.value, 1);
            if (track.type > TRACK_LINEAR) {
              WriteLine(buffer, "\t\t\t%s ", TokenText(0x15E));
              WriteKeyData(buffer, &key.inTan, 1);
              WriteLine(buffer, "\t\t\t%s ", TokenText(0x18A));
              WriteKeyData(buffer, &key.outTan, 1);
            }
          }
          WriteLine(buffer, "\t}\n");
        }
        if (attachment->attachmentId != attachment - data.attachments.Ptr()) {
          WriteLine(buffer, "\t%s %d,\n", TokenText(0x124), attachment->attachmentId);
        }
        WriteObjectTrailer(*attachment, buffer);
      }
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

  static void IWriteBinAttachmentSection(const MDLATTACHMENTSECTION &section, UINT geosetAnimId, CMsgBuffer &buffer, CMDLStatus *status) {
    buffer.AddUint(GetBinAttachmentSize(section));
    WriteBinGenObject(section, buffer, status);
    buffer.AddUint(section.attachmentId);
    buffer.AddByte(static_cast<BYTE>(geosetAnimId));
    buffer.AddTcharArray(section.path, 260, 1);
    const MDLKEYTRACK<float> &track = section.visibilityKeys;
    if (track.keys.Count()) {
      buffer.AddDword('SIVK');
      buffer.AddUint(track.keys.Count());
      buffer.AddUint(track.type);
      buffer.AddUint(track.globalSeqId);
      UINT values = track.type > TRACK_LINEAR ? 3 : 1;
      for (UINT i = 0; i < track.keys.Count(); ++i) {
        const MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value, values);
      }
    }
  }

  static BOOL ReadBinAttachment(CMsgBuffer &buffer, MDLATTACHMENTSECTION *attachment, CMDLStatus *status, UINT &totalRead) {
    UINT sectionLength = buffer.GetUint();
    UINT localRead = 4;
    if (!ReadBinGenObject(*attachment, buffer, status, localRead)) {
      status->Add(STATUS_ERROR, "Error reading gen object portion of attachment.\n");
      return 0;
    }
    attachment->attachmentId = buffer.GetUint();
    buffer.GetByte();
    buffer.GetTcharArray(attachment->path, 260);
    localRead += 265;

    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      if (tag == 'SIVK') {
        if (buffer.Bytes() < 12) {
          status->Add(STATUS_ERROR, "Error reading visibility keys in attachment.\n");
          return 0;
        }
        UINT count = buffer.GetUint();
        localRead += 4;
        if (!count) {
          status->Add(STATUS_ERROR, "Error reading visibility keys in attachment.\n");
          return 0;
        }
        MDLKEYTRACK<float> &track = attachment->visibilityKeys;
        track.type = static_cast<MDLTRACKTYPE>(buffer.GetUint());
        track.globalSeqId = buffer.GetUint();
        localRead += 8;
        UINT values = track.type > TRACK_LINEAR ? 3 : 1;
        UINT bytesPerKey = 4 + 4 * values;
        if (count * bytesPerKey > static_cast<UINT>(buffer.Bytes())) {
          status->Add(STATUS_ERROR, "Error reading visibility keys in attachment.\n");
          return 0;
        }
        track.keys.SetCount(count);
        for (UINT i = 0; i < count; ++i) {
          MDLKEYFRAME<float> &key = track.keys.Ptr()[i];
          key.time = buffer.GetInt();
          buffer.GetFloatArray(&key.value, values);
          localRead += bytesPerKey;
        }
      } else {
        SkipUnknown(buffer, localRead);
      }
      if (localRead > sectionLength) {
        status->FatalOverran("Attachment visKeys", -1);
        return 0;
      }
    }
    totalRead += localRead;
    return 1;
  }

  BOOL ReadBinAttachments(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
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

  static UINT GetParentGeosetAnimId(const MDLDATA &data, const MDLATTACHMENTSECTION &attachment) {
    const MDLGENOBJECT *object = &attachment;
    for (;;) {
      UINT parentId = object->parentId;
      if (parentId == static_cast<UINT>(-1)) {
        return static_cast<UINT>(-1);
      }
      object = data.objects.Ptr()[parentId];
      const MDLBONESECTION *first = data.bones.Ptr();
      UINT                  offset = reinterpret_cast<LPCSTR>(object) - reinterpret_cast<LPCSTR>(first);
      if (offset < sizeof(MDLBONESECTION) * data.bones.Count()) {
        const MDLBONESECTION *bone = reinterpret_cast<const MDLBONESECTION *>(object);
        return bone->geosetId == static_cast<UINT>(-1) ? static_cast<UINT>(-1) : bone->geosetAnimId;
      }
    }
  }

  BOOL WriteBinAttachments(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
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

}  // namespace MDL
