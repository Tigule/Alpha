#include "Model/ModelInternal.h"

#include "Services/Camera.h"

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

void __fastcall MdxReadCameras(unsigned char *data, unsigned int fileBytes, TSFixedArray<HCAMERA> *cameras) {
  ASSERT(data);
  ASSERT(cameras);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x534D4143);
  if (!section) {
    return;
  }

  unsigned int   sectionBytes = *reinterpret_cast<unsigned int *>(section) - 4;
  unsigned int   numCameras = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *cameraData = section + 8;

  cameras->SetCount(numCameras);

  for (unsigned int i = 0; i < numCameras; ++i) {
    unsigned int bytesThisCamera = *reinterpret_cast<unsigned int *>(cameraData);
    float       *values = reinterpret_cast<float *>(cameraData + 0x54);

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

    ASSERT(sectionBytes >= bytesThisCamera);
    sectionBytes -= bytesThisCamera;
    cameraData += bytesThisCamera;
  }

  ASSERT(sectionBytes == 0);
}
