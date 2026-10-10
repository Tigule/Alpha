#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "UIBindings.h"
#include <Os/OsTime.h>

#include "Console/ConsoleCommand.h"

#include <Base/Status.h>
#include <FrameScript/FrameScript.h>
#include <FrameXML/LoadXML.h>
#include <FrameXML/XMLTree.h>
#include <Event/CMouseEvent.h>
#include <Os/W32/Debugging.h>
#include <Os/W32/OsFile.h>
#include <storm.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>

class CGUIBindingsStatus : public CStatus {
 public:
  virtual void Add(int severity, LPCSTR format, ...);
};

static CGUIBindingsStatus s_nullStatus;

BOOL ConsoleCommand_RunExec(LPCSTR cmd, LPCSTR arguments);

void CGUIBindingsStatus::Add(int, LPCSTR format, ...) {
  char    buffer[512];
  va_list arguments;

  va_start(arguments, format);
  SStrVPrintf(buffer, sizeof(buffer), format, arguments);
  va_end(arguments);
  OsOutputDebugString(buffer);
}

static BOOL Bind_CommandHandler(LPCSTR command, LPCSTR arguments) {
  char keyName[32];

  if (!*arguments) {
    return 0;
  }

  CGUIBindings *keybinding = CGUIBindings::GetActive();
  FATALASSERT(keybinding);
  SStrTokenize(&arguments, keyName, sizeof(keyName), " ,;\"\t\n\r\x1a", 0);
  keybinding->Bind(keyName, arguments);
  return 1;
}

CGUIBindings *CGUIBindings::s_bindings;

CGUIBindings *CGUIBindings::Initialize(LPCSTR commandsFile, CStatus *status) {
  FATALASSERT(!s_bindings);
  s_bindings = NEW(CGUIBindings);
  FATALASSERT(s_bindings);

  if (commandsFile) {
    s_bindings->Load(commandsFile, status);
  }

  ConsoleCommandRegister("bind", Bind_CommandHandler, DEFAULT, 0);
  return s_bindings;
}

void CGUIBindings::Shutdown() {
  ConsoleCommandUnregister("bind");
  if (s_bindings) {
    DEL(s_bindings);
    s_bindings = 0;
  }
}

void CGUIBindings::LoadBindings(int useDefault) {
  CGUIBindings *bindings = GetActive();
  FATALASSERT(bindings);

  bindings->m_bindings.Clear();
  if (useDefault || !ConsoleCommand_RunExec("run", "bindings.wtf")) {
    ConsoleCommand_RunExec("run", "DefaultBindings.wtf");
  }
}

void CGUIBindings::SaveBindings() {
  char fileName[MAX_PATH];
  SStrCopy(fileName, "bindings.wtf", sizeof(fileName));
  HOSFILE file = OsCreateFile(fileName, 0x40000000, 0, 2, 0x80, 0x3F3F3F3F);
  if (file != HOSFILE_INVALID) {
    CGUIBindings *bindings = GetActive();
    ASSERT(bindings);

    int i;
    for (i = 0; i < bindings->GetNumCommands(); ++i) {
      int    keyIndex = 0;
      LPCSTR command = "";
      bindings->GetCommand(i, command);
      for (LPCSTR key = bindings->GetCommandKey(command, 0); key; key = bindings->GetCommandKey(command, keyIndex)) {
        char buffer[128];
        ++keyIndex;
        SStrPrintf(buffer, sizeof(buffer), "bind %s %s\n", key, command);
        DWORD count = 0;
        OsWriteFile(file, buffer, SStrLen(buffer), &count);
      }
    }

    for (i = 0; i < bindings->GetNumHiddenCommands(); ++i) {
      int    keyIndex = 0;
      LPCSTR command = "";
      bindings->GetHiddenCommand(i, command);
      for (LPCSTR key = bindings->GetCommandKey(command, 0); key; key = bindings->GetCommandKey(command, keyIndex)) {
        char buffer[128];
        ++keyIndex;
        SStrPrintf(buffer, sizeof(buffer), "bind %s %s\n", key, command);
        DWORD count = 0;
        OsWriteFile(file, buffer, SStrLen(buffer), &count);
      }
    }

    OsCloseFile(file);
    if (!SFile::FileExists(fileName)) {
      SFile::RebuildHash();
    }
    FrameScript_SignalEvent(369);
  }
}

BOOL CGUIBindings::AddMetaPrefix(UINT metaKeyState, char *&string, int &maxLen) {
  static struct {
    UINT   metaKey;
    LPCSTR metaStr;
  } metaList[3] = {
      {  KEY_SHIFT, "SHIFT-"},
      {KEY_CONTROL,  "CTRL-"},
      {    KEY_ALT,   "ALT-"}
  };

  for (UINT index = 3; index--;) {
    if (metaKeyState & (1 << metaList[index].metaKey)) {
      int length = SStrLen(metaList[index].metaStr);
      if (length >= maxLen) {
        return 0;
      }
      SStrCopy(string, metaList[index].metaStr, 0x7FFFFFFF);
      string += length;
      maxLen -= length;
    }
  }
  return 1;
}

LPCSTR CGUIBindings::KeyEventToString(const CKeyEvent &evt, char *string, int maxLen) {
  static char charBuf[8];
  LPCSTR      result = string;
  LPCSTR      keyString;
  UINT        metaKeyState = evt.metaKeyState;
  KEY         key = evt.key;

  if (evt.repeat > 1) {
    return 0;
  }
  if (key <= KEY_LASTMETAKEY) {
    metaKeyState &= ~(1 << key);
  }
  if (!AddMetaPrefix(metaKeyState, string, maxLen)) {
    return 0;
  }

  switch (key) {
    case KEY_SHIFT:
      keyString = "SHIFT";
      break;
    case KEY_CONTROL:
      keyString = "CTRL";
      break;
    case KEY_ALT:
      keyString = "ALT";
      break;
    case KEY_SPACE:
      keyString = "SPACE";
      break;
    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
    case KEY_A:
    case KEY_B:
    case KEY_C:
    case KEY_D:
    case KEY_E:
    case KEY_F:
    case KEY_G:
    case KEY_H:
    case KEY_I:
    case KEY_J:
    case KEY_K:
    case KEY_L:
    case KEY_M:
    case KEY_N:
    case KEY_O:
    case KEY_P:
    case KEY_Q:
    case KEY_R:
    case KEY_S:
    case KEY_T:
    case KEY_U:
    case KEY_V:
    case KEY_W:
    case KEY_X:
    case KEY_Y:
    case KEY_Z:
      SStrPrintf(charBuf, sizeof(charBuf), "%c", key);
      keyString = charBuf;
      break;
    case KEY_TILDE:
      keyString = "TILDE";
      break;
    case KEY_NUMPAD0:
    case KEY_NUMPAD1:
    case KEY_NUMPAD2:
    case KEY_NUMPAD3:
    case KEY_NUMPAD4:
    case KEY_NUMPAD5:
    case KEY_NUMPAD6:
    case KEY_NUMPAD7:
    case KEY_NUMPAD8:
    case KEY_NUMPAD9:
      SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", key - KEY_NUMPAD0);
      keyString = charBuf;
      break;
    case KEY_NUMPAD_PLUS:
      keyString = "NUMPADPLUS";
      break;
    case KEY_NUMPAD_MINUS:
      keyString = "NUMPADMINUS";
      break;
    case KEY_NUMPAD_MULTIPLY:
      keyString = "NUMPADMULTIPLY";
      break;
    case KEY_NUMPAD_DIVIDE:
      keyString = "NUMPADDIVIDE";
      break;
    case KEY_PLUS:
      keyString = "PLUS";
      break;
    case KEY_MINUS:
      keyString = "MINUS";
      break;
    case KEY_BRACKET_OPEN:
      keyString = "LEFTBRACKET";
      break;
    case KEY_BRACKET_CLOSE:
      keyString = "RIGHTBRACKET";
      break;
    case KEY_SLASH:
      keyString = "SLASH";
      break;
    case KEY_BACKSLASH:
      keyString = "BACKSLASH";
      break;
    case KEY_SEMICOLON:
      keyString = "SEMICOLON";
      break;
    case KEY_APOSTROPHE:
      keyString = "APOSTROPHE";
      break;
    case KEY_COMMA:
      keyString = "COMMA";
      break;
    case KEY_PERIOD:
      keyString = "PERIOD";
      break;
    case KEY_ESCAPE:
      keyString = "ESCAPE";
      break;
    case KEY_ENTER:
      keyString = "ENTER";
      break;
    case KEY_BACKSPACE:
      keyString = "BACKSPACE";
      break;
    case KEY_TAB:
      keyString = "TAB";
      break;
    case KEY_LEFT:
      keyString = "LEFT";
      break;
    case KEY_UP:
      keyString = "UP";
      break;
    case KEY_RIGHT:
      keyString = "RIGHT";
      break;
    case KEY_DOWN:
      keyString = "DOWN";
      break;
    case KEY_INSERT:
      keyString = "INSERT";
      break;
    case KEY_DELETE:
      keyString = "DELETE";
      break;
    case KEY_HOME:
      keyString = "HOME";
      break;
    case KEY_END:
      keyString = "END";
      break;
    case KEY_PAGEUP:
      keyString = "PAGEUP";
      break;
    case KEY_PAGEDOWN:
      keyString = "PAGEDOWN";
      break;
    case KEY_NUMLOCK:
      keyString = "NUMLOCK";
      break;
    case KEY_CAPSLOCK:
      keyString = "CAPSLOCK";
      break;
    case KEY_SCROLLLOCK:
      keyString = "SCROLLLOCK";
      break;
    case KEY_PAUSE:
      keyString = "PAUSE";
      break;
    case KEY_PRINTSCREEN:
      keyString = "PRINTSCREEN";
      break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
      SStrPrintf(charBuf, sizeof(charBuf), "F%d", key - KEY_F1 + 1);
      keyString = charBuf;
      break;
    default:
      keyString = "UNKNOWN";
      break;
  }

  if (!*keyString || (int)SStrLen(keyString) >= maxLen) {
    return 0;
  }
  SStrCopy(string, keyString, 0x7FFFFFFF);
  return result;
}

LPCSTR CGUIBindings::MouseEventToString(const CMouseEvent &evt, char *string, int maxLen) {
  LPCSTR result = string;
  if (!AddMetaPrefix(evt.metaKeyState, string, maxLen)) {
    return 0;
  }

  switch (evt.Id()) {
    case EVENT_MOUSE_DOWN:
    case EVENT_MOUSE_UP:
      switch (evt.button) {
        case MOUSE_BUTTON_LEFT:
          SStrCopy(string, "BUTTON1", maxLen);
          break;
        case MOUSE_BUTTON_RIGHT:
          SStrCopy(string, "BUTTON2", maxLen);
          break;
        case MOUSE_BUTTON_MIDDLE:
          SStrCopy(string, "BUTTON3", maxLen);
          break;
        case MOUSE_BUTTON_XBUTTON1:
          SStrCopy(string, "BUTTON4", maxLen);
          break;
        case MOUSE_BUTTON_XBUTTON2:
          SStrCopy(string, "BUTTON5", maxLen);
          break;
        default: {
          ASSERT(evt.button > MOUSE_BUTTON_RIGHT);
          *string = 0;
          for (int i = 4; i < 32; ++i) {
            if (evt.button == 1 << (i - 1)) {
              SStrPrintf(string, maxLen, "BUTTON%d", i);
              break;
            }
          }
          ASSERT(*string);
          break;
        }
      }
      break;
    case EVENT_MOUSE_WHEEL:
      if (evt.wheelDistance < 0) {
        SStrCopy(string, "MOUSEWHEELDOWN", maxLen);
      } else {
        SStrCopy(string, "MOUSEWHEELUP", maxLen);
      }
      break;
    default:
      return 0;
  }
  return result;
}

CGUIBindings::CGUIBindings() {
}

CGUIBindings::~CGUIBindings() {
  m_bindings.Clear();
  m_commands.Clear();
}

BOOL CGUIBindings::Load(LPCSTR commandsFile, CStatus *status) {
  char           headerBuf[64];
  XMLTree       *tree;
  LPCSTR         script;
  DWORD          bytesRead;
  int            headerIndex;
  LPVOID         buffer;
  LPCSTR         name;
  const XMLNode *node;

  if (!status) {
    status = &s_nullStatus;
  }

  m_numCommands = 0;
  m_numHiddenCommands = 0;

  if (!SFile::LoadFile(commandsFile, &buffer, &bytesRead, 0, 0)) {
    status->Add(STATUS_ERROR, "Couldn't open %s", commandsFile);
    return 0;
  }

  tree = XMLTree_Load((LPCSTR)buffer, bytesRead);
  FREE(buffer);
  if (!tree) {
    status->Add(STATUS_ERROR, "Couldn't parse XML in %s", commandsFile);
    return 0;
  }

  for (node = XMLTree_GetRoot(tree)->GetChild(); node; node = node->GetSibling()) {
    if (SStrCmpI(node->GetName(), "Binding", 0x7FFFFFFF)) {
      status->Add(STATUS_WARNING, "Unknown node type %s in %s", node->GetName(), commandsFile);
      continue;
    }

    name = node->GetAttributeByName("name");
    script = node->GetBody();
    if (name && *name) {
      if (script && *script) {
        if (!m_commands.Ptr(name)) {
          LPCSTR header = node->GetAttributeByName("header");
          if (header && *header) {
            headerIndex = SStrToInt(header);
            if (headerIndex >= 0) {
              SStrPrintf(headerBuf, sizeof(headerBuf), "HEADER%d", headerIndex);
              if (!m_commands.Ptr(headerBuf)) {
                KEYCOMMAND *headerCommand = m_commands.New(headerBuf, 0, 0);
                headerCommand->index = m_numCommands++;
                headerCommand->headerIndex = headerIndex;
                headerCommand->function = 0;
              } else {
                status->Add(STATUS_WARNING, "Binding header %d is defined more than once in %s", headerIndex, commandsFile);
              }
            }
          }

          KEYCOMMAND *command = m_commands.New(name, 0, 0);
          LPCSTR      hidden = node->GetAttributeByName("hidden");
          if (hidden && StringToBOOL(hidden)) {
            command->index = -++m_numHiddenCommands;
          } else {
            command->index = m_numCommands++;
          }

          command->function = FrameScript_CompileFunction(script, name);
          LPCSTR runOnUp = node->GetAttributeByName("runOnUp");
          if (runOnUp) {
            command->runOnUp = StringToBOOL(runOnUp);
          } else {
            command->runOnUp = 0;
          }
        } else {
          status->Add(STATUS_WARNING, "Binding %s is defined more than once in %s", name, commandsFile);
        }
      } else {
        status->Add(STATUS_WARNING, "Found binding %s with no script in %s", name, commandsFile);
      }
    } else {
      status->Add(STATUS_WARNING, "Found binding with no name in %s", commandsFile);
    }
  }

  XMLTree_Free(tree);
  return 1;
}

BOOL CGUIBindings::Bind(LPCSTR keystring, LPCSTR command) {
  if (!keystring) {
    return 0;
  }

  KEYBINDING *binding = m_bindings.Ptr(keystring);
  if (!binding) {
    if (!command || !*command) {
      return 1;
    }
    binding = m_bindings.New(keystring, 0, 0);
  }

  if (command && *command) {
    const KEYCOMMAND *keyCommand = m_commands.Ptr(command);
    if (keyCommand && keyCommand->runOnUp && !SStrCmpI(keystring, "MOUSEWHEEL", SStrLen("MOUSEWHEEL"))) {
      return 0;
    }
    if (binding->command) {
      AdjustCommandKeyIndices(binding->command, binding->index);
    }
    FREEIFUSED(binding->command);
    UINT index = GetNumCommandKeys(command);
    binding->command = SStrDupA(command, __FILE__, __LINE__);
    binding->index = index;
  } else {
    AdjustCommandKeyIndices(binding->command, binding->index);
    m_bindings.Delete(binding);
  }
  return 1;
}

int CGUIBindings::ExecKey(LPCSTR keystring, DWORD timestamp, int down) const {
  if (!keystring || !*keystring) {
    return 0;
  }

  const KEYBINDING *binding = m_bindings.Ptr(keystring);
  if (!binding) {
    return 0;
  }

  char command[80];
  SStrCopy(command, binding->command, sizeof(command));

  return ExecCommand(command, timestamp, down);
}

BOOL CGUIBindings::ExecCommand(LPCSTR command, DWORD timestamp, int down) const {
  if (!command || !*command) {
    return 0;
  }
  const KEYCOMMAND *keyCommand = m_commands.Ptr(command);
  if (!keyCommand || (!down && !keyCommand->runOnUp)) {
    return 0;
  }

  lua_State *state = FrameScript_GetContext();
  lua_pushstring(state, down ? "down" : "up");
  lua_pushstring(state, "keystate");
  lua_insert(state, -2);
  lua_settable(state, LUA_GLOBALSINDEX);
  FrameScript_Execute(keyCommand->function, 0, "%u", timestamp);
  lua_pushnil(state);
  lua_pushstring(state, "keystate");
  lua_insert(state, -2);
  lua_settable(state, LUA_GLOBALSINDEX);
  return 1;
}

void CGUIBindings::GetCommand(int index, LPCSTR &command) const {
  FATALASSERT(index >= 0 && index < GetNumCommands());
  CONSTITERATELIST(KEYCOMMAND, m_commands, entry) {
    if (entry->index == index) {
      command = entry->GetString();
      return;
    }
  }
}

void CGUIBindings::GetHiddenCommand(int index, LPCSTR &command) const {
  FATALASSERT(index >= 0 && index < GetNumHiddenCommands());
  index = -1 - index;
  CONSTITERATELIST(KEYCOMMAND, m_commands, entry) {
    if (entry->index == index) {
      command = entry->GetString();
      return;
    }
  }
}

LPCSTR CGUIBindings::GetCommandKey(LPCSTR command, int keyindex) const {
  LPCSTR key = 0;
  CONSTITERATELIST(KEYBINDING, m_bindings, binding) {
    if (binding->command && !SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index == keyindex) {
      key = binding->GetString();
      break;
    }
  }
  return key;
}

UINT CGUIBindings::GetNumCommandKeys(LPCSTR command) const {
  UINT count = 0;
  CONSTITERATELIST(KEYBINDING, m_bindings, binding) {
    if (binding->command && !SStrCmpI(binding->command, command, 0x7FFFFFFF)) {
      ++count;
    }
  }
  return count;
}

void CGUIBindings::AdjustCommandKeyIndices(LPCSTR command, int index) const {
  for (KEYBINDING *binding = m_bindings.Head(); (int)binding > 0; binding = m_bindings.RawNext(binding)) {
    if (binding->command && !SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index > index) {
      --binding->index;
    }
  }
}

LPCSTR CGUIBindings::GetCommandAction(LPCSTR keystring) const {
  if (!keystring || !*keystring) {
    return 0;
  }

  const KEYBINDING *binding = m_bindings.Ptr(keystring);
  if (!binding) {
    return 0;
  }

  return binding->command;
}

static int Script_GetNumBindings(lua_State *L) {
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  lua_pushnumber(L, bindings->GetNumCommands());
  return 1;
}

static int Script_GetBinding(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGUIBindings *bindings = CGUIBindings::GetActive();
    ASSERT(bindings);
    int    index = lua_tonumber(L, 1);
    LPCSTR command = "";
    int    count = 0;
    bindings->GetCommand(index - 1, command);
    lua_pushstring(L, command);
    for (LPCSTR key = bindings->GetCommandKey(command, 0); key; key = bindings->GetCommandKey(command, count)) {
      ++count;
      lua_pushstring(L, key);
    }
    return count + 1;
  }
  luaL_error(L, "Usage: GetBinding(index)");
  return 0;
}

static int Script_SetBinding(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUIBindings *bindings = CGUIBindings::GetActive();
    ASSERT(bindings);
    if (bindings->Bind(lua_tostring(L, 1), lua_tostring(L, 2))) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  luaL_error(L, "Usage: SetBinding(\"KEY\"[, \"COMMAND\"])");
  return 0;
}

static int Script_GetBindingKey(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUIBindings *bindings = CGUIBindings::GetActive();
    FATALASSERT(bindings);
    int    count = 0;
    LPCSTR command = lua_tostring(L, 1);
    for (LPCSTR key = bindings->GetCommandKey(command, 0); key; key = bindings->GetCommandKey(command, count)) {
      ++count;
      lua_pushstring(L, key);
    }
    return count;
  }
  luaL_error(L, "Usage: GetBindingKey(\"COMMAND\")");
  return 0;
}

static int Script_GetBindingAction(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUIBindings *bindings = CGUIBindings::GetActive();
    FATALASSERT(bindings);
    LPCSTR command = bindings->GetCommandAction(lua_tostring(L, 1));
    lua_pushstring(L, command ? command : "");
    return 1;
  }
  luaL_error(L, "Usage: GetBindingAction(\"KEY\")");
  return 0;
}

static int Script_RunBinding(lua_State *L) {
  if (lua_isstring(L, 1)) {
    CGUIBindings *bindings = CGUIBindings::GetActive();
    ASSERT(bindings);
    int down = 1;
    if (lua_isstring(L, 2) && !SStrCmpI(lua_tostring(L, 2), "up", 0x7FFFFFFF)) {
      down = 0;
    }
    bindings->ExecCommand(lua_tostring(L, 1), OsGetAsyncTimeMs(), down);
    return 0;
  }
  luaL_error(L, "Usage: RunBinding(\"COMMAND\")");
  return 0;
}

static int Script_ResetBindings(lua_State *L) {
  CGUIBindings::LoadBindings(0);
  return 0;
}

static int Script_DefaultBindings(lua_State *L) {
  CGUIBindings::LoadBindings(1);
  return 0;
}

static int Script_SaveBindings(lua_State *L) {
  CGUIBindings::SaveBindings();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[9] = {
    {  "GetNumBindings",   Script_GetNumBindings},
    {      "GetBinding",       Script_GetBinding},
    {      "SetBinding",       Script_SetBinding},
    {   "GetBindingKey",    Script_GetBindingKey},
    {"GetBindingAction", Script_GetBindingAction},
    {      "RunBinding",       Script_RunBinding},
    {   "ResetBindings",    Script_ResetBindings},
    { "DefaultBindings",  Script_DefaultBindings},
    {    "SaveBindings",     Script_SaveBindings}
};

void UIBindingsRegisterScriptFunctions() {
  for (UINT i = 0; i < 9; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void UIBindingsUnegisterScriptFunctions() {
  for (UINT i = 0; i < 9; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
