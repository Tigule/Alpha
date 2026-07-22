#include "UIBindings.h"

#include "Console/ConsoleCommand.h"

#include <Base/Status.h>
#include <FrameScript/FrameScript.h>
#include <FrameXML/LoadXML.h>
#include <FrameXML/XMLTree.h>
#include <Os/W32/Debugging.h>
#include <storm.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>

class CGUIBindingsStatus : public CStatus {
 public:
  CGUIBindingsStatus() {
  }
  virtual ~CGUIBindingsStatus() {
  }
  virtual void Add(STATUS_TYPE severity, const char *format, ...);
};

static CGUIBindingsStatus s_nullStatus;

int __fastcall ConsoleCommand_RunExec(const char *cmd, const char *arguments);

void CGUIBindingsStatus::Add(STATUS_TYPE, const char *format, ...) {
  char    buffer[512];
  va_list arguments;

  va_start(arguments, format);
  SStrVPrintf(buffer, sizeof(buffer), format, reinterpret_cast<char *>(arguments));
  va_end(arguments);
  OsOutputDebugString(buffer);
}

static int __fastcall Bind_CommandHandler(const char *command, const char *arguments) {
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

CGUIBindings *__fastcall CGUIBindings::Initialize(const char *commandsFile, CStatus *status) {
  FATALASSERT(!s_bindings);
  s_bindings = NEW(CGUIBindings);
  FATALASSERT(s_bindings);

  if (commandsFile) {
    s_bindings->Load(commandsFile, status);
  }

  ConsoleCommandRegister("bind", Bind_CommandHandler, DEFAULT, 0);
  return s_bindings;
}

void __fastcall CGUIBindings::Shutdown() {
  ConsoleCommandUnregister("bind");
  if (s_bindings) {
    DEL(s_bindings);
    s_bindings = 0;
  }
}

void __fastcall CGUIBindings::LoadBindings(int useDefault) {
  CGUIBindings *bindings = GetActive();
  FATALASSERT(bindings);

  bindings->m_bindings.Clear();
  if (useDefault) {
    ConsoleCommand_RunExec("run", "DefaultBindings.wtf");
  } else if (!ConsoleCommand_RunExec("run", "bindings.wtf")) {
    ConsoleCommand_RunExec("run", "DefaultBindings.wtf");
  }
}

void __fastcall CGUIBindings::SaveBindings() {
  CGUIBindings *bindings = GetActive();
  FATALASSERT(bindings);

  FILE *file = fopen("bindings.wtf", "wt");
  if (!file) {
    return;
  }
  for (KEYBINDING *binding = bindings->m_bindings.Head(); binding; binding = bindings->m_bindings.Next(binding)) {
    fprintf(file, "bind %s %s\n", binding->GetString(), binding->command);
  }
  fclose(file);
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
    if (SStrCmp(node->GetName(), "Binding", 0x7FFFFFFF)) {
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
  if (!keystring || !*keystring || !command) {
    return 0;
  }
  if (*command && !m_commands.Ptr(command)) {
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
  } else {
    if (!*command) {
      return 1;
    }
    binding = m_bindings.New(keystring, 0, 0);
  }

  binding->command = SStrDupA(command, __FILE__, __LINE__);
  binding->index = GetNumCommandKeys(command);
  return 1;
}

int CGUIBindings::ExecKey(const char *keystring, unsigned long timestamp, int down) {
  const char *command = GetCommandAction(keystring);
  return command ? ExecCommand(command, timestamp, down) : 0;
}

int CGUIBindings::ExecCommand(const char *command, unsigned long timestamp, int down) {
  KEYCOMMAND *keyCommand = m_commands.Ptr(command);
  if (!keyCommand || !keyCommand->function || (keyCommand->runOnUp ? down : !down)) {
    return 0;
  }
  FrameScript_Execute(keyCommand->function);
  return 1;
}

void CGUIBindings::GetCommand(int index, const char *&command) {
  command = 0;
  for (KEYCOMMAND *entry = m_commands.Head(); entry; entry = m_commands.Next(entry)) {
    if (entry->index == index) {
      command = entry->GetString();
      return;
    }
  }
}

void CGUIBindings::GetHiddenCommand(int index, const char *&command) {
  command = 0;
  for (KEYCOMMAND *entry = m_commands.Head(); entry; entry = m_commands.Next(entry)) {
    if (entry->index == -index - 1) {
      command = entry->GetString();
      return;
    }
  }
}

const char *CGUIBindings::GetCommandKey(const char *command, int keyindex) {
  for (KEYBINDING *binding = m_bindings.Head(); binding; binding = m_bindings.Next(binding)) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index == keyindex) {
      return binding->GetString();
    }
  }
  return 0;
}

unsigned int CGUIBindings::GetNumCommandKeys(const char *command) {
  unsigned int count = 0;
  for (KEYBINDING *binding = m_bindings.Head(); binding; binding = m_bindings.Next(binding)) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF)) {
      ++count;
    }
  }
  return count;
}

void CGUIBindings::AdjustCommandKeyIndices(const char *command, int index) {
  for (KEYBINDING *binding = m_bindings.Head(); binding; binding = m_bindings.Next(binding)) {
    if (!SStrCmpI(binding->command, command, 0x7FFFFFFF) && binding->index > index) {
      --binding->index;
    }
  }
}

const char *CGUIBindings::GetCommandAction(const char *keystring) {
  KEYBINDING *binding = m_bindings.Ptr(keystring);
  return binding ? binding->command : 0;
}

static int __fastcall Script_GetNumBindings(lua_State *L) {
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  lua_pushnumber(L, bindings->GetNumCommands());
  return 1;
}

static int __fastcall Script_GetBinding(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetBinding(index)");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  const char *command;
  bindings->GetCommand(static_cast<int>(lua_tonumber(L, 1)) - 1, command);
  if (!command) {
    return 0;
  }
  lua_pushstring(L, command);
  const char *key = bindings->GetCommandKey(command, 0);
  key ? lua_pushstring(L, key) : lua_pushnil(L);
  key = bindings->GetCommandKey(command, 1);
  key ? lua_pushstring(L, key) : lua_pushnil(L);
  return 3;
}

static int __fastcall Script_SetBinding(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SetBinding(\"key\" [, \"command\"])");
  }
  const char   *command = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  lua_pushnumber(L, bindings->Bind(lua_tostring(L, 1), command));
  return 1;
}

static int __fastcall Script_GetBindingKey(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetBindingKey(\"command\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  const char  *command = lua_tostring(L, 1);
  unsigned int count = bindings->GetNumCommandKeys(command);
  for (unsigned int i = 0; i < count; ++i) {
    lua_pushstring(L, bindings->GetCommandKey(command, i));
  }
  return count;
}

static int __fastcall Script_GetBindingAction(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetBindingAction(\"key\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  const char *command = bindings->GetCommandAction(lua_tostring(L, 1));
  lua_pushstring(L, command ? command : "");
  return 1;
}

static int __fastcall Script_RunBinding(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: RunBinding(\"command\")");
  }
  CGUIBindings *bindings = CGUIBindings::GetActive();
  FATALASSERT(bindings);
  lua_pushnumber(L, bindings->ExecCommand(lua_tostring(L, 1), GetTickCount(), 1));
  return 1;
}

static int __fastcall Script_ResetBindings(lua_State *L) {
  CGUIBindings::LoadBindings(0);
  return 0;
}

static int __fastcall Script_DefaultBindings(lua_State *L) {
  CGUIBindings::LoadBindings(1);
  return 0;
}

static int __fastcall Script_SaveBindings(lua_State *L) {
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

void __fastcall UIBindingsRegisterScriptFunctions() {
  for (int i = 0; i < 9; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall UIBindingsUnegisterScriptFunctions() {
  for (int i = 0; i < 9; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
