#include "WorldClient/CMapObj.h"

#include "Services/SysMessage.h"
#include "Services/Texture.h"

void CMapObjGroup::FreeLightmaps() {
  unsigned int freed = 0;

  for (unsigned int i = 0; i < lightmapTexCount; ++i) {
    if (lightmapTexList[i].hTexture) {
      HandleClose(lightmapTexList[i].hTexture);
      lightmapTexList[i].hTexture = 0;
      freed = 1;
    }
  }

  if (freed) {
    SysMsgPrintf(SYSMSG_INFO, 2, "FREELIGHTMAPS|%d", lightmapTexCount);
  }
}
