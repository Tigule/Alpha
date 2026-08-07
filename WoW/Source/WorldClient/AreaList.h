#pragma once

namespace NTempest {
  class C3Vector;
}

void AreaListInitialize();
void AreaListShutdown();

BOOL AreaListGetName(UINT continentID, UINT areaID, UINT subAreaID, char *buffer, UINT size, int fullName);

void AreaListRegisterLocation(const NTempest::C3Vector &location, UINT continent, DWORD worldObject);

int AreaListZoneHasBreathParticles(DWORD worldObject, UINT continentID, const NTempest::C3Vector &position);
