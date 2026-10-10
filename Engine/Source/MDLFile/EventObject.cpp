#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
}  // namespace MDL

void ReadEventKeyframes(Parser &parse, MDLSIMPLEKEYTRACK<MDLEVENTKEY> *keyTrack) {
  UINT       savedtoken;
  LPCSTR     tokentext;
  UTokenData value;
  long       actual = 0;
  long       count = parse.GetOptionalInt(&savedtoken, &tokentext, 0);
  if (count > 0 && keyTrack) {
    keyTrack->keys.ReserveSpace(count);
  }
  parse.Expect('{', savedtoken, tokentext);
  savedtoken = parse.Token(&tokentext, &value);
  while (savedtoken == MDLTOK_DONTINTERP || savedtoken == MDLTOK_GLOBALSEQID) {
    if (savedtoken == MDLTOK_GLOBALSEQID) {
      if (keyTrack) {
        keyTrack->globalSeqId = parse.ExpectInt();
      } else {
        parse.ExpectInt();
      }
    }
    parse.Expect(',');
    savedtoken = parse.Token(&tokentext, &value);
  }
  while (savedtoken == MDLTOK_LONG) {
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

void WriteEventKeyFrames(const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes, TSGrowableArray<char> &buffer) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  MDL::WriteLine(buffer, "\t%s %u {\n", MDL::TokenText(MDLTOK_EVENT_TRACK), numKeys);
  if (keyframes.globalSeqId != (UINT)-1) {
    MDL::WriteLine(buffer, "\t\t%s %d,\n", MDL::TokenText(MDLTOK_GLOBALSEQID), keyframes.globalSeqId);
  }
  const MDLEVENTKEY *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    MDL::WriteLine(buffer, "\t\t%d,\n", key->time);
  }
  MDL::WriteLine(buffer, "\t}\n");
}

BOOL ReadBinEventKeyFrames(MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes, CMsgBuffer &buf, UINT *totalRead) {
  if (buf.Bytes() < 2 * sizeof(UINT)) {
    return 0;
  }
  UINT numKeys = buf.GetUint();
  *totalRead += sizeof(UINT);
  if (!numKeys) {
    return 0;
  }
  keyframes.globalSeqId = buf.GetUint();
  *totalRead += sizeof(UINT);
  if ((int)(numKeys * sizeof(MDLEVENTKEY)) > buf.Bytes()) {
    return 0;
  }
  keyframes.keys.SetCount(numKeys);
  MDLEVENTKEY *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    key->time = buf.GetInt();
    *totalRead += sizeof(int);
  }
  return 1;
}

void WriteBinEventKeyFrames(const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes, CMsgBuffer &buf) {
  UINT numKeys = keyframes.keys.Count();
  if (!numKeys) {
    return;
  }
  buf.AddDword('TVEK');
  buf.AddUint(numKeys);
  buf.AddUint(keyframes.globalSeqId);
  const MDLEVENTKEY *key = keyframes.keys.Ptr();
  for (UINT i = numKeys; i; --i, ++key) {
    buf.AddInt(key->time);
  }
}

UINT GetBinEventKeyFramesSize(const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyframes) {
  if (!keyframes.keys.Count()) {
    return 0;
  }
  return 4 * keyframes.keys.Count() + 12;
}

namespace MDL {

  BOOL ReadEventObject(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet             errors;
    MDLEVENTSECTION *eventObject = data.events.New();
    AddObjectErrors(errors);
    ReadObjectName(parse, eventObject->name);
    parse.Expect('{');
    LPCSTR tokentext;
    UINT   token = parse.Token(&tokentext, 0);
    while (token != '}' && token) {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokentext);
      }
      if (!ReadObjectBody(parse, token, 0, eventObject, status)) {
        if (token == MDLTOK_EVENT_TRACK) {
          ReadEventKeyframes(parse, &eventObject->eventKeys);
        } else {
          parse.FatalUnexpected(tokentext);
        }
      }
      token = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', token, tokentext);
    ReadObjectEnd(errors, data, eventObject, data.events.Count() - 1, 0x60000000);
    errors.Complete(status);
    return !parse.FoundError();
  }

  BOOL WriteEventObjects(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    if (data.model.animationFile[0]) {
      return 1;
    }
    UINT                   numEvents = data.events.Count();
    int                    needObjIds = numEvents != data.objects.Count();
    const MDLEVENTSECTION *eventObject = data.events.Ptr();
    for (UINT i = numEvents; i; --i, ++eventObject) {
      WriteObjectHeader(data, *eventObject, MDLTOK_EVENTOBJECT, needObjIds, buffer);
      WriteEventKeyFrames(eventObject->eventKeys, buffer);
      WriteObjectTrailer(*eventObject, buffer);
    }
    return 1;
  }

  BOOL WriteBinEventObjects(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *status) {
    UINT numEvents = data.events.Count();
    if (data.model.animationFile[0] || !numEvents) {
      return 1;
    }
    buf.AddDword('STVE');
    UINT totalSize = 4;
    for (UINT n = 0; n < numEvents; ++n) {
      const MDLEVENTSECTION &eventObject = data.events[n];
      UINT                   size = GetBinGenObjectSize(eventObject) + 4;
      size += GetBinEventKeyFramesSize(eventObject.eventKeys);
      totalSize += size;
    }
    buf.AddUint(totalSize);
    buf.AddUint(numEvents);
    for (n = 0; n < numEvents; ++n) {
      const MDLEVENTSECTION &eventObject = data.events[n];
      UINT                   size = GetBinGenObjectSize(eventObject) + 4;
      size += GetBinEventKeyFramesSize(eventObject.eventKeys);
      buf.AddUint(size);
      WriteBinGenObject(eventObject, buf, status);
      WriteBinEventKeyFrames(eventObject.eventKeys, buf);
    }
    return 1;
  }

  BOOL ReadBinEventObjects(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT totalRead = 4;
    UINT numEvents = buf.GetUint();
    data.events.SetCount(0);
    data.events.ReserveSpace(numEvents);
    while (totalRead < length) {
      MDLEVENTSECTION *eventObject = data.events.New();
      if (!eventObject) {
        status->FatalFlunked("EventObject", -1);
        return 0;
      }
      UINT sectionLength = buf.GetUint();
      UINT read = 4;
      if (!ReadBinGenObject(*eventObject, buf, status, read)) {
        status->Add(STATUS_ERROR, "Error reading gen object portion of event.\n");
        return 0;
      }
      if (read < sectionLength) {
        DWORD tag = buf.GetDword();
        read += 4;
        switch (tag) {
          case 'TVEK':
            if (!ReadBinEventKeyFrames(eventObject->eventKeys, buf, &read)) {
              status->Add(STATUS_ERROR, "Error reading event keys portion of event object.\n");
              return 0;
            }
            break;
          default:
            SkipUnknown(buf, read);
            break;
        }
      }
      totalRead += read;
      if (totalRead > length) {
        status->FatalOverran("EventObject section overran read buffer.\n", -1);
        return 0;
      }
      ReadBinObjectEnd(data, eventObject, data.events.Count() - 1, 0x60000000);
    }
    return 1;
  }

}  // namespace MDL
