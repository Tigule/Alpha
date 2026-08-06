#include "MDLFile/MDLTypes.h"
#include "Base/Status.h"

int MdlReadValidate(const MDLDATA &data, CStatus *status) {
  if (!data.geosets.Count() && !data.lights.Count() && !data.attachments.Count() && !data.cameras.Count() && !data.particleEmitters2.Count() &&
      !data.ribbonEmitters.Count())
  {
    status->Add(STATUS_WARNING, "File contains no geosets, lights, sound emitters, attachments, cameras, particle emitters, or ribbon emitters.\n");
  }

  return 1;
}
