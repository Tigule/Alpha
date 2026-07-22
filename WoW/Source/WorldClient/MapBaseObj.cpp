#include "Map.h"

#include "Base/Base.h"

CMapBaseObj::CMapBaseObj() {
  rot.x = 0.0f;
  rot.y = 0.0f;
  rot.z = 0.0f;
  rot.w = 1.0f;
  aaSphere.r = 0.0f;
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
