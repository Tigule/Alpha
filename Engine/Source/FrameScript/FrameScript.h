#ifndef ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H
#define ENGINE_SOURCE_FRAMESCRIPT_FRAMESCRIPT_H

#include <stpl.h>

struct lua_State;

struct FrameScript_Method {
  const char *name;
  int(*method)(lua_State *state);
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

  virtual const char *GetName() const;

  void RegisterScriptObject(const char *name);
  void UnregisterScriptObject(const char *name);

  static void
  FillScriptMethodTable(FrameScript_Method *methods, int methodCount, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static void EmptyScriptMethodTable(TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);
  static int LookupScriptMethod(lua_State *state);
  static int LookupScriptMethod(lua_State *state, const char *name, TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> &methodTable);

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

int FrameScript_Initialize();
void FrameScript_Destroy();
void FrameScript_Flush();
void FrameScript_MemoryCleanup(int enableGC);
lua_State *FrameScript_GetContext();
void FrameScript_CreateEvents(const char **const names, unsigned int count);
int FrameScript_LoadTextTables(const char *filename);
unsigned int FrameScript_GetPluralIndex(int value);
void FrameScript_DestroyEvents();
void __cdecl            FrameScript_DisplayError(const char *format, ...);
void FrameScript_RegisterFunction(const char *name, int(*function)(lua_State *state));
void FrameScript_UnregisterFunction(const char *name);
void FrameScript_SetVariable(const char *name, int value);
int FrameScript_GetVariable(const char *name, int &value);
void FrameScript_SetVariable(const char *name, float value);
int FrameScript_GetVariable(const char *name, float &value);
void FrameScript_SetVariable(const char *name, const char *value);
int FrameScript_ExecuteFile(const char *filename);
int FrameScript_ExecuteBuffer(void *buffer, unsigned long bytes, const char *filename);
int FrameScript_CompileFunction(const char *source, const char *description);
void FrameScript_ReleaseFunction(int function);
void FrameScript_UnsetVariable(const char *name);
void FrameScript_Execute(const char *buffer, const char *filename);
void FrameScript_Execute(int function);
void FrameScript_Execute(int function, FrameScript_Object *objectTHIS);
void __cdecl            FrameScript_Execute(int function, FrameScript_Object *objectTHIS, const char *args_fmt, ...);
void __cdecl            FrameScript_ExecuteV(int function, FrameScript_Object *objectTHIS, const char *args_fmt, char *arguments);
const char *FrameScript_GetText(const char *text, int unk, FRAMESCRIPT_GENDER gender);
int FrameScript_GetVariable(const char *name, const char *&value);
void FrameScript_SignalEvent(unsigned int index);
void __cdecl            FrameScript_SignalEvent(unsigned int index, const char *format, ...);
void RegisterSimpleFrameScriptMethods();
void UnregisterSimpleFrameScriptMethods();

#endif
