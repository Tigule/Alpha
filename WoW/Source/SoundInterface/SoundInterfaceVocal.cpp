#include <Base/Base.h>
#include <WowConst.h>

#include "SoundInterface.h"

#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/SoundCharacterMacroLinesRec.h"

#include <FrameScript/FrameScript.h>
#include <stpl.h>

#include <lauxlib.h>
#include <lua.h>

struct MACRODESC {
  MACRODESC() {
    memset(soundID, 0, sizeof(soundID));
  }

  UINT soundID[12][3];
};

static TSGrowableArray<MACRODESC> s_macroRaceDescs;

static int Script_PlayVocalCategory(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PlayVocalCategory(category)");
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->PlayVocalMacro(static_cast<int>(lua_tonumber(L, 1)));
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[1] = {
    {"PlayVocalCategory", Script_PlayVocalCategory}
};

void SndInterfaceRegisterVocalScriptFunctions() {
  s_macroRaceDescs.SetCount(g_chrRacesDB.GetMaxID() + 1);
  for (int i = 0; i < g_soundCharacterMacroLinesDB.GetNumRecords(); ++i) {
    const SoundCharacterMacroLinesRec *record = g_soundCharacterMacroLinesDB.GetRecordByIndex(i);
    if (record && record->m_Category < 12 && record->m_Race < static_cast<int>(s_macroRaceDescs.Count()) && record->m_Sex < 3) {
      s_macroRaceDescs[record->m_Race].soundID[record->m_Category][record->m_Sex] = record->m_SoundID;
    }
  }
  FrameScript_RegisterFunction(s_ScriptFunctions[0].name, s_ScriptFunctions[0].method);
}

void SndInterfaceUnregisterVocalScriptFunctions() {
  s_macroRaceDescs.Clear();
  FrameScript_UnregisterFunction(s_ScriptFunctions[0].name);
}

void SoundInterfacePlayVocalMacro(const CGPlayer_C *player, int category) {
  if (player && (player->GetType() & TYPE_PLAYER) && category < 12) {
    const CGUnitData *unitData = player->GetUnitData();
    UINT              race = unitData->race;
    UINT              sex = unitData->sex;
    UINT              soundID = s_macroRaceDescs[race].soundID[category][sex];
    SndInterfacePlaySound(soundID, player->GetPosition(), -1, 1.0f);
  }
}
