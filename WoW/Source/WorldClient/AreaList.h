#pragma once

namespace NTempest {
  class C3Vector;
}

void __fastcall AreaListInitialize();
void __fastcall AreaListShutdown();

int __fastcall AreaListGetName(unsigned int continentID, unsigned int areaID, unsigned int subAreaID, char *buffer, unsigned int size, int fullName);

void __fastcall AreaListRegisterLocation(const NTempest::C3Vector &location, unsigned int continent, unsigned long worldObject);

int __fastcall AreaListZoneHasBreathParticles(unsigned long worldObject, unsigned int continentID, const NTempest::C3Vector &position);
