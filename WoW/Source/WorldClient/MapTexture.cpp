#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"

#include "Base/Status.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"

#include <storm.h>

HTEXTURE CMap::LoadTexture(LPCSTR fileName) {
  CStatus     status;
  CGxTexFlags texFlags(
      CWorld::enables & CWorld::Enable_Anisotropic ? GxTex_Anisotropic
      : CWorld::enables & CWorld::Enable_Trilinear ? GxTex_LinearMipLinear
                                                   : GxTex_LinearMipNearest,
      1, 1, 0, 0, 0, CWorld::texMaxAnisotropy
  );
  HTEXTURE texture = TextureCreate(fileName, texFlags, &status, 0);
  SysMsgAdd(status, 2);
  if (!texture) {
    FATALERROR(("Failed to load texture: %s\n", fileName));
  }
  return texture;
}
