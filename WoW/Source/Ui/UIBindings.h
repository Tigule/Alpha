#ifndef WOW_SOURCE_UI_UIBINDINGS_H
#define WOW_SOURCE_UI_UIBINDINGS_H

#include <stpl.h>

class CKeyEvent;
class CMouseEvent;
class CStatus;

struct KEYBINDING : public TSHashObject<KEYBINDING, HASHKEY_STRI> {
  KEYBINDING() : index(0), command(0) {
  }
  ~KEYBINDING() {
    FREEIFUSED(command);
  }

  int   index;
  char *command;
};

struct KEYCOMMAND : public TSHashObject<KEYCOMMAND, HASHKEY_STRI> {
  KEYCOMMAND() {
  }
  ~KEYCOMMAND() {
  }

  int index;
  int headerIndex;
  int function;
  int runOnUp;
};

class CGUIBindings {
 public:
  static CGUIBindings *__fastcall Initialize(const char *commandsFile, CStatus *status);
  static void __fastcall          Shutdown();
  static void __fastcall          LoadBindings(int useDefault);
  static void __fastcall          SaveBindings();
  static const char *__fastcall   KeyEventToString(const CKeyEvent &evt, char *string, int maxLen);
  static const char *__fastcall   MouseEventToString(const CMouseEvent &evt, char *string, int maxLen);

  static CGUIBindings *GetActive() {
    return s_bindings;
  }

  CGUIBindings();
  ~CGUIBindings();

  int Load(const char *commandsFile, CStatus *status);
  int Bind(const char *keystring, const char *command);
  int ExecKey(const char *keystring, unsigned long timestamp, int down);
  int ExecCommand(const char *command, unsigned long timestamp, int down);
  int GetNumCommands() const {
    return m_numCommands;
  }
  int GetNumHiddenCommands() const {
    return m_numHiddenCommands;
  }
  void         GetCommand(int index, const char *&command);
  void         GetHiddenCommand(int index, const char *&command);
  const char  *GetCommandKey(const char *command, int keyindex);
  unsigned int GetNumCommandKeys(const char *command);
  const char  *GetCommandAction(const char *keystring);
  void         AdjustCommandKeyIndices(const char *command, int index);
  void         ClearBindings() {
    m_bindings.Clear();
  }

 private:
  static int __fastcall AddMetaPrefix(unsigned int metaKeyState, char *&string, int &maxLen);

  static CGUIBindings *s_bindings;

  int                                   m_numCommands;
  int                                   m_numHiddenCommands;
  TSHashTable<KEYBINDING, HASHKEY_STRI> m_bindings;
  TSHashTable<KEYCOMMAND, HASHKEY_STRI> m_commands;
};

void __fastcall UIBindingsRegisterScriptFunctions();
void __fastcall UIBindingsUnegisterScriptFunctions();

#endif
