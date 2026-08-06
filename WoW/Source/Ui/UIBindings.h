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
  static CGUIBindings *Initialize(LPCSTR commandsFile, CStatus *status);
  static void          Shutdown();
  static void          LoadBindings(int useDefault);
  static void          SaveBindings();
  static LPCSTR        KeyEventToString(const CKeyEvent &evt, char *string, int maxLen);
  static LPCSTR        MouseEventToString(const CMouseEvent &evt, char *string, int maxLen);

  static CGUIBindings *GetActive() {
    return s_bindings;
  }

  CGUIBindings();
  ~CGUIBindings();

  int Load(LPCSTR commandsFile, CStatus *status);
  int Bind(LPCSTR keystring, LPCSTR command);
  int ExecKey(LPCSTR keystring, DWORD timestamp, int down) const;
  int ExecCommand(LPCSTR command, DWORD timestamp, int down) const;
  int GetNumCommands() const {
    return m_numCommands;
  }
  int GetNumHiddenCommands() const {
    return m_numHiddenCommands;
  }
  void   GetCommand(int index, LPCSTR &command) const;
  void   GetHiddenCommand(int index, LPCSTR &command) const;
  LPCSTR GetCommandKey(LPCSTR command, int keyindex) const;
  UINT   GetNumCommandKeys(LPCSTR command) const;
  LPCSTR GetCommandAction(LPCSTR keystring) const;
  void   AdjustCommandKeyIndices(LPCSTR command, int index) const;
  void   ClearBindings() {
    m_bindings.Clear();
  }

 protected:
  static int AddMetaPrefix(UINT metaKeyState, char *&string, int &maxLen);

 private:
  static CGUIBindings *s_bindings;

  int                                           m_numCommands;
  int                                           m_numHiddenCommands;
  mutable TSHashTable<KEYBINDING, HASHKEY_STRI> m_bindings;
  TSHashTable<KEYCOMMAND, HASHKEY_STRI>         m_commands;
};

void UIBindingsRegisterScriptFunctions();
void UIBindingsUnegisterScriptFunctions();

#endif
