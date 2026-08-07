#include "FrameScript.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdarg.h>
#include <stpl.h>

void __cdecl SOutputDebugString(LPCSTR format, ...);

NODEDECL(EVENTLISTENERNODE) {
  FrameScript_Object *object;
};

class FrameScript_EventObject {
 public:
  FrameScript_EventObject();
  FrameScript_EventObject(const FrameScript_EventObject &eventObject);
  ~FrameScript_EventObject();

  char *name;
  LISTDECL(EVENTLISTENERNODE, list);
};

static TSFixedArray<FrameScript_Object *>    s_objectStack;
static UINT                                  s_objectCount;
static char                                  s_debugIndent[128];
static TSFixedArray<FrameScript_EventObject> s_scriptEvents;
static lua_State                            *s_context;
static int                                   s_errorFunction;
static char                                  s_argName[] = "arg0";

static int  getglobal(lua_State *state);
static int  next(lua_State *state);
static int  debuginfo(lua_State *state);
static void print_variable(lua_State *state, LPCSTR name, int depth);
static void PushThisStack(FrameScript_Object *object);
static void PopThisStack();

void GetErrorFunction(lua_State *state) {
  if (s_errorFunction <= 0) {
    lua_getglobal(state, "_ERRORMESSAGE");
    s_errorFunction = luaL_ref(state, LUA_REGISTRYINDEX);
  }

  lua_rawgeti(state, LUA_REGISTRYINDEX, s_errorFunction);
}

FrameScript_Object::FrameScript_Object() : lua_registered(0), lua_objectRef(LUA_NOREF), m_onEvent(0) {
}

FrameScript_Object::~FrameScript_Object() {
  ASSERT(!lua_registered);
  SetOnEventScript(0);
  UnregisterAllScriptEvents();
}

LPCSTR FrameScript_Object::GetName() const {
  return 0;
}

FrameScript_EventObject::FrameScript_EventObject() : name(0) {
}

FrameScript_EventObject::FrameScript_EventObject(const FrameScript_EventObject &eventObject) : name(eventObject.name), list(eventObject.list) {
}

FrameScript_EventObject::~FrameScript_EventObject() {
  FREEIFUSED(name);
  ASSERT(list.IsEmpty());
}

void FrameScript_Object::RegisterScriptObject(LPCSTR name) {
  lua_State *state = FrameScript_GetContext();

  if (lua_registered) {
    lua_rawgeti(state, LUA_REGISTRYINDEX, lua_objectRef);
  } else {
    lua_newtable(state);

    lua_pushnumber(state, 0.0);
    lua_pushlightuserdata(state, this);
    lua_rawset(state, -3);

    lua_getglobal(state, "__framescript_meta");
    lua_setmetatable(state, -2);
  }

  if (name) {
    if (!lua_registered) {
      lua_objectRef = luaL_ref(state, LUA_REGISTRYINDEX);
      lua_rawgeti(state, LUA_REGISTRYINDEX, lua_objectRef);
    }

    ++lua_registered;
    lua_setglobal(state, name);
  }
}

void FrameScript_Object::UnregisterScriptObject(LPCSTR name) {
  lua_State *state = FrameScript_GetContext();

  if (name) {
    ASSERT(lua_registered > 0);

    --lua_registered;
    if (!lua_registered) {
      luaL_unref(state, LUA_REGISTRYINDEX, lua_objectRef);
      lua_objectRef = LUA_NOREF;
    }

    lua_pushnil(state);
    lua_setglobal(state, name);
  }
}

void FrameScript_Object::FillScriptMethodTable(
    FrameScript_Method                                   *methods,
    int                                                   methodCount,
    TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable
) {
  lua_State *state = FrameScript_GetContext();
  int        index;

  for (index = 0; index < methodCount; ++index) {
    FrameScriptObject_Variable *entry = methodTable.Ptr(methods[index].name);

    ASSERT(!entry);
    entry = methodTable.New(methods[index].name, 0, 0);
    lua_pushcfunction(state, methods[index].method);
    entry->reference = luaL_ref(state, LUA_REGISTRYINDEX);
  }
}

void FrameScript_Object::EmptyScriptMethodTable(TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable) {
  lua_State                  *state = FrameScript_GetContext();
  FrameScriptObject_Variable *entry = methodTable.Head();

  while (reinterpret_cast<long>(entry) > 0) {
    luaL_unref(state, LUA_REGISTRYINDEX, entry->reference);
    entry = methodTable.DeleteNode(entry);
  }
}

BOOL FrameScript_Object::LookupScriptMethod(lua_State *state, LPCSTR name, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable) {
  FrameScriptObject_Variable *entry = methodTable.Ptr(name);

  if (!entry) {
    return 0;
  }

  lua_rawgeti(state, LUA_REGISTRYINDEX, entry->reference);
  return 1;
}

int FrameScript_Object::LookupScriptMethod(lua_State *state) {
  FrameScript_Object *object;

  lua_rawgeti(state, 1, 0);
  if (lua_type(state, -1) == LUA_TLIGHTUSERDATA) {
    object = static_cast<FrameScript_Object *>(lua_touserdata(state, -1));
    lua_pop(state, 1);

    if (!object->LookupScriptMethod(state, lua_tostring(state, 2))) {
      lua_pushnil(state);
    }
    return 1;
  }

  lua_pop(state, 1);
  lua_rawget(state, 1);
  return 1;
}

BOOL FrameScript_Object::RegisterScriptEvent(LPCSTR name) {
  UINT                     count;
  UINT                     index;
  FrameScript_EventObject *eventObject;
  EVENTLISTENERNODE       *listener;

  ASSERT(name && *name);

  count = s_scriptEvents.Count();
  for (index = 0; index < count; ++index) {
    eventObject = &s_scriptEvents[index];
    if (!eventObject->name || SStrCmpI(eventObject->name, name, 0x7FFFFFFF)) {
      continue;
    }

    ITERATELIST(EVENTLISTENERNODE, eventObject->list, listener) {
      if (listener->object == this) {
        return 1;
      }
    }

    listener = eventObject->list.NewNode(LIST_TAIL, 0, 0);
    listener->object = this;
    return 1;
  }

  return 0;
}

void FrameScript_Object::UnregisterScriptEvent(LPCSTR name) {
  UINT                     count;
  UINT                     index;
  FrameScript_EventObject *eventObject;

  ASSERT(name && *name);

  count = s_scriptEvents.Count();
  for (index = 0; index < count; ++index) {
    eventObject = &s_scriptEvents[index];
    if (!eventObject->name || SStrCmpI(eventObject->name, name, 0x7FFFFFFF)) {
      continue;
    }

    ITERATELIST(EVENTLISTENERNODE, eventObject->list, listener) {
      if (listener->object == this) {
        eventObject->list.DeleteNode(listener);
        return;
      }
    }
    return;
  }
}

void FrameScript_Object::UnregisterAllScriptEvents() {
  UINT count = s_scriptEvents.Count();
  UINT index;

  for (index = 0; index < count; ++index) {
    FrameScript_EventObject *eventObject = &s_scriptEvents[index];

    ITERATELIST(EVENTLISTENERNODE, eventObject->list, listener) {
      if (listener->object == this) {
        eventObject->list.DeleteNode(listener);
        break;
      }
    }
  }
}

void FrameScript_Object::SetEventScript(int &script, LPCSTR source, LPCSTR description) {
  if (script) {
    FrameScript_ReleaseFunction(script);
  }

  if (source && *source) {
    script = FrameScript_CompileFunction(source, description);
  } else {
    script = 0;
  }
}

void FrameScript_Object::SetOnEventScript(LPCSTR source) {
  char description[1024];

  SStrPrintf(description, sizeof(description), "%s:OnEvent", GetName());
  SetEventScript(m_onEvent, source, description);
}

void FrameScript_Object::OnScriptEvent(LPCSTR name) {
  lua_State *state;

  if (!m_onEvent) {
    return;
  }

  state = FrameScript_GetContext();
  lua_pushstring(state, name);
  lua_setglobal(state, "event");
  FrameScript_Execute(m_onEvent, this);
  lua_pushnil(state);
  lua_setglobal(state, "event");
}

void __cdecl FrameScript_Object::OnScriptEvent(LPCSTR name, LPCSTR format, char *arguments) {
  lua_State *state;

  if (!m_onEvent) {
    return;
  }

  state = FrameScript_GetContext();
  lua_pushstring(state, name);
  lua_setglobal(state, "event");
  FrameScript_ExecuteV(m_onEvent, this, format, arguments);
  lua_pushnil(state);
  lua_setglobal(state, "event");
}

static int getglobal(lua_State *state) {
  LPCSTR name = lua_tostring(state, 1);

  lua_getglobal(state, name);
  return 1;
}

static int next(lua_State *state) {
  luaL_checktype(state, 1, LUA_TTABLE);
  lua_settop(state, 2);
  if (lua_next(state, 1)) {
    return 2;
  }

  lua_pushnil(state);
  return 1;
}

static int debuginfo(lua_State *state) {
  lua_Debug debugInfo;
  int       present;
  char      arg;
  int       index;
  LPCSTR    name;
  int       count;
  char      description[32];

  SOutputDebugString("// ==========================================================\n");

  if (lua_getstack(state, 1, &debugInfo)) {
    lua_getinfo(state, "S", &debugInfo);
    SOutputDebugString("// DEBUGINFO - %s:%d\n", debugInfo.source, debugInfo.currentline);
  }

  lua_getglobal(state, "this");
  if (lua_type(state, -1)) {
    print_variable(state, "this", 0);
  }
  lua_pop(state, 1);

  lua_getglobal(state, "event");
  if (lua_type(state, -1)) {
    print_variable(state, "event", 0);
  }
  lua_pop(state, 1);

  present = 1;
  arg = 1;
  do {
    s_argName[3] = arg + '0';
    lua_getglobal(state, s_argName);
    if (lua_type(state, -1)) {
      print_variable(state, s_argName, 0);
    } else {
      present = 0;
    }
    lua_pop(state, 1);
    ++arg;
  } while (present);

  if (lua_getstack(state, 1, &debugInfo)) {
    index = 1;
    while ((name = lua_getlocal(state, &debugInfo, index)) != 0) {
      print_variable(state, name, 0);
      lua_pop(state, 1);
      ++index;
    }
  }

  count = lua_gettop(state);
  for (index = 1; index <= count; ++index) {
    SStrPrintf(description, sizeof(description), "debuginfo[%d]", index);
    lua_pushvalue(state, index);
    print_variable(state, description, 0);
    lua_pop(state, 1);
  }

  return 0;
}

static void print_variable(lua_State *state, LPCSTR name, int depth) {
  FrameScript_Object *object;
  LPCSTR              tableName;
  lua_Debug           debugInfo;
  char                keyBuffer[32];
  LPCSTR              keyName;
  int                 type;

  if (static_cast<UINT>(depth) < sizeof(s_debugIndent)) {
    if (depth > 0) {
      s_debugIndent[depth - 1] = ' ';
    }
    s_debugIndent[depth] = 0;
  }

  type = lua_type(state, -1);
  switch (type) {
    case LUA_TNIL:
      SOutputDebugString("%s%s = <nil>\n", s_debugIndent, name);
      break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      SOutputDebugString("%s%s = <userdata>\n", s_debugIndent, name);
      break;

    case LUA_TNUMBER:
    case LUA_TSTRING:
      SOutputDebugString("%s%s = %s\n", s_debugIndent, name, lua_tostring(state, -1));
      break;

    case LUA_TTABLE:
      object = 0;
      tableName = "<table>";

      lua_rawgeti(state, -1, 0);
      if (lua_type(state, -1) == LUA_TLIGHTUSERDATA) {
        object = static_cast<FrameScript_Object *>(lua_touserdata(state, -1));
      }
      lua_pop(state, 1);

      if (object) {
        if (object->GetName()) {
          tableName = object->GetName();
        } else {
          tableName = "<unnamed>";
        }
      }

      SOutputDebugString("%s%s = %s {\n", s_debugIndent, name, tableName);

      lua_pushnil(state);
      while (lua_next(state, -2)) {
        if (lua_isnumber(state, -2)) {
          SStrPrintf(keyBuffer, sizeof(keyBuffer), "%d", static_cast<int>(lua_tonumber(state, -2)));
          keyName = keyBuffer;
        } else {
          keyName = lua_tostring(state, -2);
        }

        print_variable(state, keyName, depth + 1);
        lua_pop(state, 1);
      }

      SOutputDebugString("%s}\n", s_debugIndent);
      break;

    case LUA_TFUNCTION:
      lua_pushvalue(state, -1);
      lua_getinfo(state, ">nS", &debugInfo);
      if (debugInfo.name) {
        SOutputDebugString("%s%s = %s() defined %s:%d\n", s_debugIndent, name, debugInfo.name, debugInfo.source, debugInfo.linedefined);
      } else {
        SOutputDebugString("%s%s = <function> defined %s:%d\n", s_debugIndent, name, debugInfo.source, debugInfo.linedefined);
      }
      break;

    default:
      SOutputDebugString("%s%s = <%s>\n", s_debugIndent, name, lua_typename(state, -1));
      break;
  }

  if (static_cast<UINT>(depth) < sizeof(s_debugIndent) && depth > 0) {
    s_debugIndent[depth - 1] = 0;
  }
}

int FrameScript_Initialize() {
  LPVOID buffer;
  DWORD  bytes;

  ASSERT(!s_context);

  s_context = lua_open();
  lua_disablegc(s_context);

  lua_pushstring(s_context, "__framescript_meta");
  lua_newtable(s_context);
  lua_pushstring(s_context, "__index");
  lua_pushcfunction(s_context, FrameScript_Object::LookupScriptMethod);
  lua_settable(s_context, -3);
  lua_settable(s_context, LUA_GLOBALSINDEX);

  lua_pushstring(s_context, "getglobal");
  lua_pushcfunction(s_context, getglobal);
  lua_settable(s_context, LUA_GLOBALSINDEX);

  lua_pushstring(s_context, "next");
  lua_pushcfunction(s_context, next);
  lua_settable(s_context, LUA_GLOBALSINDEX);

  lua_pushstring(s_context, "debuginfo");
  lua_pushcfunction(s_context, debuginfo);
  lua_settable(s_context, LUA_GLOBALSINDEX);

  luaopen_string(s_context);
  luaopen_table(s_context);
  luaopen_math(s_context);

  if (SFile::LoadFile("Interface\\FrameXML\\compat.lua", &buffer, &bytes, 0, 0)) {
    luaL_loadbuffer(s_context, static_cast<LPCSTR>(buffer), bytes, "compat.lua");
    SFile::Unload(buffer);

    if (lua_pcall(s_context, 0, 0, 0)) {
      luaL_error(s_context, "Error executing compat script");
    }
  }

  lua_getglobal(s_context, "_ERRORMESSAGE");
  s_errorFunction = luaL_ref(s_context, LUA_REGISTRYINDEX);
  return 1;
}

void FrameScript_Flush() {
  if (s_context) {
    FrameScript_Destroy();
    FrameScript_Initialize();
  }
}

void FrameScript_Destroy() {
  ASSERT(s_context);

  lua_close(s_context);
  s_context = 0;
  s_objectStack.Clear();
  s_objectCount = 0;
}

void FrameScript_MemoryCleanup(int enableGC) {
  lua_State *state = FrameScript_GetContext();

  lua_enablegc(state);
  lua_setgcthreshold(state, 0);
  if (!enableGC) {
    lua_disablegc(state);
  }
}

int FrameScript_LoadTextTables(LPCSTR filename) {
  return FrameScript_ExecuteFile(filename);
}

LPCSTR FrameScript_GetText(LPCSTR text, int unk, FRAMESCRIPT_GENDER gender) {
  LPCSTR     result = "";
  lua_State *state;
  int        errorIndex;

  if (gender == GENDER_NOT_APPLICABLE && unk < 0) {
    FrameScript_GetVariable(text, result);
    return result;
  }

  state = FrameScript_GetContext();
  GetErrorFunction(state);
  errorIndex = lua_gettop(state);

  lua_getglobal(state, "GetText");
  lua_pushstring(state, text);

  if (gender != GENDER_NOT_APPLICABLE) {
    lua_pushnumber(state, static_cast<int>(gender));
  } else {
    lua_pushnil(state);
  }

  if (unk >= 0) {
    lua_pushnumber(state, unk);
  } else {
    lua_pushnil(state);
  }

  lua_pcall(state, 3, 1, errorIndex);
  if (lua_isstring(state, -1)) {
    result = lua_tostring(state, -1);
  }
  lua_settop(state, -3);
  return result;
}

UINT FrameScript_GetPluralIndex(int value) {
  UINT       result = 0;
  lua_State *state = FrameScript_GetContext();

  GetErrorFunction(state);
  lua_getglobal(state, "GetPluralIndex");
  lua_pushnumber(state, value);

  if (!lua_pcall(state, 1, 1, -3) && lua_isnumber(state, -1)) {
    result = static_cast<int>(lua_tonumber(state, -1)) - 1;
  }

  lua_pop(state, 2);
  return result;
}

void FrameScript_CreateEvents(LPCSTR *const names, UINT count) {
  UINT index;

  s_scriptEvents.Clear();
  s_scriptEvents.SetCount(count);

  for (index = 0; index < count; ++index) {
    FrameScript_EventObject *eventObject = &s_scriptEvents[index];
    LPCSTR                   name = names[index];

    if (name && *name) {
      eventObject->name = SStrDupA(name, __FILE__, __LINE__);
    }
  }
}

void FrameScript_SignalEvent(UINT index) {
  FrameScript_EventObject *eventObject;

  ASSERT(index < s_scriptEvents.Count());
  ASSERT(s_scriptEvents[index].name);

  eventObject = &s_scriptEvents[index];
  ITERATELIST(EVENTLISTENERNODE, eventObject->list, listener) {
    listener->object->OnScriptEvent(eventObject->name);
  }
}

void __cdecl FrameScript_SignalEvent(UINT index, LPCSTR format, ...) {
  FrameScript_EventObject *eventObject;
  va_list                  arguments;

  va_start(arguments, format);

  ASSERT(index < s_scriptEvents.Count());
  ASSERT(s_scriptEvents[index].name);

  eventObject = &s_scriptEvents[index];
  ITERATELIST(EVENTLISTENERNODE, eventObject->list, listener) {
    listener->object->OnScriptEvent(eventObject->name, format, arguments);
  }

  va_end(arguments);
}

void FrameScript_DestroyEvents() {
  s_scriptEvents.Clear();
}

lua_State *FrameScript_GetContext() {
  ASSERT(s_context);
  return s_context;
}
void __cdecl FrameScript_DisplayError(LPCSTR format, ...) {
  char       error[1024];
  lua_State *state = FrameScript_GetContext();
  va_list    arguments;

  va_start(arguments, format);
  SStrVPrintf(error, sizeof(error), format, arguments);
  luaL_error(state, error);
  va_end(arguments);
}

void FrameScript_RegisterFunction(LPCSTR name, int (*function)(lua_State *state)) {
  lua_State *state = FrameScript_GetContext();

  lua_pushcfunction(state, function);
  lua_setglobal(state, name);
}

void FrameScript_UnregisterFunction(LPCSTR name) {
  lua_State *state = FrameScript_GetContext();

  lua_pushnil(state);
  lua_setglobal(state, name);
}

void FrameScript_SetVariable(LPCSTR name, int value) {
  lua_State *state = FrameScript_GetContext();

  lua_pushnumber(state, value);
  lua_setglobal(state, name);
}

int FrameScript_GetVariable(LPCSTR name, int &value) {
  int        result = 0;
  lua_State *state = FrameScript_GetContext();

  lua_getglobal(state, name);
  if (lua_isnumber(state, -1)) {
    result = 1;
    value = static_cast<int>(lua_tonumber(state, -1));
  }

  lua_pop(state, 1);
  return result;
}

void FrameScript_SetVariable(LPCSTR name, float value) {
  lua_State *state = FrameScript_GetContext();

  lua_pushnumber(state, value);
  lua_setglobal(state, name);
}

int FrameScript_GetVariable(LPCSTR name, float &value) {
  int        result = 0;
  lua_State *state = FrameScript_GetContext();

  lua_getglobal(state, name);
  if (lua_isnumber(state, -1)) {
    result = 1;
    value = static_cast<float>(lua_tonumber(state, -1));
  }

  lua_pop(state, 1);
  return result;
}

void FrameScript_SetVariable(LPCSTR name, LPCSTR value) {
  lua_State *state = FrameScript_GetContext();

  if (value && *value) {
    lua_pushstring(state, value);
  } else {
    lua_pushnil(state);
  }
  lua_setglobal(state, name);
}

int FrameScript_GetVariable(LPCSTR name, LPCSTR &value) {
  int        found = 0;
  lua_State *state = FrameScript_GetContext();

  lua_getglobal(state, name);
  if (lua_isstring(state, -1)) {
    found = 1;
    value = lua_tostring(state, -1);
  }
  lua_pop(state, 1);
  return found;
}

void FrameScript_UnsetVariable(LPCSTR name) {
  lua_State *state = FrameScript_GetContext();

  lua_pushnil(state);
  lua_setglobal(state, name);
}

int FrameScript_ExecuteFile(LPCSTR filename) {
  LPVOID buffer;
  DWORD  bytes;
  int    result;

  if (!SFile::LoadFile(filename, &buffer, &bytes, 0, 0)) {
    return 0;
  }

  result = FrameScript_ExecuteBuffer(buffer, bytes, filename);
  SFile::Unload(buffer);
  return result;
}

int FrameScript_ExecuteBuffer(LPVOID buffer, DWORD bytes, LPCSTR filename) {
  lua_State *state = FrameScript_GetContext();

  GetErrorFunction(state);
  if (luaL_loadbuffer(state, static_cast<LPCSTR>(buffer), bytes, filename)) {
    if (lua_pcall(state, 1, 0, -2)) {
      lua_pop(state, 1);
    }
    lua_pop(state, 1);
    return 0;
  }

  if (lua_pcall(state, 0, 0, -2)) {
    lua_pop(state, 2);
    return 0;
  }

  lua_pop(state, 1);
  return 1;
}

BOOL FrameScript_CompileFunction(LPCSTR source, LPCSTR description) {
  lua_State *state = FrameScript_GetContext();

  if (luaL_loadbuffer(state, source, SStrLen(source), description)) {
    return LUA_REFNIL;
  }

  return luaL_ref(state, LUA_REGISTRYINDEX);
}

void FrameScript_ReleaseFunction(int function) {
  lua_State *state = FrameScript_GetContext();

  luaL_unref(state, LUA_REGISTRYINDEX, function);
}

void FrameScript_Execute(LPCSTR buffer, LPCSTR filename) {
  FrameScript_ExecuteBuffer(const_cast<char *>(buffer), SStrLen(buffer), filename);
}

void FrameScript_Execute(int function) {
  lua_State *state = FrameScript_GetContext();

  GetErrorFunction(state);
  lua_rawgeti(state, LUA_REGISTRYINDEX, function);
  if (lua_pcall(state, 0, 0, -2)) {
    lua_pop(state, 1);
  }
  lua_pop(state, 1);
}

void FrameScript_Execute(int function, FrameScript_Object *objectTHIS) {
  lua_State *state;

  FATALASSERT(function);

  FATALASSERT(objectTHIS);

  state = FrameScript_GetContext();
  PushThisStack(objectTHIS);

  GetErrorFunction(state);
  lua_rawgeti(state, LUA_REGISTRYINDEX, function);
  if (lua_pcall(state, 0, 0, -2)) {
    lua_pop(state, 1);
  }
  lua_pop(state, 1);

  PopThisStack();
}

static void PushThisStack(FrameScript_Object *object) {
  if (s_objectStack.Count() == s_objectCount) {
    s_objectStack.SetCount(s_objectCount + 1);
  }
  if (s_objectCount) {
    s_objectStack[s_objectCount - 1]->UnregisterScriptObject("this");
  }
  s_objectStack[s_objectCount] = object;
  s_objectStack[s_objectCount]->RegisterScriptObject("this");
  ++s_objectCount;
}

static void PopThisStack() {
  --s_objectCount;
  s_objectStack[s_objectCount]->UnregisterScriptObject("this");
  if (s_objectCount) {
    s_objectStack[s_objectCount - 1]->RegisterScriptObject("this");
  }
}

void __cdecl FrameScript_Execute(int function, FrameScript_Object *objectTHIS, LPCSTR args_fmt, ...) {
  va_list arguments;

  va_start(arguments, args_fmt);
  FrameScript_ExecuteV(function, objectTHIS, args_fmt, arguments);
  va_end(arguments);
}

void __cdecl FrameScript_ExecuteV(int function, FrameScript_Object *objectTHIS, LPCSTR args_fmt, char *arguments) {
  lua_State *state;
  int        argCount;
  char       current;
  int        index;

  FATALASSERT(function);

  FATALASSERT(args_fmt);

  state = FrameScript_GetContext();
  argCount = 0;
  current = *args_fmt;

  while (current && argCount < 9) {
    ++args_fmt;
    if (current == '%') {
      switch (*args_fmt) {
        case 'd':
          lua_pushnumber(state, va_arg(arguments, int));
          break;

        case 'u':
          lua_pushnumber(state, va_arg(arguments, UINT));
          break;

        case 'f':
          lua_pushnumber(state, va_arg(arguments, double));
          break;

        case 's':
          lua_pushstring(state, va_arg(arguments, LPCSTR));
          break;

        default:
          current = *args_fmt;
          continue;
      }

      ++argCount;
      s_argName[3] = argCount + '0';
      lua_setglobal(state, s_argName);
    }

    current = *args_fmt;
  }

  if (objectTHIS) {
    PushThisStack(objectTHIS);
  }

  GetErrorFunction(state);
  lua_rawgeti(state, LUA_REGISTRYINDEX, function);
  if (lua_pcall(state, 0, 0, -2)) {
    lua_pop(state, 1);
  }
  lua_pop(state, 1);

  if (objectTHIS) {
    PopThisStack();
  }

  for (index = 1; index <= argCount; ++index) {
    lua_pushnil(state);
    s_argName[3] = index + '0';
    lua_setglobal(state, s_argName);
  }
}
