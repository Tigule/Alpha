#ifndef ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H
#define ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H

#include <stpl.h>

struct lua_State;

struct FrameScript_Method {
  LPCSTR name;
  int (*method)(lua_State *state);
};

struct FrameScriptObject_Variable : public TSHashObject<FrameScriptObject_Variable, HASHKEY_STR> {
  int reference;
};

enum FRAMESCRIPT_GENDER {
  GENDER_NOT_APPLICABLE = 0,
  GENDER_NONE = 1,
  GENDER_MALE = 2,
  GENDER_FEMALE = 3,
  GENDER_MALE_PLURAL = 4,
  GENDER_FEMALE_PLURAL = 5,
  GENDER_MIXED_PLURAL = 6
};

class FrameScript_Object {
 public:
  FrameScript_Object();
  FrameScript_Object(lua_State *state);
  virtual ~FrameScript_Object();

  virtual LPCSTR GetName() const;

  void RegisterScriptObject(LPCSTR name);
  void UnregisterScriptObject(LPCSTR name);

  static void FillScriptMethodTable(FrameScript_Method *methods, int methodCount, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static void EmptyScriptMethodTable(TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static int  LookupScriptMethod(lua_State *state);
  static int  LookupScriptMethod(lua_State *state, LPCSTR name, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);

  int          RegisterScriptEvent(LPCSTR name);
  void         UnregisterScriptEvent(LPCSTR name);
  void         UnregisterAllScriptEvents();
  void         SetEventScript(int &script, LPCSTR source, LPCSTR description);
  void         SetOnEventScript(LPCSTR source);
  void         OnScriptEvent(LPCSTR name);
  void __cdecl OnScriptEvent(LPCSTR name, LPCSTR format, char *arguments);

 protected:
  virtual int LookupScriptMethod(lua_State *state, LPCSTR name) = 0;

  int lua_registered;
  int lua_objectRef;
  int m_onEvent;
};

int          FrameScript_Initialize();
void         FrameScript_Destroy();
void         FrameScript_Flush();
void         FrameScript_MemoryCleanup(int enableGC);
lua_State   *FrameScript_GetContext();
void         FrameScript_CreateEvents(LPCSTR *const names, UINT count);
int          FrameScript_LoadTextTables(LPCSTR filename);
UINT         FrameScript_GetPluralIndex(int value);
void         FrameScript_DestroyEvents();
void __cdecl FrameScript_DisplayError(LPCSTR format, ...);
void         FrameScript_RegisterFunction(LPCSTR name, int (*function)(lua_State *state));
void         FrameScript_UnregisterFunction(LPCSTR name);
void         FrameScript_SetVariable(LPCSTR name, int value);
int          FrameScript_GetVariable(LPCSTR name, int &value);
void         FrameScript_SetVariable(LPCSTR name, float value);
int          FrameScript_GetVariable(LPCSTR name, float &value);
void         FrameScript_SetVariable(LPCSTR name, LPCSTR value);
int          FrameScript_ExecuteFile(LPCSTR filename);
int          FrameScript_ExecuteBuffer(LPVOID buffer, DWORD bytes, LPCSTR filename);
int          FrameScript_CompileFunction(LPCSTR source, LPCSTR description);
void         FrameScript_ReleaseFunction(int function);
void         FrameScript_UnsetVariable(LPCSTR name);
void         FrameScript_Execute(LPCSTR buffer, LPCSTR filename);
void         FrameScript_Execute(int function);
void         FrameScript_Execute(int function, FrameScript_Object *objectTHIS);
void __cdecl FrameScript_Execute(int function, FrameScript_Object *objectTHIS, LPCSTR args_fmt, ...);
void __cdecl FrameScript_ExecuteV(int function, FrameScript_Object *objectTHIS, LPCSTR args_fmt, char *arguments);
LPCSTR       FrameScript_GetText(LPCSTR text, int unk, FRAMESCRIPT_GENDER gender);
int          FrameScript_GetVariable(LPCSTR name, LPCSTR &value);
void         FrameScript_SignalEvent(UINT index);
void __cdecl FrameScript_SignalEvent(UINT index, LPCSTR format, ...);
void         RegisterSimpleFrameScriptMethods();
void         UnregisterSimpleFrameScriptMethods();

#endif
