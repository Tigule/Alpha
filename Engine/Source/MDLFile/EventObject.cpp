#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
const char *TokenText(unsigned int token);
void __cdecl WriteLine(TSGrowableArray<char> &buffer, const char *format, ...);
}

void ReadEventKeyframes(
    Parser &parse,
    MDLSIMPLEKEYTRACK<MDLEVENTKEY> *keyTrack
) {
  unsigned int savedtoken;
  const char *tokentext;
  UTokenData value;
  long actual = 0;
  long count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0 && keyTrack) {
    keyTrack->keys.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &value);
  while (savedtoken == 0x140 || savedtoken == 0x152) {
    if (savedtoken == 0x152) {
      if (keyTrack) {
        keyTrack->globalSeqId = parse.ExpectInt();
      } else {
        parse.ExpectInt();
      }
    }
    parse.Expect(',');
    savedtoken = parse.Token(&tokentext, &value);
  }
  while (savedtoken == 0x100) {
    if (keyTrack) {
      keyTrack->keys.New()->time = value.lVal;
    }
    parse.Expect(',');
    ++actual;
    savedtoken = parse.Token(&tokentext, &value);
  }
  parse.Expect('}', savedtoken, tokentext);
  if (count >= 0 && actual != count) {
    parse.WarningCount("event key frames", count, actual);
  }
}

void WriteEventKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes,
    TSGrowableArray<char> &buffer
) {
  if (keyframes.keys.Count()) {
    MDL::WriteLine(
        buffer,
        "\t%s %u {\n",
        MDL::TokenText(0x147),
        keyframes.keys.Count()
    );
    if (keyframes.globalSeqId != static_cast<unsigned int>(-1)) {
      MDL::WriteLine(
          buffer,
          "\t\t%s %d,\n",
          MDL::TokenText(0x152),
          keyframes.globalSeqId
      );
    }
    for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
      MDL::WriteLine(buffer, "\t\t%d,\n", keyframes.keys.Ptr()[i].time);
    }
    MDL::WriteLine(buffer, "\t}\n");
  }
}

int ReadBinEventKeyFrames(
    MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes,
    CMsgBuffer &buf,
    unsigned int *totalRead
) {
  if (buf.Bytes() < 8) {
    return 0;
  }
  keyframes.keys.SetCount(buf.GetUint());
  *totalRead += 4;
  if (!keyframes.keys.Count()) {
    return 0;
  }
  keyframes.globalSeqId = buf.GetUint();
  *totalRead += 4;
  if (4 * keyframes.keys.Count() > static_cast<unsigned int>(buf.Bytes())) {
    return 0;
  }
  for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
    keyframes.keys.Ptr()[i].time = buf.GetInt();
    *totalRead += 4;
  }
  return 1;
}

void WriteBinEventKeyFrames(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes,
    CMsgBuffer &buf
) {
  if (keyframes.keys.Count()) {
    buf.AddDword('TVEK');
    buf.AddUint(keyframes.keys.Count());
    buf.AddUint(keyframes.globalSeqId);
    for (unsigned int i = 0; i < keyframes.keys.Count(); ++i) {
      buf.AddInt(keyframes.keys.Ptr()[i].time);
    }
  }
}

unsigned int GetBinEventKeyFramesSize(
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes
) {
  return keyframes.keys.Count() ? 4 * keyframes.keys.Count() + 12 : 0;
}

namespace MDL {

int ReadEventObject(
    Parser &parse,
    MDLDATA &data,
    CMDLStatus *status
) {
  TSet errors;
  MDLEVENTSECTION *eventObject = data.events.New();
  AddObjectErrors(errors);
  ReadObjectName(parse, eventObject->name);
  parse.Expect('{');
  const char *tokentext;
  unsigned int token = parse.Token(&tokentext, 0);
  while (token && token != '}') {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    if (!ReadObjectBody(parse, token, 0, eventObject, status)) {
      if (token == 0x147) {
        ReadEventKeyframes(parse, &eventObject->eventKeys);
      } else {
        parse.FatalUnexpected(tokentext);
      }
    }
    token = parse.Token(&tokentext, 0);
  }
  parse.Expect('}', token, tokentext);
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

int WriteEventObjects(
    const MDLDATA &data,
    TSGrowableArray<char> &buffer,
    CMDLStatus *
) {
  if (!static_cast<const char *>(data.model.animationFile)[0]) {
    int needObjIds = data.events.Count() != data.objects.Count();
    const MDLEVENTSECTION *eventObject = data.events.Ptr();
    while (eventObject != data.events.Ptr() + data.events.Count()) {
      WriteObjectHeader(data, *eventObject, 0x115, needObjIds, buffer);
      WriteEventKeyFrames(eventObject->eventKeys, buffer);
      WriteObjectTrailer(*eventObject, buffer);
      ++eventObject;
    }
  }
  return 1;
}

int WriteBinEventObjects(
    const MDLDATA &data,
    CMsgBuffer &buf,
    CMDLStatus *status
) {
  unsigned int numEvents = data.events.Count();
  unsigned int totalSize = 4;
  unsigned int n = 0;
  if (!static_cast<const char *>(data.model.animationFile)[0]
      && numEvents) {
    buf.AddDword('STVE');
    while (n < numEvents) {
      const MDLEVENTSECTION &eventObject = data.events[n];
      totalSize += GetBinEventKeyFramesSize(eventObject.eventKeys)
          + GetBinGenObjectSize(eventObject);
      ++n;
    }
    buf.AddUint(totalSize);
    buf.AddUint(numEvents);
    n = 0;
    while (n < numEvents) {
      const MDLEVENTSECTION &eventObject = data.events[n];
      unsigned int size = GetBinEventKeyFramesSize(eventObject.eventKeys)
          + GetBinGenObjectSize(eventObject);
      buf.AddUint(size);
      WriteBinGenObject(eventObject, buf, status);
      WriteBinEventKeyFrames(eventObject.eventKeys, buf);
      ++n;
    }
  }
  return 1;
}

int ReadBinEventObjects(
    CMsgBuffer &buf,
    unsigned int length,
    MDLDATA &data,
    CMDLStatus *status
) {
  unsigned int numEvents = buf.GetUint();
  unsigned int totalRead = 4;
  data.events.SetCount(0);
  data.events.ReserveSpace(numEvents);
  while (numEvents) {
    --numEvents;
    MDLEVENTSECTION *eventObject = data.events.New();
    if (!eventObject) {
      status->FatalFlunked("EventObject", -1);
      return 0;
    }
    unsigned int sectionLength = buf.GetUint();
    unsigned int read = 4;
    if (!ReadBinGenObject(*eventObject, buf, status, read)) {
      status->Add(
          STATUS_ERROR,
          "Error reading gen object portion of event.\n"
      );
      return 0;
    }
    if (read < sectionLength) {
      unsigned long tag = buf.GetDword();
      read += 4;
      if (tag == 'TVEK') {
        if (!ReadBinEventKeyFrames(eventObject->eventKeys, buf, &read)) {
          status->Add(
              STATUS_ERROR,
              "Error reading event keys portion of event object.\n"
          );
          return 0;
        }
      } else {
        SkipUnknown(buf, read);
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
