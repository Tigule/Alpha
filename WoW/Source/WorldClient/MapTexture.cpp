#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "WorldClient/Map.h"

#include "Base/Status.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"

#include <storm.h>

HTEXTURE CMap::LoadTexture(LPCSTR fileName) {
  CStatus     status;
  EGxTexFilter filter = GxTex_LinearMipNearest;
  if (CWorld::enables & CWorld::Enable_Anisotropic) {
    filter = GxTex_Anisotropic;
  } else if (CWorld::enables & CWorld::Enable_Trilinear) {
    filter = GxTex_LinearMipLinear;
  }

  CGxTexFlags texFlags(filter, 1, 1, 0, 0, 0, CWorld::texMaxAnisotropy);
  HTEXTURE texture = TextureCreate(fileName, texFlags, &status, 0);
  SysMsgAdd(status, 2);
  if (!texture) {
    FATALERROR(("Failed to load texture: %s\n", fileName));
  }
  return texture;
}
