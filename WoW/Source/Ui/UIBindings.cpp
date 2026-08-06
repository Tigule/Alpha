#include <WowConst.h>
#include <MapDefs.h>

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
  virtual void Add(int severity, const char *format, ...);
};

static CGUIBindingsStatus s_nullStatus;

int ConsoleCommand_RunExec(const char *cmd, const char *arguments);

void CGUIBindingsStatus::Add(int, const char *format, ...) {
  char    buffer[512];
  va_list arguments;

  va_start(arguments, format);
  SStrVPrintf(buffer, sizeof(buffer), format, reinterpret_cast<char *>(arguments));
  va_end(arguments);
  OsOutputDebugString(buffer);
}

static int Bind_CommandHandler(const char *command, const char *arguments) {
  char keyName[32];

  if (!*arguments) {
    return 0;
  }

  CGUIBindings *keybinding = CGUIBindings::GetActive();
  FATALASSERT(keybinding);
  SStrTokenize(&arguments, keyName, sizeof(keyName), " \t", 0);
  keybinding->Bind(keyName, arguments);
  return 1;
}

CGUIBindings *CGUIBindings::s_bindings;

CGUIBindings *CGUIBindings::Initialize(const char *commandsFile, CStatus *status) {
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
  if (useDefault) {
    ConsoleCommand_RunExec("run", "DefaultBindings.wtf");
  } else if (!ConsoleCommand_RunExec("run", "bindings.wtf")) {
    ConsoleCommand_RunExec("run", "DefaultBindings.wtf");
  }
}

void CGUIBindings::SaveBindings() {
  CGUIBindings *bindings = GetActive();
  FATALASSERT(bindings);

  const char *fileName = "bindings.wtf";
  HOSFILE file = OsCreateFile(fileName, 0x40000000, 0, 2, 0x80, 0x3F3F3F3F);
  if (file == HOSFILE_INVALID) {
    return;
  }

  char buffer[128];
  unsigned long bytesWritten;
  for (int commandIndex = 0; commandIndex < bindings->GetNumCommands(); ++commandIndex) {
    const char *command;
    bindings->GetCommand(commandIndex, command);
    for (int keyIndex = 0;; ++keyIndex) {
      const char *key = bindings->GetCommandKey(command, keyIndex);
      if (!key) {
        break;
      }
      SStrPrintf(buffer, sizeof(buffer), "bind %s %s\n", key, command);
      OsWriteFile(file, buffer, SStrLen(buffer), &bytesWritten);
    }
  }

  for (int hiddenCommandIndex = 0; hiddenCommandIndex < bindings->GetNumHiddenCommands(); ++hiddenCommandIndex) {
    const char *hiddenCommand;
    bindings->GetHiddenCommand(hiddenCommandIndex, hiddenCommand);
    for (int hiddenKeyIndex = 0;; ++hiddenKeyIndex) {
      const char *hiddenKey = bindings->GetCommandKey(hiddenCommand, hiddenKeyIndex);
      if (!hiddenKey) {
        break;
      }
      SStrPrintf(buffer, sizeof(buffer), "bind %s %s\n", hiddenKey, hiddenCommand);
      OsWriteFile(file, buffer, SStrLen(buffer), &bytesWritten);
    }
  }

  OsCloseFile(file);
  if (!SFile::FileExists(fileName)) {
    SFile::RebuildHash();
  }
  FrameScript_SignalEvent(369);
}

static struct {
  unsigned int metaKey;
  const char  *metaStr;
} metaList[3] = {
    {KEY_SHIFT, "SHIFT-"},
    {KEY_CONTROL, "CTRL-"},
    {KEY_ALT, "ALT-"}
};

int CGUIBindings::AddMetaPrefix(unsigned int metaKeyState, char *&string, int &maxLen) {
  for (int index = 2; index >= 0; --index) {
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

const char *CGUIBindings::KeyEventToString(const CKeyEvent &evt, char *string, int maxLen) {
  char        charBuf[8];
  char       *dest = string;
  const char *keyString = "UNKNOWN";
  unsigned int metaKeyState = evt.metaKeyState;

  if (evt.repeat > 1) {
    return 0;
  }
  if (evt.key <= KEY_LASTMETAKEY) {
    metaKeyState &= ~(1 << evt.key);
  }
  if (!AddMetaPrefix(metaKeyState, dest, maxLen)) {
    return 0;
  }

  if ((evt.key >= KEY_0 && evt.key <= KEY_9) || (evt.key >= KEY_A && evt.key <= KEY_Z)) {
    SStrPrintf(charBuf, sizeof(charBuf), "%c", evt.key);
    keyString = charBuf;
  } else if (evt.key >= KEY_NUMPAD0 && evt.key <= KEY_NUMPAD9) {
    SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", evt.key - KEY_NUMPAD0);
    keyString = charBuf;
  } else if (evt.key >= KEY_F1 && evt.key <= KEY_F12) {
    SStrPrintf(charBuf, sizeof(charBuf), "F%d", evt.key - KEY_F1 + 1);
    keyString = charBuf;
  } else {
    switch (evt.key) {
      case KEY_SHIFT: keyString = "SHIFT"; break;
      case KEY_CONTROL: keyString = "CTRL"; break;
      case KEY_ALT: keyString = "ALT"; break;
      case KEY_SPACE: keyString = "SPACE"; break;
      case KEY_TILDE: keyString = "TILDE"; break;
      case KEY_NUMPAD_PLUS: keyString = "NUMPADPLUS"; break;
      case KEY_NUMPAD_MINUS: keyString = "NUMPADMINUS"; break;
      case KEY_NUMPAD_MULTIPLY: keyString = "NUMPADMULTIPLY"; break;
      case KEY_NUMPAD_DIVIDE: keyString = "NUMPADDIVIDE"; break;
      case KEY_PLUS: keyString = "PLUS"; break;
      case KEY_MINUS: keyString = "MINUS"; break;
      case KEY_BRACKET_OPEN: keyString = "LEFTBRACKET"; break;
      case KEY_BRACKET_CLOSE: keyString = "RIGHTBRACKET"; break;
      case KEY_SLASH: keyString = "SLASH"; break;
      case KEY_BACKSLASH: keyString = "BACKSLASH"; break;
      case KEY_SEMICOLON: keyString = "SEMICOLON"; break;
      case KEY_APOSTROPHE: keyString = "APOSTROPHE"; break;
      case KEY_COMMA: keyString = "COMMA"; break;
      case KEY_PERIOD: keyString = "PERIOD"; break;
      case KEY_ESCAPE: keyString = "ESCAPE"; break;
      case KEY_ENTER: keyString = "ENTER"; break;
      case KEY_BACKSPACE: keyString = "BACKSPACE"; break;
      case KEY_TAB: keyString = "TAB"; break;
      case KEY_LEFT: keyString = "LEFT"; break;
      case KEY_UP: keyString = "UP"; break;
      case KEY_RIGHT: keyString = "RIGHT"; break;
      case KEY_DOWN: keyString = "DOWN"; break;
      case KEY_INSERT: keyString = "INSERT"; break;
      case KEY_DELETE: keyString = "DELETE"; break;
      case KEY_HOME: keyString = "HOME"; break;
      case KEY_END: keyString = "END"; break;
      case KEY_PAGEUP: keyString = "PAGEUP"; break;
      case KEY_PAGEDOWN: keyString = "PAGEDOWN"; break;
      case KEY_NUMLOCK: keyString = "NUMLOCK"; break;
      case KEY_CAPSLOCK: keyString = "CAPSLOCK"; break;
      case KEY_SCROLLLOCK: keyString = "SCROLLLOCK"; break;
      case KEY_PAUSE: keyString = "PAUSE"; break;
      case KEY_PRINTSCREEN: keyString = "PRINTSCREEN"; break;
    }
  }

  if (!*keyString || SStrLen(keyString) >= static_cast<unsigned int>(maxLen)) {
    return 0;
  }
  SStrCopy(dest, keyString, 0x7FFFFFFF);
  return string;
}

const char *CGUIBindings::MouseEventToString(const CMouseEvent &evt, char *string, int maxLen) {
  char *dest = string;
  if (!AddMetaPrefix(evt.metaKeyState, dest, maxLen)) {
    return 0;
  }

  if (evt.Id() == 0x400500CD) {
    SStrCopy(dest, evt.wheelDistance >= 0 ? "MOUSEWHEELUP" : "MOUSEWHEELDOWN", maxLen);
    return string;
  }
  if (evt.Id() < 0x400500C8 || evt.Id() > 0x400500C9) {
    return 0;
  }

  switch (evt.button) {
    case MOUSE_BUTTON_LEFT: SStrCopy(dest, "BUTTON1", maxLen); break;
    case MOUSE_BUTTON_MIDDLE: SStrCopy(dest, "BUTTON3", maxLen); break;
    case MOUSE_BUTTON_RIGHT: SStrCopy(dest, "BUTTON2", maxLen); break;
    case MOUSE_BUTTON_XBUTTON1: SStrCopy(dest, "BUTTON4", maxLen); break;
    case MOUSE_BUTTON_XBUTTON2: SStrCopy(dest, "BUTTON5", maxLen); break;
    default: {
      ASSERT(evt.button > MOUSE_BUTTON_RIGHT);
      *dest = 0;
      int button = 4;
      while (button < 32 && evt.button != 1 << (button - 1)) {
        ++button;
      }
      if (button < 32) {
        SStrPrintf(dest, maxLen, "BUTTON%d", button);
      }
      ASSERT(*dest);
      break;
    }
  }
  return string;
}

CGUIBindings::CGUIBindings() {
}

CGUIBindings::~CGUIBindings() {
  m_bindings.Clear();
  m_commands.Clear();
}

int CGUIBindings::Load(const char *commandsFile, CStatus *status) {
  char           headerBuf[64];
  XMLTree       *tree;
  const char    *script;
  unsigned long  bytesRead;
  int            headerIndex;
  void          *buffer;
  const char    *name;
  const XMLNode *node;

  if (!status) {
    status = &s_nullStatus;
  }

  m_numCommands = 0;
  m_numHiddenCommands = 0;

  if (!SFileLoadFile(commandsFile, &buffer, &bytesRead, 0, 0)) {
    status->Add(STATUS_ERROR, "Couldn't open %s", commandsFile);
    return 0;
  }

  tree = XMLTree_Load(static_cast<const char *>(buffer), bytesRead);
  SFileUnloadFile(buffer);
  if (!tree) {
    status->Add(STATUS_ERROR, "Couldn't parse XML in %s", commandsFile);
    return 0;
  }

  node = XMLTree_GetRoot(tree)->GetChild();
  while (node) {
    if (SStrCmpI(node->GetName(), "Binding", 0x7FFFFFFF)) {
      status->Add(STATUS_WARNING, "Unknown node type %s in %s", node->GetName(), commandsFile);
      node = node->GetSibling();
      continue;
    }

    name = node->GetAttributeByName("name");
    script = node->GetBody();
    if (!name || !*name) {
      status->Add(STATUS_WARNING, "Found binding with no name in %s", commandsFile);
      node = node->GetSibling();
      continue;
    }

    if (!script || !*script) {
      status->Add(STATUS_WARNING, "Found binding %s with no script in %s", name, commandsFile);
      node = node->GetSibling();
      continue;
    }

    if (m_commands.Ptr(name)) {
      status->Add(STATUS_WARNING, "Binding %s is defined more than once in %s", name, commandsFile);
      node = node->GetSibling();
      continue;
    }

    const char *header = node->GetAttributeByName("header");
    if (header && *header) {
      headerIndex = SStrToInt(header);
      if (headerIndex >= 0) {
        SStrPrintf(headerBuf, sizeof(headerBuf), "HEADER%d", headerIndex);
        if (m_commands.Ptr(headerBuf)) {
          status->Add(STATUS_WARNING, "Binding header %d is defined more than once in %s", headerIndex, commandsFile);
        } else {
          KEYCOMMAND *headerCommand = m_commands.New(headerBuf, 0, 0);
          headerCommand->index = m_numCommands++;
          headerCommand->headerIndex = headerIndex;
          headerCommand->function = 0;
        }
      }
    }

    KEYCOMMAND *command = m_commands.New(name, 0, 0);
    const char *hidden = node->GetAttributeByName("hidden");
    if (hidden && StringToBOOL(hidden)) {
      command->index = -++m_numHiddenCommands;
    } else {
      command->index = m_numCommands++;
    }

    command->function = FrameScript_CompileFunction(script, name);
    const char *runOnUp = node->GetAttributeByName("runOnUp");
    command->runOnUp = runOnUp ? StringToBOOL(runOnUp) : 0;
    node = node->GetSibling();
  }

  XMLTree_Free(tree);
  return 1;
}

int CGUIBindings::Bind(const char *keystring, const char *command) {
  if (!keystring || !command) {
    return 0;
  }
  if (*command && !m_commands.Ptr(command)) {
    return 0;
  }
  const KEYCOMMAND *keyCommand = m_commands.Ptr(command);
  if (keyCommand && keyCommand->runOnUp && !SStrCmpI(keystring, "MOUSEWHEEL", SStrLen("MOUSEWHEEL"))) {
    return 0;
  }

  KEYBINDING *binding = m_bindings.Ptr(keystring);
  if (binding) {
    AdjustCommandKeyIndices(binding->command, binding->index);
    if (!*command) {
      m_bindings.Delete(binding);
      return 1;
    }
    FREEIFUSED(binding->command);
    binding->index = GetNumCommandKeys(command);
  } else {
    if (!*command) {
      return 1;
    }
    binding = m_bindings.New(keystring, 0, 0);
  }

  binding->command = SStrDupA(command, __FILE__, __LINE__);
  return 1;
}

int CGUIBindings::ExecKey(const char *keystring, unsigned long timestamp, int down) const {
  if (!keystring || !*keystring) {
    return 0;
  }
  const char *action = GetCommandAction(keystring);
  if (!action) {
    return 0;
  }
  char command[80];
  SStrCopy(command, action, sizeof(command));
  return ExecCommand(command, timestamp, down);
}

int CGUIBindings::ExecCommand(const char *command, unsigned long timestamp, int down) const {
  if (!command || !*command) {
    return 0;
  }
  const KEYCOMMAND *keyCommand = m_commands.Ptr(command);
  if (!keyCommand || !keyCommand->function || (!down && !keyCommand->runOnUp)) {
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

void CGUIBindings::GetCommand(int index, const char *&command) const {
  command = 0;
  for (const KEYCOMMAND *entry = m_commands.Head(); entry; entry = m_commands.Next(entry)) {
    if (entry->index == index) {
      command = entry->GetString();
      return;
    }
  }
}

void CGUIBindings::GetHiddenCommand(int index, const char *&command) const {
  command = 0;
  for (const KEYCOMMAND *entry = m_commands.Head(); entry; entry = m_commands.Next(entry)) {
    if (entry->index == -index - 1) {
      command = entry->GetString();
      return;
    }
  }
}

const char *CGUIBindings::GetCommandKey(const char *command, int keyindex) const {
  for (const KEYBINDING *binding = m_bindings.Head(); binding; binding = m_bindings.Next(binding)) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index == keyindex) {
      return binding->GetString();
    }
  }
  return 0;
}

unsigned int CGUIBindings::GetNumCommandKeys(const char *command) const {
  unsigned int count = 0;
  for (const KEYBINDING *binding = m_bindings.Head(); binding; binding = m_bindings.Next(binding)) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF)) {
      ++count;
    }
  }
  return count;
}

void CGUIBindings::AdjustCommandKeyIndices(const char *command, int index) const {
  ITERATELIST(KEYBINDING, m_bindings, binding) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index > index) {
      --binding->index;
    }
  }
}

const char *CGUIBindings::GetCommandAction(const char *keystring) const {
  const KEYBINDING *binding = m_bindings.Ptr(keystring);
  return binding ? binding->command : 0;
}

static int Script_GetNumBindings(lua_State *L) {
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  lua_pushnumber(L, bindings->GetNumCommands());
  return 1;
}

static int Script_GetBinding(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetBinding(index)");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  ASSERT(bindings);
  const char *command = "";
  bindings->GetCommand(static_cast<int>(lua_tonumber(L, 1)) - 1, command);
  lua_pushstring(L, command);
  int count = 0;
  for (const char *key = bindings->GetCommandKey(command, 0);
       key;
       key = bindings->GetCommandKey(command, count)) {
    ++count;
    lua_pushstring(L, key);
  }
  return count + 1;
}

static int Script_SetBinding(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SetBinding(\"KEY\"[, \"COMMAND\"])");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  ASSERT(bindings);
  if (bindings->Bind(lua_tostring(L, 1), lua_tostring(L, 2))) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetBindingKey(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetBindingKey(\"command\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  const char *command = lua_tostring(L, 1);
  int         count = 0;
  for (const char *key = bindings->GetCommandKey(command, 0); key; key = bindings->GetCommandKey(command, count)) {
    ++count;
    lua_pushstring(L, key);
  }
  return count;
}

static int Script_GetBindingAction(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetBindingAction(\"key\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  const char *command = bindings->GetCommandAction(lua_tostring(L, 1));
  lua_pushstring(L, command ? command : "");
  return 1;
}

static int Script_RunBinding(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: RunBinding(\"COMMAND\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  ASSERT(bindings);
  int down = 1;
  if (lua_isstring(L, 2)) {
    down = SStrCmpI(lua_tostring(L, 2), "up", 0x7FFFFFFF) != 0;
  }
  bindings->ExecCommand(lua_tostring(L, 1), OsGetAsyncTimeMs(), down);
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
  for (int i = 0; i < 9; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void UIBindingsUnegisterScriptFunctions() {
  for (int i = 0; i < 9; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
