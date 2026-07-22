#ifndef ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H
#define ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H

#include <stpl.h>

struct lua_State;

struct FrameScript_Method {
  const char *name;
  int(__fastcall *method)(lua_State *state);
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
  virtual ~FrameScript_Object();

  virtual const char *GetName() const;

  void RegisterScriptObject(const char *name);
  void UnregisterScriptObject(const char *name);

  static void __fastcall
  FillScriptMethodTable(FrameScript_Method *methods, int methodCount, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static void __fastcall EmptyScriptMethodTable(TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static int __fastcall  LookupScriptMethod(lua_State *state);
  static int __fastcall  LookupScriptMethod(lua_State *state, const char *name, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);

  int          RegisterScriptEvent(const char *name);
  void         UnregisterScriptEvent(const char *name);
  void         UnregisterAllScriptEvents();
  void         SetEventScript(int &script, const char *source, const char *description);
  void         SetOnEventScript(const char *source);
  void         OnScriptEvent(const char *name);
  void __cdecl OnScriptEvent(const char *name, const char *format, char *arguments);

 protected:
  virtual int LookupScriptMethod(lua_State *state, const char *name) = 0;

  int lua_registered;
  int lua_objectRef;
  int m_onEvent;
};

int __fastcall          FrameScript_Initialize();
void __fastcall         FrameScript_Destroy();
void __fastcall         FrameScript_Flush();
void __fastcall         FrameScript_MemoryCleanup(int enableGC);
lua_State *__fastcall   FrameScript_GetContext();
void __fastcall         FrameScript_CreateEvents(const char **const names, unsigned int count);
int __fastcall          FrameScript_LoadTextTables(const char *filename);
unsigned int __fastcall FrameScript_GetPluralIndex(int value);
void __fastcall         FrameScript_DestroyEvents();
void __cdecl            FrameScript_DisplayError(const char *format, ...);
void __fastcall         FrameScript_RegisterFunction(const char *name, int(__fastcall *function)(lua_State *state));
void __fastcall         FrameScript_UnregisterFunction(const char *name);
void __fastcall         FrameScript_SetVariable(const char *name, int value);
int __fastcall          FrameScript_GetVariable(const char *name, int &value);
void __fastcall         FrameScript_SetVariable(const char *name, float value);
int __fastcall          FrameScript_GetVariable(const char *name, float &value);
void __fastcall         FrameScript_SetVariable(const char *name, const char *value);
int __fastcall          FrameScript_ExecuteFile(const char *filename);
int __fastcall          FrameScript_ExecuteBuffer(void *buffer, unsigned long bytes, const char *filename);
int __fastcall          FrameScript_CompileFunction(const char *source, const char *description);
void __fastcall         FrameScript_ReleaseFunction(int function);
void __fastcall         FrameScript_UnsetVariable(const char *name);
void __fastcall         FrameScript_Execute(const char *buffer, const char *filename);
void __fastcall         FrameScript_Execute(int function);
void __fastcall         FrameScript_Execute(int function, FrameScript_Object *objectTHIS);
void __cdecl            FrameScript_Execute(int function, FrameScript_Object *objectTHIS, const char *args_fmt, ...);
void __cdecl            FrameScript_ExecuteV(int function, FrameScript_Object *objectTHIS, const char *args_fmt, char *arguments);
const char *__fastcall  FrameScript_GetText(const char *text, int unk, FRAMESCRIPT_GENDER gender);
int __fastcall          FrameScript_GetVariable(const char *name, const char *&value);
void __fastcall         FrameScript_SignalEvent(unsigned int index);
void __cdecl            FrameScript_SignalEvent(unsigned int index, const char *format, ...);
void __fastcall         RegisterSimpleFrameScriptMethods();
void __fastcall         UnregisterSimpleFrameScriptMethods();

#endif
