#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "Images/blit.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"

void CMapObjGroup::CreateLightmaps() {
  lightmapTexFlushTime = 30.0f;

  for (UINT i = 0; i < lightmapTexCount; ++i) {
    SMOLightmapTex &lightmapTex = lightmapTexList[i];
    if (!lightmapTex.hTexture) {
      EGxTexFormat format = GxCaps().m_texFmtDxt ? LIGHTMAP_FORMAT : GxTex_Rgb565;

      lightmapTex.hTexture = TextureCreate("Lightmap", 256, 256, format, LIGHTMAP_FORMAT, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1));
      CGxTex *texture = TextureGetGxTex(lightmapTex.hTexture, 1, 0);
      GxTexSetUserData(texture, UpdateLightmapTex, &lightmapTex);
    }
  }
}

void CMapObjGroup::FreeLightmaps() {
  UINT freed = 0;

  UINT i;
  for (i = 0; i < lightmapTexCount; ++i) {
    SMOLightmapTex &lightmapTex = lightmapTexList[i];
    if (lightmapTex.hTexture) {
      HandleClose(lightmapTex.hTexture);
      lightmapTex.hTexture = 0;
      freed = 1;
    }
  }

  if (freed) {
    SysMsgPrintf(SYSMSG_INFO, 2, "FREELIGHTMAPS|%d", lightmapTexCount);
  }
}

void CMapObjGroup::UpdateLightmapTex(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  SMOLightmapTex *lightmapTex = static_cast<SMOLightmapTex *>(userArg);
  FATALASSERT(lightmapTex);

  switch (cmd) {
    case GxTex_Lock:
      return;

    case GxTex_Latch:
      texelStrideInBytes = CalcRowStride(GxGetBlitFormat(LIGHTMAP_FORMAT), w);
      texels = lightmapTex->texels;
      return;
  }
}
