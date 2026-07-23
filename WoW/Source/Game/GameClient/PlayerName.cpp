#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Game/GameClient/PlayerName.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <Base/Handle.h>
#include <FrameScript/FrameScript.h>
#include <Gxu/IGxuFont.h>
#include <storm.h>

class CGUnit_C;
struct HMODEL__;
struct HWORLDTEXT__;

enum UNITNAME_SHOWTYPE_GROUPS {
  UNITNAME_SHOWTYPE_GROUP_PLAYER,
  UNITNAME_SHOWTYPE_GROUP_UNIT
};

enum UNIT_UNITNAME_SHOWTYPE {
  UNIT_UNITNAME_SHOWTYPE_NAME,
  UNIT_UNITNAME_SHOWTYPE_GUILD,
  UNIT_UNITNAME_SHOWTYPE_TITLE,
  UNIT_UNITNAME_SHOWTYPE_SUMMONED_BY
};

struct PLAYERNAMEDESC : public CHandleObject {
  virtual ~PLAYERNAMEDESC();

  void CreateWorldText(WORLDTEXTTYPE type, const char *text, const NTempest::CImVector *colorOverride);
  void RenderWorldText();
  void ShowWorldText(int show);

  TSLink<PLAYERNAMEDESC> m_link;
  CGxString             *m_string;
  unsigned int           m_customGeosetID;
  NTempest::CImVector    m_stringColor;
  unsigned int           m_lastUpdateTime;
  NTempest::C3Vector     m_basePos;
  CGUnit_C              *m_unitPtr;
  unsigned int           m_flags;
  unsigned int           m_lastRenderFrame;
  HWORLDTEXT__          *m_worldTextHandles[4];
  float                  m_heightOffset;
};

struct CVARINFO {
  UNITNAME_SHOWTYPE_GROUPS group;
  UNIT_UNITNAME_SHOWTYPE   showType;
};

struct UNITNAMESTRINGS {
  const char *cvarName;
  const char *cvarDefaultValue;
  const char *cvarHelp;
};

static CGxFont                          *s_playerNameFont;
static TSExplicitList<PLAYERNAMEDESC, 8> s_playerNames;
static int                               s_showNames;
static CVar                             *s_unitShowMode;
static CVar                             *s_showTypeCVars[8];
static unsigned int                      s_showTypeFlags[2] = {-1, -1};
static unsigned int                      s_lastRenderFrame;

static void PlayerNameRenderCallback(HMODEL__* model, const NTempest::C34Matrix& basis, void* param) {
    // TODO: implement
}

static const CVARINFO s_cvarInfo[8] = {
    {UNITNAME_SHOWTYPE_GROUP_PLAYER,        UNIT_UNITNAME_SHOWTYPE_NAME},
    {UNITNAME_SHOWTYPE_GROUP_PLAYER,       UNIT_UNITNAME_SHOWTYPE_GUILD},
    {UNITNAME_SHOWTYPE_GROUP_PLAYER,       UNIT_UNITNAME_SHOWTYPE_TITLE},
    {UNITNAME_SHOWTYPE_GROUP_PLAYER, UNIT_UNITNAME_SHOWTYPE_SUMMONED_BY},
    {  UNITNAME_SHOWTYPE_GROUP_UNIT,        UNIT_UNITNAME_SHOWTYPE_NAME},
    {  UNITNAME_SHOWTYPE_GROUP_UNIT,       UNIT_UNITNAME_SHOWTYPE_GUILD},
    {  UNITNAME_SHOWTYPE_GROUP_UNIT,       UNIT_UNITNAME_SHOWTYPE_TITLE},
    {  UNITNAME_SHOWTYPE_GROUP_UNIT, UNIT_UNITNAME_SHOWTYPE_SUMMONED_BY}
};

static const UNITNAMESTRINGS s_cvarStrings[8] = {
    {    "UnitNamePlayerName", "1",     "Toggles showing your name in worldname"},
    {   "UnitNamePlayerGuild", "1",    "Toggles showing your guild in worldname"},
    {   "UnitNamePlayerTitle", "1",    "Toggles showing your title in worldname"},
    {                       0,   0,                                            0},
    {      "UnitNameUnitName", "1",  "Toggles showing units' names in worldname"},
    {     "UnitNameUnitGuild", "1", "Toggles showing units' guilds in worldname"},
    {     "UnitNameUnitTitle", "1", "Toggles showing units' titles in worldname"},
    {"UnitNameUnitSummonedBy", "1", "Toggles showing units' owners in worldname"}
};

static bool __fastcall   UnitNameShowTypeCallback(CVar *h, const char *oldValue, const char *newValue, void *arg);
static void __fastcall   TriggerNameRegenerate();
void __fastcall          PlayerNameShutdown();
HWORLDTEXT__ *__fastcall WorldTextCreate(WORLDTEXTTYPE type, const char *text, unsigned __int64 object, const NTempest::CImVector *colorOverride);

static void CalculateBillboardRotation(const NTempest::C3Vector& direction, NTempest::C44Matrix& matrix) {
    // TODO: implement
}

void PLAYERNAMEDESC::RenderWorldText() {
  for (unsigned int i = 0; i < 4; ++i) {
    if (m_worldTextHandles[i]) {
      WorldTextRender(m_worldTextHandles[i]);
    }
  }
}

void PLAYERNAMEDESC::ShowWorldText(int show) {
  if ((show && (m_flags & 4)) || (!show && !(m_flags & 4))) {
    for (unsigned int i = 0; i < 4; ++i) {
      if (m_worldTextHandles[i]) {
        WorldTextShow(m_worldTextHandles[i], show);
      }
    }

    if (show) {
      m_flags &= ~4;
    } else {
      m_flags |= 4;
    }
  }
}

void PLAYERNAMEDESC::CreateWorldText(WORLDTEXTTYPE type, const char *text, const NTempest::CImVector *colorOverride) {
  for (unsigned int i = 0; i < 4; ++i) {
    if (!m_worldTextHandles[i]) {
      m_worldTextHandles[i] = WorldTextCreate(type, text, 0, colorOverride);
      return;
    }
  }
}

static void __fastcall TriggerNameRegenerate() {
  for (PLAYERNAMEDESC *desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
    desc->m_flags |= 1;
  }
}

static bool __fastcall UnitNameShowTypeCallback(CVar *h, const char *oldValue, const char *newValue, void *arg) {
  unsigned int index = reinterpret_cast<unsigned int>(arg);
  ASSERT(index < sizeof(s_cvarInfo) / sizeof(s_cvarInfo[0]));

  UNITNAME_SHOWTYPE_GROUPS group = s_cvarInfo[index].group;
  unsigned int             oldFlags = s_showTypeFlags[group];
  unsigned int             showTypeFlag = 1 << s_cvarInfo[index].showType;

  if (SStrToInt(newValue)) {
    s_showTypeFlags[group] |= showTypeFlag;
  } else {
    s_showTypeFlags[group] &= ~showTypeFlag;
  }

  if (oldFlags != s_showTypeFlags[group]) {
    TriggerNameRegenerate();
  }

  return true;
}

void __fastcall PlayerNameInitialize() {
  PlayerNameShutdown();

  const char *fontName;
  if (FrameScript_GetVariable("UNIT_NAME_FONT", fontName)) {
    GxuFontCreateFont(fontName, 0.99f, s_playerNameFont, 4);
  }

  s_unitShowMode = CVar::Register(
      "UnitNameRenderMode", "sets unitname mode (0=none,1=lockedunits/lockedplayers, 2=playersalways+lockedunits, 3=all units always", 0, "2", 0,
      GRAPHICS, false, 0
  );

  for (unsigned int i = 0; i < sizeof(s_cvarStrings) / sizeof(s_cvarStrings[0]); ++i) {
    if (s_cvarStrings[i].cvarName) {
      s_showTypeCVars[i] = CVar::Register(
          s_cvarStrings[i].cvarName, s_cvarStrings[i].cvarHelp, 0, s_cvarStrings[i].cvarDefaultValue, UnitNameShowTypeCallback, GRAPHICS, false,
          reinterpret_cast<void *>(i)
      );
    }
  }
}

void __fastcall PlayerNameShutdown() {
  if (s_playerNameFont) {
    GxuFontDestroyFont(s_playerNameFont);
  }

  s_playerNameFont = 0;
  ConsoleCommandUnregister("PlayerNames");
}

void __fastcall PlayerNameShow(int show) {
  s_showNames = show;
}

HPLAYERNAME__* __fastcall PlayerNameCreate(CGUnit_C* unitPtr) {
    // TODO: implement
    return 0;
}

void __fastcall PlayerNameTriggerColorUpdate(HPLAYERNAME__ *name) {
  if (name) {
    reinterpret_cast<PLAYERNAMEDESC *>(name)->m_flags |= 2;
  }
}

void __fastcall PlayerNameCreateText(HPLAYERNAME__ *name, WORLDTEXTTYPE type, const char *text, const NTempest::CImVector *colorOverride) {
  if (ClntObjMgrGetPlayerType()) {
    return;
  }

  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  if (name) {
    reinterpret_cast<PLAYERNAMEDESC *>(name)->CreateWorldText(type, text, colorOverride);
  }
}

void __fastcall PlayerNameUpdateWorldText(HPLAYERNAME__* name) {
    // TODO: implement
}

void __fastcall PlayerNameUpdateEarly() {
  ++s_lastRenderFrame;
}

void __fastcall PlayerNameUpdateLate() {
  for (PLAYERNAMEDESC *desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
    if (desc->m_lastRenderFrame != s_lastRenderFrame) {
      desc->ShowWorldText(0);
    }
  }
}

void __fastcall PlayerNameTriggerNameRegenerate(HPLAYERNAME__ *name) {
  if (name) {
    reinterpret_cast<PLAYERNAMEDESC *>(name)->m_flags |= 1;
  }
}

void __fastcall PlayerNameChangeLocation(HPLAYERNAME__* name, const NTempest::C3Vector& namePosition) {
    // TODO: implement
}

unsigned int __fastcall PlayerNameGetUnitNameMode() {
  FATALASSERT(s_unitShowMode);
  return s_unitShowMode->GetInt();
}

void __fastcall PlayerNameRenderWorldText() {
  for (PLAYERNAMEDESC *desc = s_playerNames.Head(); desc; desc = s_playerNames.Next(desc)) {
    desc->RenderWorldText();
  }
}
