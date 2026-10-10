#include "GenObject.h"
#include "MDLStatus.h"
#include "Parser.h"
#include "TSet.h"
#include "Base/MsgBuffer.h"


namespace MDL {
  LPCSTR       TokenText(UINT token);
  void __cdecl WriteLine(TSGrowableArray<char> &buffer, LPCSTR format, ...);
  BOOL         ReadCamera(Parser &, MDLDATA &, CMDLStatus *);
  BOOL         WriteCameras(const MDLDATA &, TSGrowableArray<char> &, CMDLStatus *);
  BOOL         WriteBinCameras(const MDLDATA &, CMsgBuffer &, CMDLStatus *);
  BOOL         ReadBinCameras(CMsgBuffer &, UINT, MDLDATA &, CMDLStatus *);
}  // namespace MDL

static void AddCameraErrors(TSet &errors) {
  errors.Add(MDLTOK_POSITION, 0, 0);
  errors.Add(MDLTOK_TRANSLATION, 0, 0);
  errors.Add(MDLTOK_ROTATION, 0, 0);
  errors.Add(MDLTOK_FIELDOFVIEW, 1, 0);
  errors.Add(MDLTOK_FAR_CLIP, 1, 0);
  errors.Add(MDLTOK_NEAR_CLIP, 0, 0);
  errors.Add(MDLTOK_TARGET, 0, 0);
  errors.Add(MDLTOK_VISIBILITY, 0, 0);
}

static void AddCameraTargetErrors(TSet &errors) {
  errors.Add(MDLTOK_POSITION, 0, 0);
  errors.Add(MDLTOK_TRANSLATION, 0, 0);
}

static void IReadCameraTarget(Parser &parse, MDLTARGETSECTION *target, CMDLStatus *status) {
  FATALASSERT(target);
  TSet errors;
  AddCameraTargetErrors(errors);
  parse.Expect('{');
  LPCSTR tokenText;
  UINT   token;
  for (token = parse.Token(&tokenText, 0); token != '}'; token = parse.Token(&tokenText, 0)) {
    if (!token) {
      break;
    }
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokenText);
    }
    switch (token) {
      case MDLTOK_TRANSLATION:
        ReadObjectFloatKeyframes(parse, &target->transkeys);
        break;
      case MDLTOK_POSITION:
        ReadFloatKeyData(parse, &target->pivot.x, 3);
        parse.Expect(',');
        break;
      default:
        parse.FatalUnexpected(tokenText);
        parse.Expect(',');
        break;
    }
  }
  parse.Expect('}', token, tokenText);
  errors.Complete(status);
}

BOOL MDL::ReadCamera(Parser &parse, MDLDATA &data, CMDLStatus *status) {
  TSet              errors;
  MDLCAMERASECTION *camera = data.cameras.New();
  AddCameraErrors(errors);
  ReadObjectName(parse, camera->name);
  parse.Expect('{');
  LPCSTR tokentext;
  UINT   token = parse.Token(&tokentext, 0);
  while (token != '}' && token) {
    if (!errors.Check(token)) {
      parse.FatalDuplicate(tokentext);
    }
    switch (token) {
      case MDLTOK_POSITION:
        ReadFloatKeyData(parse, &camera->pivot.x, 3);
        parse.Expect(',');
        break;
      case MDLTOK_TRANSLATION:
        ReadObjectFloatKeyframes(parse, &camera->transkeys);
        break;
      case MDLTOK_ROTATION:
        ReadObjectFloatKeyframes(parse, &camera->rollkeys);
        break;
      case MDLTOK_FIELDOFVIEW:
        camera->fieldOfView = parse.ExpectFloat();
        parse.Expect(',');
        break;
      case MDLTOK_FAR_CLIP:
        camera->farClip = parse.ExpectFloat();
        parse.Expect(',');
        break;
      case MDLTOK_NEAR_CLIP:
        camera->nearClip = parse.ExpectFloat();
        parse.Expect(',');
        break;
      case MDLTOK_TARGET:
        IReadCameraTarget(parse, &camera->target, status);
        break;
      case MDLTOK_VISIBILITY:
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
  MDL::WriteLine(buffer, "%s \"%s\" {\n", MDL::TokenText(MDLTOK_CAMERA), (LPCSTR)section.name);
  MDL::WriteLine(buffer, "\t%s { %g, %g, %g },\n", MDL::TokenText(MDLTOK_POSITION), section.pivot.x, section.pivot.y, section.pivot.z);
  WriteFloatKeyFrames(MDLTOK_TRANSLATION, "\t", section.transkeys, buffer);
  WriteFloatKeyFrames(MDLTOK_ROTATION, "\t", section.rollkeys, buffer);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_FIELDOFVIEW), section.fieldOfView);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_FAR_CLIP), section.farClip);
  MDL::WriteLine(buffer, "\t%s %g,\n", MDL::TokenText(MDLTOK_NEAR_CLIP), section.nearClip);
  MDL::WriteLine(buffer, "\t%s {\n", MDL::TokenText(MDLTOK_TARGET));
  MDL::WriteLine(buffer, "\t\t%s { %g, %g, %g },\n", MDL::TokenText(MDLTOK_POSITION), section.target.pivot.x, section.target.pivot.y, section.target.pivot.z);
  WriteFloatKeyFrames(MDLTOK_TRANSLATION, "\t\t", section.target.transkeys, buffer);
  MDL::WriteLine(buffer, "\t}\n");
  WriteFloatKeyFrames(MDLTOK_VISIBILITY, "\t", section.visibilityKeys, buffer);
  MDL::WriteLine(buffer, "}\n");
}

BOOL MDL::WriteCameras(const MDLDATA &data, TSGrowableArray<char> &buffer, CMDLStatus *) {
  const MDLCAMERASECTION *pCamera = data.cameras.Ptr();
  for (UINT i = data.cameras.Count(); i; --i, ++pCamera) {
    IWriteCamera(*pCamera, buffer);
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
  WriteBinFloatKeyFrames(section.transkeys, 'RTCK', buffer);
  WriteBinFloatKeyFrames(section.rollkeys, 'LRCK', buffer);
  WriteBinFloatKeyFrames(section.target.transkeys, 'RTTK', buffer);
  WriteBinFloatKeyFrames(section.visibilityKeys, 'SIVK', buffer);
}

BOOL MDL::WriteBinCameras(const MDLDATA &data, CMsgBuffer &buf, CMDLStatus *) {
  UINT numCameras = data.cameras.Count();
  if (!numCameras) {
    return 1;
  }
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
  return 1;
}

BOOL ReadBinCamera(CMsgBuffer &buffer, MDLCAMERASECTION *camera, CMDLStatus *status, UINT &totalLength) {
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

BOOL MDL::ReadBinCameras(CMsgBuffer &buf, UINT length, MDLDATA &data, CMDLStatus *status) {
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
