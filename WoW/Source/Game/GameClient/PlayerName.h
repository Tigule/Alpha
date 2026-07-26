#pragma once

#include "Game/GameClient/WorldText.h"

namespace NTempest {
  class C3Vector;
  class CImVector;
}

struct HPLAYERNAME__;
class CGUnit_C;

HPLAYERNAME__ *__fastcall PlayerNameCreate(CGUnit_C *unitPtr);
void __fastcall PlayerNameShow(int show);

void __fastcall PlayerNameCreateText(HPLAYERNAME__ *name, WORLDTEXTTYPE type, const char *text, const NTempest::CImVector *colorOverride);

void __fastcall PlayerNameTriggerNameRegenerate(HPLAYERNAME__ *name);
void __fastcall PlayerNameTriggerColorUpdate(HPLAYERNAME__ *name);
void __fastcall PlayerNameChangeLocation(HPLAYERNAME__ *name, const NTempest::C3Vector &namePosition);
void __fastcall PlayerNameUpdateEarly();
void __fastcall PlayerNameUpdateLate();
void __fastcall PlayerNameUpdateWorldText(HPLAYERNAME__ *name);
void __fastcall PlayerNameRenderWorldText();
