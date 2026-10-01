#pragma once

#include "Game/GameClient/WorldText.h"

namespace NTempest {
  class C3Vector;
  class CImVector;
}  // namespace NTempest

DECLARE_DERIVED_HANDLE(HPLAYERNAME, HOBJECT);
class CGUnit_C;

HPLAYERNAME PlayerNameCreate(CGUnit_C *unitPtr);
void           PlayerNameShow(int show);

void PlayerNameCreateText(HPLAYERNAME name, WORLDTEXTTYPE type, LPCSTR text, const NTempest::CImVector *colorOverride);

void PlayerNameTriggerNameRegenerate(HPLAYERNAME name);
void PlayerNameTriggerColorUpdate(HPLAYERNAME name);
void PlayerNameChangeLocation(HPLAYERNAME name, const NTempest::C3Vector &namePosition);
void PlayerNameUpdateEarly();
void PlayerNameUpdateLate();
void PlayerNameUpdateWorldText(HPLAYERNAME name);
void PlayerNameRenderWorldText();
