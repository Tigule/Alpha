#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"

namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);

  static void AddCameraErrors(TSet &errors) {
    errors.Add(0x1A5, 0, 0);
    errors.Add(0x1C7, 0, 0);
    errors.Add(0x1AD, 0, 0);
    errors.Add(0x14A, 1, 0);
    errors.Add(0x149, 1, 0);
    errors.Add(0x176, 0, 0);
    errors.Add(0x1C1, 0, 0);
    errors.Add(0x1D9, 0, 0);
  }

  static void AddCameraTargetErrors(TSet &errors) {
    errors.Add(0x1A5, 0, 0);
    errors.Add(0x1C7, 0, 0);
  }

  static void IReadCameraTarget(Parser &parse, MDLTARGETSECTION *target, CMDLStatus *status) {
    FATALASSERT(target);
    TSet errors;
    AddCameraTargetErrors(errors);
    parse.Expect('{');
    LPCSTR tokenText;
    UINT   token = parse.Token(&tokenText, 0);
    while (token && token != '}') {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokenText);
      }
      if (token == 0x1A5) {
        ReadFloatKeyData(parse, &target->pivot.x, 3);
        parse.Expect(',');
      } else if (token == 0x1C7) {
        ReadObjectFloatKeyframes(parse, &target->transkeys);
      } else {
        parse.FatalUnexpected(tokenText);
        parse.Expect(',');
      }
      token = parse.Token(&tokenText, 0);
    }
    parse.Expect('}', token, tokenText);
    errors.Complete(status);
  }

  int ReadCamera(Parser &parse, MDLDATA &data, CMDLStatus *status) {
    TSet              errors;
    MDLCAMERASECTION *camera = data.cameras.New();
    AddCameraErrors(errors);
    ReadObjectName(parse, camera->name);
    parse.Expect('{');
    LPCSTR tokentext;
    UINT   token = parse.Token(&tokentext, 0);
    while (token && token != '}') {
      if (!errors.Check(token)) {
        parse.FatalDuplicate(tokentext);
      }
      switch (token) {
        case 0x149:
          camera->farClip = parse.ExpectFloat();
          parse.Expect(',');
          break;
        case 0x14A:
          camera->fieldOfView = parse.ExpectFloat();
          parse.Expect(',');
          break;
        case 0x176:
          camera->nearClip = parse.ExpectFloat();
          parse.Expect(',');
          break;
        case 0x1A5:
          ReadFloatKeyData(parse, &camera->pivot.x, 3);
          parse.Expect(',');
          break;
        case 0x1AD:
          ReadObjectFloatKeyframes(parse, &camera->rollkeys);
          break;
        case 0x1C1:
          IReadCameraTarget(parse, &camera->target, status);
          break;
        case 0x1C7:
          ReadObjectFloatKeyframes(parse, &camera->transkeys);
          break;
        case 0x1D9:
          ReadObjectFloatKeyframes(parse, &camera->visibilityKeys);
          break;
        default:
          parse.FatalUnexpected(tokentext);
          parse.Expect(',');
          break;
      }
      token = parse.Token(&tokentext, 0);
    }
    parse.Expect('}', token, tokentext);
    errors.Complete(status);
    return !parse.FoundError();
  }

  static void IWriteCamera(const MDLCAMERASECTION &section, TSGrowableArray<char> &buffer) {
    WriteLine(buffer, "%s \"%s\" {\n", TokenText(0x114), static_cast<LPCSTR>(section.name));
    WriteLine(buffer, "\t%s { %g, %g, %g },\n", TokenText(0x1A5), section.pivot.x, section.pivot.y, section.pivot.z);
    if (section.transkeys.keys.Count()) {
      WriteLine(buffer, "\t%s %d {\n", TokenText(0x1C7), section.transkeys.keys.Count());
      WriteTrackHeader("\t", section.transkeys, buffer);
      for (UINT i = 0; i < section.transkeys.keys.Count(); ++i) {
        const MDLKEYFRAME<NTempest::C3Vector> &key = section.transkeys.keys.Ptr()[i];
        WriteLine(buffer, "\t\t%d: ", key.time);
        WriteKeyData(buffer, &key.value.x, 3);
        if (section.transkeys.type > TRACK_LINEAR) {
          WriteLine(buffer, "\t\t\t%s ", TokenText(0x15E));
          WriteKeyData(buffer, &key.inTan.x, 3);
          WriteLine(buffer, "\t\t\t%s ", TokenText(0x18A));
          WriteKeyData(buffer, &key.outTan.x, 3);
        }
      }
      WriteLine(buffer, "\t}\n");
    }
    WriteFloatKeyFrames(0x1AD, "\t", section.rollkeys, buffer);
    WriteLine(buffer, "\t%s %g,\n", TokenText(0x14A), section.fieldOfView);
    WriteLine(buffer, "\t%s %g,\n", TokenText(0x149), section.farClip);
    WriteLine(buffer, "\t%s %g,\n", TokenText(0x176), section.nearClip);
    WriteLine(buffer, "\t%s {\n", TokenText(0x1C1));
    WriteLine(buffer, "\t\t%s { %g, %g, %g },\n", TokenText(0x1A5), section.target.pivot.x, section.target.pivot.y, section.target.pivot.z);
    if (section.target.transkeys.keys.Count()) {
      WriteLine(buffer, "\t\t%s %d {\n", TokenText(0x1C7), section.target.transkeys.keys.Count());
      WriteTrackHeader("\t\t", section.target.transkeys, buffer);
      for (UINT i = 0; i < section.target.transkeys.keys.Count(); ++i) {
        const MDLKEYFRAME<NTempest::C3Vector> &key = section.target.transkeys.keys.Ptr()[i];
        WriteLine(buffer, "\t\t\t%d: ", key.time);
        WriteKeyData(buffer, &key.value.x, 3);
        if (section.target.transkeys.type > TRACK_LINEAR) {
          WriteLine(buffer, "\t\t\t\t%s ", TokenText(0x15E));
          WriteKeyData(buffer, &key.inTan.x, 3);
          WriteLine(buffer, "\t\t\t\t%s ", TokenText(0x18A));
          WriteKeyData(buffer, &key.outTan.x, 3);
        }
      }
      WriteLine(buffer, "\t\t}\n");
    }
    WriteLine(buffer, "\t}\n");
    WriteFloatKeyFrames(0x1D9, "\t", section.visibilityKeys, buffer);
    WriteLine(buffer, "}\n");
  }

  int WriteCameras(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
    for (UINT i = 0; i < data.cameras.Count(); ++i) {
      IWriteCamera(data.cameras.Ptr()[i], buffer);
    }
    return 1;
  }

  static UINT GetBinCameraSize(const MDLCAMERASECTION &section) {
    UINT size = 120;
    if (section.transkeys.keys.Count()) {
      UINT dataSize = section.transkeys.type > TRACK_LINEAR ? 36 : 12;
      size += 16 + section.transkeys.keys.Count() * (4 + dataSize);
    }
    if (section.rollkeys.keys.Count()) {
      UINT dataSize = section.rollkeys.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.rollkeys.keys.Count() * (4 + dataSize);
    }
    if (section.target.transkeys.keys.Count()) {
      UINT dataSize = section.target.transkeys.type > TRACK_LINEAR ? 36 : 12;
      size += 16 + section.target.transkeys.keys.Count() * (4 + dataSize);
    }
    if (section.visibilityKeys.keys.Count()) {
      UINT dataSize = section.visibilityKeys.type > TRACK_LINEAR ? 12 : 4;
      size += 16 + section.visibilityKeys.keys.Count() * (4 + dataSize);
    }
    return size;
  }

  static void IWriteBinCamera(const MDLCAMERASECTION &section, CMsgBuffer &buffer) {
    buffer.AddUint(GetBinCameraSize(section));
    buffer.AddTcharArray(section.name, 80, 1);
    buffer.AddFloatArray(&section.pivot.x, 3);
    buffer.AddFloat(section.fieldOfView);
    buffer.AddFloat(section.farClip);
    buffer.AddFloat(section.nearClip);
    buffer.AddFloatArray(&section.target.pivot.x, 3);
    if (section.transkeys.keys.Count()) {
      buffer.AddDword('RTCK');
      buffer.AddUint(section.transkeys.keys.Count());
      buffer.AddUint(section.transkeys.type);
      buffer.AddUint(section.transkeys.globalSeqId);
      UINT values = section.transkeys.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < section.transkeys.keys.Count(); ++i) {
        const MDLKEYFRAME<NTempest::C3Vector> &key = section.transkeys.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.x, values);
      }
    }
    WriteBinFloatKeyFrames(section.rollkeys, 'LRCK', buffer);
    if (section.target.transkeys.keys.Count()) {
      buffer.AddDword('RTTK');
      buffer.AddUint(section.target.transkeys.keys.Count());
      buffer.AddUint(section.target.transkeys.type);
      buffer.AddUint(section.target.transkeys.globalSeqId);
      UINT values = section.target.transkeys.type > TRACK_LINEAR ? 9 : 3;
      for (UINT i = 0; i < section.target.transkeys.keys.Count(); ++i) {
        const MDLKEYFRAME<NTempest::C3Vector> &key = section.target.transkeys.keys.Ptr()[i];
        buffer.AddInt(key.time);
        buffer.AddFloatArray(&key.value.x, values);
      }
    }
    WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
  }

  int WriteBinCameras(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
    UINT numCameras = data.cameras.Count();
    if (numCameras) {
      buf.AddDword('SMAC');
      UINT totalSize = 4;
      UINT i;
      for (i = 0; i < data.cameras.Count(); ++i) {
        totalSize += GetBinCameraSize(data.cameras[i]);
      }
      buf.AddUint(totalSize);
      buf.AddUint(numCameras);
      for (i = 0; i < numCameras; ++i) {
        IWriteBinCamera(data.cameras[i], buf);
      }
    }
    return 1;
  }

  int ReadBinCamera(CMsgBuffer &buffer, MDLCAMERASECTION *camera, CMDLStatus *status, UINT &totalLength) {
    UINT sectionLength = buffer.GetUint();
    buffer.GetTcharArray(camera->name, 80);
    buffer.GetFloatArray(&camera->pivot.x, 3);
    camera->fieldOfView = buffer.GetFloat();
    camera->farClip = buffer.GetFloat();
    camera->nearClip = buffer.GetFloat();
    buffer.GetFloatArray(&camera->target.pivot.x, 3);
    UINT localRead = 120;
    while (localRead < sectionLength) {
      DWORD tag = buffer.GetDword();
      localRead += 4;
      if (tag == 'RTCK') {
        if (!ReadBinFloatKeyFrames(camera->transkeys, buffer, localRead)) {
          status->Add(STATUS_ERROR, "Error reading translation keys in camera section.\n");
          return 0;
        }
      } else if (tag == 'LRCK') {
        if (!ReadBinFloatKeyFrames(camera->rollkeys, buffer, localRead)) {
          status->Add(STATUS_ERROR, "Error reading roll keys in camera section.\n");
          return 0;
        }
      } else if (tag == 'RTTK') {
        if (!ReadBinFloatKeyFrames(camera->target.transkeys, buffer, localRead)) {
          status->Add(STATUS_ERROR, "Error reading target translation keys in camera section.\n");
          return 0;
        }
      } else if (tag == 'SIVK') {
        if (!ReadBinFloatKeyFrames(camera->visibilityKeys, buffer, localRead)) {
          status->Add(STATUS_ERROR, "Error reading visibility keys in camera section.\n");
          return 0;
        }
      } else {
        SkipUnknown(buffer, localRead);
      }
    }
    if (localRead > sectionLength) {
      status->FatalOverran("Camera", -1);
      return 0;
    }
    totalLength += localRead;
    return 1;
  }

  int ReadBinCameras(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
    UINT numCameras;
    numCameras = buf.GetUint();
    UINT totalRead = 4;
    data.cameras.SetCount(0);
    data.cameras.ReserveSpace(numCameras);
    MDLCAMERASECTION *pCam;
    while (totalRead < length) {
      pCam = data.cameras.New();
      if (!ReadBinCamera(buf, pCam, status, totalRead)) {
        status->Add(STATUS_ERROR, "Failure reading camera section.\n");
        return 0;
      }
    }
    if (totalRead > length) {
      status->FatalOverran("Camera", -1);
      return 0;
    }
    return 1;
  }

}  // namespace MDL
