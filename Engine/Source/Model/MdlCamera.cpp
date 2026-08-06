#include <Base/Base.h>

#include "Model/ModelInternal.h"

#include "MDLFile/MDLTypes.h"
#include "Services/Camera.h"

#include <string.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

int MdlReadCameras(const MDLDATA &data, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(cameras);

  cameras->SetCount(data.cameras.Count());
  memset(cameras->Ptr(), 0, cameras->Count() * sizeof(HCAMERA));

  for (UINT i = 0; i < data.cameras.Count(); ++i) {
    const MDLCAMERASECTION &source = data.cameras[i];
    HCAMERA                 camera = CameraCreate();

    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 4, source.fieldOfView);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 2, source.farClip);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 3, source.nearClip);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 5, 0.0f);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 7, source.pivot, 0);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 8, source.target.pivot, 0);

    (*cameras)[i] = camera;
  }

  return 1;
}

void MdxReadCameras(BYTE *data, UINT fileBytes, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(data);
  ASSERT(cameras);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, 0x534D4143);
  if (!section) {
    return;
  }

  fileBytes = *reinterpret_cast<UINT *>(section) - 4;
  UINT  numCameras = *reinterpret_cast<UINT *>(section + 4);
  BYTE *cameraData = section + 8;

  cameras->SetCount(numCameras);

  UINT i;
  for (i = 0; i < numCameras; ++i) {
    UINT   bytesThisCamera = *reinterpret_cast<UINT *>(cameraData);
    float *values = reinterpret_cast<float *>(cameraData + 0x54);

    HCAMERA camera = CameraCreate();

    NTempest::C3Vector point(values[0], values[1], values[2]);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 7, point, 0);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 4, values[3]);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 2, values[4]);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 3, values[5]);

    point = NTempest::C3Vector(values[6], values[7], values[8]);
    DataMgrSetCoord(reinterpret_cast<HDATAMGR>(camera), 8, point, 0);
    DataMgrSetFloat(reinterpret_cast<HDATAMGR>(camera), 5, 0.0f);

    (*cameras)[i] = camera;

    ASSERT(fileBytes >= bytesThisCamera);
    fileBytes -= bytesThisCamera;
    cameraData += bytesThisCamera;
  }

  ASSERT(fileBytes == 0);
}
