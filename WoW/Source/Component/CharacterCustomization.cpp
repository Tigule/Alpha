#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <BLPFile/blp.h>
#include "Object/ObjectClient/Unit_C.h"

#include "../../Common/ComponentCore/CharacterCustomizationCore.cpp"

void CharCustomizationCommitItemGeosets(HCHARGEOSET geosetHandle, int doNotCommitGeosets, HMODEL paperDollModel) {
  CCharGeoset *geoset = (CCharGeoset *)geosetHandle;
  if (geoset) {
    CharCustomizationCommitItemGeosets(geosetHandle, doNotCommitGeosets);
    if (paperDollModel) {
      int group;
      for (group = 0; group < 9; ++group) {
        UINT geosetNumber = geoset->m_geosetInfo.currentGeosets[group];
        if (geoset->m_workingGeosetInfo.flags[group] & 1) {
          geoset->HideGeosetSection(paperDollModel, s_clothingGeosetRanges[group]);
        } else {
          geoset->ShowGeosetSection(paperDollModel, s_clothingGeosetRanges[group], geoset->m_geosetInfo.flags[group] & 2 ? 1 : geosetNumber, 0);
        }
      }
    }
  }
}

void CCharGeoset::ShowGeosetSection(HMODEL model, CHARACTER_GEOSET_SECTIONS section, UINT geosetNumber, int hideRemainder) const {
  ASSERT(section < NUM_CHARGEOSETS);
  ASSERT(model);
  ASSERT(geosetNumber < 100);

  if (hideRemainder) {
    HideGeosetSection(model, section);
  }
  if (!geosetNumber && !g_charGeosetIsOverlay[section]) {
    geosetNumber = 1;
  }
  if (geosetNumber != m_currentGeosets[section] && m_currentGeosets[section] && !hideRemainder) {
    ModelHideGeosets(model, 100 * section + m_currentGeosets[section], 1);
  }
}

void CCharGeoset::HideGeosetSection(HMODEL model, CHARACTER_GEOSET_SECTIONS section) const {
  ASSERT(section < NUM_CHARGEOSETS);
  ASSERT(model);
  ModelHideGeosetsRange(model, 100 * section + 1, 100 * section + 99, 1);
}
