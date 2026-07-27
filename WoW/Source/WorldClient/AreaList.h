#pragma once

namespace NTempest {
  class C3Vector;
}

void AreaListInitialize();
void AreaListShutdown();

int AreaListGetName(unsigned int continentID, unsigned int areaID, unsigned int subAreaID, char *buffer, unsigned int size, int fullName);

void AreaListRegisterLocation(const NTempest::C3Vector &location, unsigned int continent, unsigned long worldObject);

int AreaListZoneHasBreathParticles(unsigned long worldObject, unsigned int continentID, const NTempest::C3Vector &position);
