#pragma once

#include "Game/GameClient/WorldText.h"

namespace NTempest {
  class C3Vector;
  class CImVector;
}  // namespace NTempest

struct HPLAYERNAME__;
class CGUnit_C;

HPLAYERNAME__ *PlayerNameCreate(CGUnit_C *unitPtr);
void           PlayerNameShow(int show);

void PlayerNameCreateText(HPLAYERNAME__ *name, WORLDTEXTTYPE type, LPCSTR text, const NTempest::CImVector *colorOverride);

void PlayerNameTriggerNameRegenerate(HPLAYERNAME__ *name);
void PlayerNameTriggerColorUpdate(HPLAYERNAME__ *name);
void PlayerNameChangeLocation(HPLAYERNAME__ *name, const NTempest::C3Vector &namePosition);
void PlayerNameUpdateEarly();
void PlayerNameUpdateLate();
void PlayerNameUpdateWorldText(HPLAYERNAME__ *name);
void PlayerNameRenderWorldText();
