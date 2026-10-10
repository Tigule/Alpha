#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Game/GameClient/PlayerName.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/WorldFrame.h"

#include <Base/Handle.h>
#include <FrameScript/FrameScript.h>
#include <Gxu/IGxuFont.h>
#include <Model/IModel.h>
#include <Os/OsTime.h>
#include <Tempest/cmath.h>
#include <storm.h>

class CGUnit_C;

enum UNITNAME_SHOWTYPE_GROUPS {
  SHOWTYPE_LOCALPLAYER,
  SHOWTYPE_ALLOTHERUNITS,
  NUM_UNITNAME_SHOWTYPE_CATEGORIES
};

enum UNIT_UNITNAME_SHOWTYPE {
  UNITNAMEINFO_NAME,
  UNITNAMEINFO_GUILD,
  UNITNAMEINFO_TITLE,
  UNITNAMEINFO_SUMMONEDBY,
  NUM_UNITNAME_SHOWTYPES
};

class PLAYERNAMEDESC : public CHandleObject {
 public:
  PLAYERNAMEDESC();
  virtual ~PLAYERNAMEDESC();

  LINKDECLEX(PLAYERNAMEDESC, m_link);
  CGxString          *m_string;
  UINT                m_customGeosetID;
  NTempest::CImVector m_stringColor;
  UINT                m_lastUpdateTime;
  NTempest::C3Vector  m_basePos;
  CGUnit_C           *m_unitPtr;
  UINT                m_flags;
  UINT                m_lastRenderFrame;
  HWORLDTEXT          m_worldTextHandles[4];
  float               m_heightOffset;

  void                UpdateWorldPos();
  void                Render(const NTempest::C44Matrix &basis);
  void                SetStringColor(const NTempest::CImVector &color);
  NTempest::CImVector GetStringColor() const;
  void                CreateWorldText(WORLDTEXTTYPE type, LPCSTR text, const NTempest::CImVector *colorOverride);
  void                UpdateWorldText();
  void                ShowWorldText(int show);
  void                RenderWorldText();
  void                MoveGeoset(const NTempest::C3Vector &pos);
};

struct CVARINFO {
  UNITNAME_SHOWTYPE_GROUPS group;
  UNIT_UNITNAME_SHOWTYPE   showType;
};

struct UNITNAMESTRINGS {
  LPCSTR cvarName;
  LPCSTR cvarDefaultValue;
  LPCSTR cvarHelp;
};

static CGxFont *s_playerNameFont;
static LISTDECLEX(PLAYERNAMEDESC, m_link, s_playerNames);
static int   s_showNames = 1;
static CVar *s_unitShowMode;
static CVar *s_showTypeCVars[8];
static UINT  s_showTypeFlags[2] = {-1, -1};
static UINT  s_lastRenderFrame;

#include "Object/ObjectClient/Unit_C.h"

PLAYERNAMEDESC::PLAYERNAMEDESC()
    : m_string(0),
      m_customGeosetID(-1),
      m_stringColor(0ul),
      m_basePos(0.0f),
      m_flags(3),
      m_lastRenderFrame(s_lastRenderFrame),
      m_heightOffset(0.0f) {
  m_lastUpdateTime = OsGetAsyncTimeMs();
  memset(m_worldTextHandles, 0, sizeof(m_worldTextHandles));
}

PLAYERNAMEDESC::~PLAYERNAMEDESC() {
  if (m_string) {
    GxuFontDestroyString(m_string);
  }
  if (m_unitPtr) {
    HMODEL model = m_unitPtr->GetCharacterModel(0);
    if (model) {
      if (m_customGeosetID != -1) {
        ModelCustGeosetRemove(model, m_customGeosetID);
      }
      HandleClose(model);
    }
  }
  UINT i;
  for (i = 4; i--;) {
    if (m_worldTextHandles[i]) {
      HandleClose(m_worldTextHandles[i]);
    }
  }
}

static void PlayerNameRenderCallback(HMODEL model, const NTempest::C34Matrix &basis, LPVOID param) {
  FATALASSERT(param);
  ((PLAYERNAMEDESC *)param)->Render(basis);
}

static const CVARINFO s_cvarInfo[8] = {
    {  SHOWTYPE_LOCALPLAYER,       UNITNAMEINFO_NAME},
    {  SHOWTYPE_LOCALPLAYER,      UNITNAMEINFO_GUILD},
    {  SHOWTYPE_LOCALPLAYER,      UNITNAMEINFO_TITLE},
    {  SHOWTYPE_LOCALPLAYER, UNITNAMEINFO_SUMMONEDBY},
    {SHOWTYPE_ALLOTHERUNITS,       UNITNAMEINFO_NAME},
    {SHOWTYPE_ALLOTHERUNITS,      UNITNAMEINFO_GUILD},
    {SHOWTYPE_ALLOTHERUNITS,      UNITNAMEINFO_TITLE},
    {SHOWTYPE_ALLOTHERUNITS, UNITNAMEINFO_SUMMONEDBY}
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

static bool   UnitNameShowTypeCallback(CVar *h, LPCSTR oldValue, LPCSTR newValue, LPVOID arg);
static void   TriggerNameRegenerate();
void          PlayerNameShutdown();
HWORLDTEXT WorldTextCreate(WORLDTEXTTYPE type, LPCSTR text, DWORDLONG object, const NTempest::CImVector *colorOverride);

static void CalculateBillboardRotation(const NTempest::C3Vector &direction, NTempest::C44Matrix &matrix) {
  NTempest::C3Vector zprime(direction);
  zprime.Normalize();
  matrix.c0 = zprime.x;
  matrix.c1 = zprime.y;
  matrix.c2 = zprime.z;

  NTempest::C3Vector xprime(matrix.c1, -matrix.c0, 0.0f);
  xprime.Normalize();
  matrix.a0 = xprime.x;
  matrix.a1 = xprime.y;
  matrix.a2 = xprime.z;

  matrix.b0 = matrix.a1 * matrix.c2;
  matrix.b1 = -matrix.a0 * matrix.c2;
  matrix.b2 = matrix.a0 * matrix.c1 - matrix.a1 * matrix.c0;
}

void PLAYERNAMEDESC::Render(const NTempest::C44Matrix &b) {
  FATALASSERT(m_unitPtr);
  ShowWorldText(1);
  m_lastRenderFrame = s_lastRenderFrame;

  UINT mode = s_unitShowMode->GetInt();
  if (s_showNames && mode < 4) {
    BOOL nameVisible = m_unitPtr->ShouldRenderUnitName(mode) != 0;
    if (nameVisible != ((m_flags >> 3) & 1)) {
      m_unitPtr->PlayerNameVisibilityChanged(nameVisible);
    }

    if (nameVisible) {
      m_flags |= 8;
      if (m_flags & 2) {
        m_flags &= ~2;
        m_unitPtr->GetSelectionHighlightColor(&m_stringColor);
        if (m_string && !(m_flags & 1)) {
          GxuFontSetStringColor(m_string, m_stringColor);
        }
      }

      if (m_flags & 1) {
        if (m_string) {
          GxuFontDestroyString(m_string);
        }
        m_heightOffset = 0.0f;
        m_string = 0;

        char buffer[260];
        buffer[0] = 0;
        m_heightOffset = m_unitPtr->UpdateUnitNameString(s_showTypeFlags[0], s_showTypeFlags[1], buffer, sizeof(buffer)) * 0.2f;
        if (s_playerNameFont && buffer[0]) {
          GxuFontCreateString(
              s_playerNameFont, buffer, 0.2f, NTempest::C3Vector(0.0f, 0.0f, 0.0f), 100000.0f, 100000.0f, 0.0f, m_string, GxVJ_Bottom, GxHJ_Center, 200,
              m_stringColor, 0.0f
          );
        }
        m_flags &= ~1;
      }

      if (m_string) {
        NTempest::C44Matrix worldTranslate(
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, b.d0, b.d1, b.d2 + m_heightOffset, 1.0f
        );
        NTempest::C3Vector facing(0.0f, 0.0f, 0.0f);
        CGWorldFrame::GetCameraFacing(&facing);
        NTempest::C44Matrix rotation;
        CalculateBillboardRotation(facing, rotation);
        rotation *= worldTranslate;
        GxuFontRender(m_string, rotation);
      }
    } else {
      m_flags &= ~8;
    }
  }

  m_basePos = NTempest::C3Vector(b.d0, b.d1, b.d2);
  UpdateWorldPos();
}

void PLAYERNAMEDESC::SetStringColor(const NTempest::CImVector &color) {
  m_stringColor = color;
  if (m_string) {
    GxuFontSetStringColor(m_string, color);
  }
}

void PLAYERNAMEDESC::RenderWorldText() {
  for (UINT i = 4; i--;) {
    if (m_worldTextHandles[i]) {
      WorldTextRender(m_worldTextHandles[i]);
    }
  }
}

void PLAYERNAMEDESC::ShowWorldText(int show) {
  if (show) {
    if (!(m_flags & 4)) {
      return;
    }
  } else if (m_flags & 4) {
    return;
  }

  for (UINT i = 4; i--;) {
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

void PLAYERNAMEDESC::CreateWorldText(WORLDTEXTTYPE type, LPCSTR text, const NTempest::CImVector *colorOverride) {
  UINT i;
  for (i = 0; i < 4; ++i) {
    if (!m_worldTextHandles[i]) {
      break;
    }
  }
  if (i < 4) {
    m_worldTextHandles[i] = WorldTextCreate(type, text, 0, colorOverride);
  }
}

void PLAYERNAMEDESC::UpdateWorldPos() {
  NTempest::C44Matrix cameraMatrix = CGWorldFrame::GetActive()->GetCurrentWorldMatrix();
  UINT                currentTime = OsGetAsyncTimeMs();
  int                 elapsed = currentTime - m_lastUpdateTime;
  ASSERT(elapsed >= 0);
  float elapsedSeconds = elapsed * 0.001f;

  for (UINT i = 4; i--;) {
    if (m_worldTextHandles[i]) {
      WorldTextUpdate(m_worldTextHandles[i], elapsedSeconds, cameraMatrix, &m_basePos);
      if (WorldTextIsTextDone(m_worldTextHandles[i])) {
        HandleClose(m_worldTextHandles[i]);
        m_worldTextHandles[i] = 0;
      }
    }
  }
  m_lastUpdateTime = currentTime;
}

void PLAYERNAMEDESC::UpdateWorldText() {
  UpdateWorldPos();
}

static void TriggerNameRegenerate() {
  ITERATELIST(PLAYERNAMEDESC, s_playerNames, desc) {
    desc->m_flags |= 1;
  }
}

static bool UnitNameShowTypeCallback(CVar *h, LPCSTR oldValue, LPCSTR newValue, LPVOID arg) {
  UINT index = (UINT)arg;
  ASSERT(index < (sizeof(s_cvarInfo) / sizeof(s_cvarInfo[0])));

  UINT                     showTypeFlag = 1 << s_cvarInfo[index].showType;
  UNITNAME_SHOWTYPE_GROUPS group = s_cvarInfo[index].group;

  UINT &flags = s_showTypeFlags[group];
  UINT  oldFlags = flags;

  if (SStrToInt(newValue)) {
    flags |= showTypeFlag;
  } else {
    flags &= ~showTypeFlag;
  }

  if (oldFlags != flags) {
    TriggerNameRegenerate();
  }

  return true;
}

void PlayerNameInitialize() {
  PlayerNameShutdown();

  LPCSTR fontName;
  if (FrameScript_GetVariable("UNIT_NAME_FONT", fontName)) {
    GxuFontCreateFont(fontName, 0.99f, s_playerNameFont, 4);
  }

  s_unitShowMode = CVar::Register(
      "UnitNameRenderMode", "sets unitname mode (0=none,1=lockedunits/lockedplayers, 2=playersalways+lockedunits, 3=all units always", 0, "2", 0,
      GRAPHICS, false, 0
  );

  for (UINT i = 0; i < sizeof(s_cvarStrings) / sizeof(s_cvarStrings[0]); ++i) {
    if (s_cvarStrings[i].cvarName) {
      s_showTypeCVars[i] = CVar::Register(
          s_cvarStrings[i].cvarName, s_cvarStrings[i].cvarHelp, 0, s_cvarStrings[i].cvarDefaultValue, UnitNameShowTypeCallback, GRAPHICS, false,
          (LPVOID)i
      );
    }
  }
}

void PlayerNameShutdown() {
  if (s_playerNameFont) {
    GxuFontDestroyFont(s_playerNameFont);
  }

  s_playerNameFont = 0;
  ConsoleCommandUnregister("PlayerNames");
}

void PlayerNameShow(int show) {
  s_showNames = show;
}

HPLAYERNAME PlayerNameCreate(CGUnit_C *unitPtr) {
  FATALASSERT(unitPtr);
  LPVOID          storage = SMemAlloc(sizeof(PLAYERNAMEDESC), "HPLAYERNAME", SERR_LINECODE_OBJECT, 0);
  PLAYERNAMEDESC *desc = storage ? new (storage) PLAYERNAMEDESC : 0;
  FATALASSERT(desc);

  desc->m_stringColor = NTempest::CImVector(0xFFE3C436);
  desc->m_unitPtr = unitPtr;
  s_playerNames.LinkNode(desc, LIST_TAIL, 0);

  HMODEL model = unitPtr->GetCharacterModel(0);
  if (!model) {
    return 0;
  }

  NTempest::C3Vector namePosition(0.0f);
  ModelGetModelSpacePivot(model, 22 | unitPtr->IsMounted(), &namePosition);
  ModelCustGeosetAdd(model, namePosition, PlayerNameRenderCallback, desc, &desc->m_customGeosetID);
  HandleClose(model);
  return CREATEHANDLE(HPLAYERNAME, desc);
}

void PlayerNameTriggerColorUpdate(HPLAYERNAME name) {
  if (name) {
    ((PLAYERNAMEDESC *)name)->m_flags |= 2;
  }
}

void PlayerNameCreateText(HPLAYERNAME name, WORLDTEXTTYPE type, LPCSTR text, const NTempest::CImVector *colorOverride) {
  if (ClntObjMgrGetPlayerType()) {
    return;
  }

  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  if (name) {
    ((PLAYERNAMEDESC *)name)->CreateWorldText(type, text, colorOverride);
  }
}

void PlayerNameUpdateWorldText(HPLAYERNAME name) {
  if (name) {
    ((PLAYERNAMEDESC *)name)->UpdateWorldText();
  }
}

void PlayerNameUpdateEarly() {
  ++s_lastRenderFrame;
}

void PlayerNameUpdateLate() {
  SAFEITERATELIST(PLAYERNAMEDESC, s_playerNames, node) {
    if (node->m_lastRenderFrame != s_lastRenderFrame) {
      node->ShowWorldText(0);
    }
  }
}

void PlayerNameTriggerNameRegenerate(HPLAYERNAME name) {
  if (name) {
    ((PLAYERNAMEDESC *)name)->m_flags |= 1;
  }
}

void PlayerNameChangeLocation(HPLAYERNAME name, const NTempest::C3Vector &namePosition) {
  if (name) {
    ((PLAYERNAMEDESC *)name)->MoveGeoset(namePosition);
  }
}

void PLAYERNAMEDESC::MoveGeoset(const NTempest::C3Vector &pos) {
  if (m_unitPtr && m_customGeosetID != -1) {
    HMODEL charModel = m_unitPtr->GetCharacterModel(0);
    FATALASSERT(charModel);
    ModelCustGeosetMove(charModel, m_customGeosetID, pos);
    HandleClose(charModel);
  }
}

UINT PlayerNameGetUnitNameMode() {
  FATALASSERT(s_unitShowMode);
  return s_unitShowMode->GetInt();
}

void PlayerNameRenderWorldText() {
  ITERATELIST(PLAYERNAMEDESC, s_playerNames, desc) {
    desc->RenderWorldText();
  }
}
