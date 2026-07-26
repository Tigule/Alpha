#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
const char *__fastcall TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void __fastcall ReadEventKeyframes(
    Parser &parse,
    MDLSIMPLEKEYTRACK<MDLEVENTKEY> *track
) {
  unsigned int token;
  const char *tokenText;
  UTokenData value;
  long expected = parse.GetOptionalInt(&token, &tokenText, 0);
  if (expected > 0 && track) {
    track->keys.Reserve(expected);
  }
  parse.Expect('{', token, tokenText);
  token = parse.Token(&tokenText, &value);
  while (token == 0x140 || token == 0x152) {
    if (token == 0x152) {
      if (track) {
        track->globalSeqId = parse.ExpectInt();
      } else {
        parse.ExpectInt();
      }
    }
    parse.Expect(',');
    token = parse.Token(&tokenText, &value);
  }
  long actual = 0;
  while (token == 0x100) {
    if (track) {
      track->keys.New()->time = value.lVal;
    }
    parse.Expect(',');
    ++actual;
    token = parse.Token(&tokenText, &value);
  }
  parse.Expect('}', token, tokenText);
  if (expected >= 0 && actual != expected) {
    parse.WarningCount("event key frames", expected, actual);
  }
}

void __fastcall WriteEventKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &track,
    TSGrowableArray<char> &buffer
) {
  if (track.keys.Count()) {
    MDL::WriteLine(
        buffer,
        "\t%s %u {\n",
        MDL::TokenText(0x147),
        track.keys.Count()
    );
    if (track.globalSeqId != static_cast<unsigned int>(-1)) {
      MDL::WriteLine(
          buffer,
          "\t\t%s %d,\n",
          MDL::TokenText(0x152),
          track.globalSeqId
      );
    }
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      MDL::WriteLine(buffer, "\t\t%d,\n", track.keys.Ptr()[i].time);
    }
    MDL::WriteLine(buffer, "\t}\n");
  }
}

int __fastcall ReadBinEventKeyFrames(
    MDLSIMPLEKEYTRACK<MDLEVENTKEY> &track,
    CMsgBuffer &buffer,
    unsigned int *totalRead
) {
  if (buffer.Bytes() < 8) {
    return 0;
  }
  unsigned int count = buffer.GetUint();
  *totalRead += 4;
  if (!count) {
    return 0;
  }
  track.globalSeqId = buffer.GetUint();
  *totalRead += 4;
  if (4 * count > static_cast<unsigned int>(buffer.Bytes())) {
    return 0;
  }
  track.keys.SetCount(count);
  for (unsigned int i = 0; i < count; ++i) {
    track.keys.Ptr()[i].time = buffer.GetInt();
    *totalRead += 4;
  }
  return 1;
}

void __fastcall WriteBinEventKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &track,
    CMsgBuffer &buffer
) {
  if (track.keys.Count()) {
    buffer.AddDword('TVEK');
    buffer.AddUint(track.keys.Count());
    buffer.AddUint(track.globalSeqId);
    for (unsigned int i = 0; i < track.keys.Count(); ++i) {
      buffer.AddInt(track.keys.Ptr()[i].time);
    }
  }
}

unsigned int __fastcall GetBinEventKeyFramesSize(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &track
) {
  return track.keys.Count() ? 4 * track.keys.Count() + 12 : 0;
}

namespace MDL {

int __fastcall ReadEventObject(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLEVENTSECTION *eventObject = data.events.New();
  AddObjectErrors(errors);
  ReadObjectName(parse, eventObject->name);
  parse.Expect('{');
  const char *tokenText;
  unsigned int token = parse.Token(&tokenText, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    if (!ReadObjectBody(parse, token, 0, eventObject, status)) {
      if (token == 0x147) {
        ReadEventKeyframes(parse, &eventObject->eventKeys);
      } else {
        parse.FatalUnexpected(tokenText);
      }
    }
    token = parse.Token(&tokenText, 0);
  }
  parse.Expect('}', token, tokenText);
  ReadObjectEnd(
      errors,
      data,
      eventObject,
      data.events.Count() - 1,
      0x60000000
  );
  errors.Complete(status);
  return !parse.FoundError();
}

int __fastcall WriteEventObjects(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int needObjectIds = data.events.Count() != data.objects.Count();
    for (unsigned int i = 0; i < data.events.Count(); ++i) {
      const MDLEVENTSECTION &eventObject = data.events.Ptr()[i];
      WriteObjectHeader(data, eventObject, 0x115, needObjectIds, buffer);
      WriteEventKeyFrames(eventObject.eventKeys, buffer);
      WriteObjectTrailer(eventObject, buffer);
    }
  }
  return 1;
}

int __fastcall WriteBinEventObjects(
    const MDLDATA &data,
    CMsgBuffer &buffer,
    CMDLStatus *status
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && data.events.Count()) {
    buffer.AddDword('STVE');
    unsigned int totalSize = 4;
    unsigned int i;
    for (i = 0; i < data.events.Count(); ++i) {
      const MDLEVENTSECTION &eventObject = data.events.Ptr()[i];
      totalSize += GetBinGenObjectSize(eventObject)
          + GetBinEventKeyFramesSize(eventObject.eventKeys);
    }
    buffer.AddUint(totalSize);
    buffer.AddUint(data.events.Count());
    for (i = 0; i < data.events.Count(); ++i) {
      const MDLEVENTSECTION &eventObject = data.events.Ptr()[i];
      unsigned int size = GetBinGenObjectSize(eventObject)
          + GetBinEventKeyFramesSize(eventObject.eventKeys);
      buffer.AddUint(size);
      WriteBinGenObject(eventObject, buffer, status);
      WriteBinEventKeyFrames(eventObject.eventKeys, buffer);
    }
  }
  return 1;
}

int __fastcall ReadBinEventObjects(
    CMsgBuffer &buffer,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int totalRead = 4;
  unsigned int count = buffer.GetUint();
  data.events.SetCount(0);
  data.events.Reserve(count);
  while (totalRead < length) {
    MDLEVENTSECTION *eventObject = data.events.New();
    if (!eventObject) {
      status->FatalFlunked("EventObject", -1);
      return 0;
    }
    unsigned int sectionLength = buffer.GetUint();
    unsigned int read = 4;
    if (!ReadBinGenObject(*eventObject, buffer, status, read)) {
      status->Add(
          STATUS_ERROR,
          "Error reading gen object portion of event.\n"
      );
      return 0;
    }
    if (read < sectionLength) {
      unsigned long tag = buffer.GetDword();
      read += 4;
      if (tag == 'TVEK') {
        if (!ReadBinEventKeyFrames(eventObject->eventKeys, buffer, &read)) {
          status->Add(
              STATUS_ERROR,
              "Error reading event keys portion of event object.\n"
          );
          return 0;
        }
      } else {
        SkipUnknown(buffer, read);
      }
    }
    totalRead += read;
    if (totalRead > length) {
      status->FatalOverran(
          "EventObject section overran read buffer.\n",
          -1
      );
      return 0;
    }
    ReadBinObjectEnd(
        data,
        eventObject,
        data.events.Count() - 1,
        0x60000000
    );
  }
  return 1;
}

}
