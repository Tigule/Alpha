#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/CMapObj.h"

#include "Gx/Gx.h"
#include "Images/blit.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"

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

  if (cmd == GxTex_Latch) {
    texelStrideInBytes = CalcRowStride(GxGetBlitFormat(LIGHTMAP_FORMAT), w);
    texels = lightmapTex->texels;
  }
}

void CMapObjGroup::CreateLightmaps() {
  lightmapTexFlushTime = 30.0f;

  for (UINT i = 0; i < lightmapTexCount; ++i) {
    SMOLightmapTex &lightmapTex = lightmapTexList[i];
    if (!lightmapTex.hTexture) {
      EGxTexFormat format = LIGHTMAP_FORMAT;
      if (!GxCaps().m_texFmtDxt) {
        format = GxTex_Rgb565;
      }

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
