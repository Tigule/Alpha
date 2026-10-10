#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "MDLFile/MDLTypes.h"
#include "Services/Camera.h"

#include <string.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

BOOL MdlReadCameras(const MDLDATA &data, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(cameras);

  cameras->SetCount(data.cameras.Count());
  memset(cameras->Ptr(), 0, cameras->Count() * sizeof(HCAMERA));

  for (UINT i = 0; i < data.cameras.Count(); ++i) {
    const MDLCAMERASECTION &source = data.cameras[i];
    HCAMERA                 camera = CameraCreate();

    DataMgrSetFloat(camera, 4, source.fieldOfView);
    DataMgrSetFloat(camera, 2, source.farClip);
    DataMgrSetFloat(camera, 3, source.nearClip);
    DataMgrSetFloat(camera, 5, 0.0f);
    DataMgrSetCoord(camera, 7, source.pivot, 0);
    DataMgrSetCoord(camera, 8, source.target.pivot, 0);

    (*cameras)[i] = camera;
  }

  return 1;
}

void MdxReadCameras(BYTE *data, UINT fileBytes, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(data);
  ASSERT(cameras);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'SMAC');
  if (!section) {
    return;
  }

  UINT  sectionBytes = *(UINT *)section - 4;
  UINT  numCameras = *(UINT *)(section + 4);
  BYTE *cameraData = section + 8;

  cameras->SetCount(numCameras);

  UINT i;
  for (i = 0; i < numCameras; ++i) {
    UINT   bytesThisCamera = *(UINT *)cameraData;
    float *values = (float *)(cameraData + 0x54);

    HCAMERA camera = CameraCreate();

    NTempest::C3Vector point(values[0], values[1], values[2]);
    DataMgrSetCoord(camera, 7, point, 0);
    DataMgrSetFloat(camera, 4, values[3]);
    DataMgrSetFloat(camera, 2, values[4]);
    DataMgrSetFloat(camera, 3, values[5]);

    point = NTempest::C3Vector(values[6], values[7], values[8]);
    DataMgrSetCoord(camera, 8, point, 0);
    DataMgrSetFloat(camera, 5, 0.0f);

    (*cameras)[i] = camera;

    ASSERT(sectionBytes >= bytesThisCamera);
    sectionBytes -= bytesThisCamera;
    cameraData += bytesThisCamera;
  }

  ASSERT(sectionBytes == 0);
}
