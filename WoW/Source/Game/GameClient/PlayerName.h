#pragma once

#include "Game/GameClient/WorldText.h"

namespace NTempest {
  class CImVector;
}

struct HPLAYERNAME__;

void __fastcall PlayerNameShow(int show);

void __fastcall PlayerNameCreateText(HPLAYERNAME__ *name, WORLDTEXTTYPE type, const char *text, const NTempest::CImVector *colorOverride);

void __fastcall PlayerNameTriggerNameRegenerate(HPLAYERNAME__ *name);
void __fastcall PlayerNameTriggerColorUpdate(HPLAYERNAME__ *name);
void __fastcall PlayerNameUpdateEarly();
void __fastcall PlayerNameUpdateLate();
void __fastcall PlayerNameRenderWorldText();
