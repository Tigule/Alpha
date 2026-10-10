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

#include "Map.h"

#include "WorldCommon/WorldMath.h"

CMapBaseObj::CMapBaseObj() {
  refCount = 0;
  flags = 0;
  type = Type_BaseObj;
}

CMapBaseObj::~CMapBaseObj() {
  ASSERT(parentLinkList.Head() == 0);
}

void CMapBaseObj::SelectLights() {
  SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE);
}

int CMapBaseObj::TestAABox(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1) {
  return CWorldMath::VectorIntersectAABox2(aaBox, v0, v1);
}
