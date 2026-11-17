#include <Base/Base.h>

#include "Model/IModel.h"

#include "MDLFile/MDLTypes.h"
#include "Services/Camera.h"

#include <string.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

BOOL MdlReadCameras(const MDLDATA &data, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(cameras);

  cameras->SetCount(data.cameras.Count());
  memset(cameras->Ptr(), 0, cameras->Count() * sizeof(HCAMERA));

  HCAMERA                *dest = cameras->Ptr();
  const MDLCAMERASECTION *source = data.cameras.Ptr();
  for (UINT i = data.cameras.Count(); i; --i, ++source, ++dest) {
    HCAMERA camera = CameraCreate();
    DataMgrSetFloat(camera, 4, source->fieldOfView);
    DataMgrSetFloat(camera, 2, source->farClip);
    DataMgrSetFloat(camera, 3, source->nearClip);
    DataMgrSetFloat(camera, 5, 0.0f);
    DataMgrSetCoord(camera, 7, source->pivot, 0);
    DataMgrSetCoord(camera, 8, source->target.pivot, 0);
    *dest = camera;
  }

  return TRUE;
}

void MdxReadCameras(BYTE *data, UINT fileBytes, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(data);
  ASSERT(cameras);

  data = MDLFileBinarySeek(data, fileBytes, 'SMAC');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data;
  data += 4;

  UINT numCameras = *(UINT *)data;
  data += 4;

  sectionBytes -= 4;

  cameras->SetCount(numCameras);

  for (UINT i = 0; i < numCameras; ++i) {
    UINT  bytesThisCamera = *(UINT *)data;
    BYTE *cameraData = data;

    cameraData += 0x54;

    HCAMERA camera = CameraCreate();

    NTempest::C3Vector point;
    point.x = *(float *)cameraData;
    cameraData += 4;
    point.y = *(float *)cameraData;
    cameraData += 4;
    point.z = *(float *)cameraData;
    cameraData += 4;
    DataMgrSetCoord(camera, 7, point, 0);

    DataMgrSetFloat(camera, 4, *(float *)cameraData);
    cameraData += 4;

    DataMgrSetFloat(camera, 2, *(float *)cameraData);
    cameraData += 4;

    DataMgrSetFloat(camera, 3, *(float *)cameraData);
    cameraData += 4;

    point.x = *(float *)cameraData;
    cameraData += 4;
    point.y = *(float *)cameraData;
    cameraData += 4;
    point.z = *(float *)cameraData;
    cameraData += 4;
    DataMgrSetCoord(camera, 8, point, 0);

    DataMgrSetFloat(camera, 5, 0.0f);
    (*cameras)[i] = camera;

    data += bytesThisCamera;

    ASSERT(sectionBytes >= bytesThisCamera);
    sectionBytes -= bytesThisCamera;
  }

  ASSERT(sectionBytes == 0);
}
