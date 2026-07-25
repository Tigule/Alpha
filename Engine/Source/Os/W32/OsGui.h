#pragma once

#include <stpl.h>
#include <Tempest/c2ivector.h>
#include <Tempest/cimvector.h>

class COsControl;
class COsDialog;
class COsMenu;
class COsTreeView;

namespace NTempest {
  class CiRect;
}

struct OsGuiCallbackParams {
  int   type;
  int   subType;
  int   code;
  void *user;
};

struct OsGuiMenuHotkey {
  int keyID;
  int modKeyID;
};

struct OsGuiTVDDInfo {
  COsTreeView *treeView;
  int          action;
  void        *dragItem;
  void        *targItem;
  int          targX;
  int          targY;
};

struct OsGuiTreeItemParams {
  int                 used;
  NTempest::CImVector color;
  void               *user;
};

class COsControl {
 public:
  COsControl(COsDialog *inDialog, int inType, short inID, unsigned int inFlags);
  COsControl(void *inWindow, int inType, short inID, unsigned int inFlags);
  virtual ~COsControl();

  virtual void OnDestroy() {
  }

  virtual int OnEvent(int inItemID, int inNotifyCode, int inCode);

  virtual int OnDraw(void *, unsigned int, NTempest::CiRect &) {
    return 0;
  }

  virtual void *OnSetColors(void *__formal) {
    return 0;
  }

  virtual int OnReturn() {
    return 0;
  }

  virtual int OnEscape() {
    return 0;
  }

  virtual int OnMouseDown() {
    return 0;
  }

  virtual int OnMouseUp() {
    return 0;
  }

  virtual void OnMouseMove(int, int) {
  }

  virtual void OnSizeChange() {
  }

  virtual void OnTextChange() {
  }

  virtual int OnNotify(int inCode, void *__formal);
  virtual int OnCommand(int inParam);
  virtual int OnScroll(int inParam);

  virtual int OnMouseWheel(int __formal) {
    return 0;
  }

  virtual int OnContextMenu(int inX, int inY);
  virtual int IsHandleFromControl(void *inHandle);

  virtual int CanDoClipboardAction(int __formal) {
    return 0;
  }

  virtual int DoClipboardAction(int __formal) {
    return 0;
  }

  virtual void SetValue(int inVal) {
  }

  virtual int GetValue() {
    return -1;
  }

  short GetID() {
    return mID;
  }

  int GetType() {
    return mType;
  }

  void *GetHandle() {
    return mHandle;
  }

  COsDialog *GetDialog() {
    return mDialog;
  }

 protected:
  void Initialize(void *inWindow, int inType, short inID, unsigned int inFlags);
  int  SendEvent(int inEvent, int inCode);

  unsigned int mFlags;
  COsDialog   *mDialog;
  short        mID;
  int          mType;
  void        *mHandle;
  void(__fastcall *mCallback)(const OsGuiCallbackParams &);
  void    *mCallbackParam;
  COsMenu *mContextMenu;
  int      mContextMenuEnabled;
  int      mRedrawLevel;
};

class COsDialog {
 public:
  COsDialog(void *inWindowHandle, unsigned int inFlags);
  ~COsDialog();

  COsControl  *FindControl(void *inHandle);
  int          ProcessMessage(void *inMsgData);
  void        *GetParentWindow();
  int          IsInFront();
  virtual void SetSize(int inW, int inH);
  int          OnCancel();
  int          OnEvent(int inItemID, int inNotifyCode, int inCode);
  int          OnControlTab();
  int          HasFlag(unsigned int inFlag);

 protected:
  void *mHandle;
  void *mTooltips;
  void(__fastcall *mCallback)(const OsGuiCallbackParams &);
  void                         *mCallbackParam;
  TSGrowableArray<COsControl *> mControls;
  COsControl                   *mCancelButton;
  int                           mTrackMouse;
  int                           mMouseInside;
  int                           mNeedNewTrack;
  int                           mTooltipsEnabled;
  int                           mContextMenuEnabled;
  COsMenu                      *mContextMenu;
  unsigned int                  mFlags;
  TSGrowableArray<void *>       mDisabledWindows;
  NTempest::C2iVector           mMinSize;
};

class COsEditBox : public COsControl {
 public:
  COsEditBox(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsEditBox(void *inWindow, short inID, unsigned int inFlags);

  virtual int OnReturn();
  virtual int CanDoClipboardAction(int inAction);
  virtual int DoClipboardAction(int inAction);
  int         GetSelectionSize();
  int         IsCharacterAllowed(char inChar);
  void        UpdateSelection();

 protected:
  void Initialize();

  int          mFiltersEnabled;
  unsigned int mFilters;
  int          mSelSize;
};

class COsTreeView : public COsControl {
 public:
  COsTreeView(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsTreeView(void *inWindow, short inID, unsigned int inFlags);
  virtual ~COsTreeView();

  virtual int  OnReturn();
  virtual int  OnEscape();
  virtual int  OnMouseDown();
  virtual int  OnMouseUp();
  virtual void OnMouseMove(int inX, int inY);
  virtual void OnSizeChange();
  virtual int  OnNotify(int inCode, void *inParam);
  virtual int  IsHandleFromControl(void *inHandle);
  int          IsCharacterAllowed(char inChar);
  void         SelectItem(void *inItem, int inVal);

 protected:
  void *GetEditControl();

  void                *mImages;
  TSGrowableArray<int> mUnusedImageIDs;
  int                  mDragging;
  OsGuiTVDDInfo        mDragInfo;
  void                *mDragImage;
  int(__fastcall *mDragHandler)(const OsGuiTVDDInfo &, void *);
  void *mDragHandlerParam;
  int(__fastcall *mCanEditFunc)(void *, void *);
  void *mCanEditParam;
  void(__fastcall *mExpandFunc)(void *, void *);
  void                                *mExpandParam;
  int                                  mTextLimit;
  int                                  mFiltersEnabled;
  unsigned int                         mFilters;
  TSGrowableArray<OsGuiTreeItemParams> mItemParams;
};

class COsSpinButton : public COsControl {
 public:
  void OnSpinMouseUp();
};

class COsTabControl : public COsControl {
 public:
  COsTabControl(COsDialog *inDialog, short inID, unsigned int inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  int          GetNumItems();
  int          OnControlTab();
};

class COsMenuBar {
 public:
  COsMenuBar(void *inWindowHandle);
  ~COsMenuBar();

  void *GetWindow() {
    return mWindowHandle;
  }

  void *GetAccelerators() {
    return mAccelerators;
  }

 protected:
  TSGrowableArray<COsMenu *> mMenus;
  void                      *mWindowHandle;
  void                      *mMenuBarHandle;
  void                      *mAccelerators;
};

void __fastcall  OsGuiSetApplicationInfo(void *inData);
void __fastcall  OsGuiEnableMenuHotkeys(int inVal);
void __fastcall  OsGuiBeep();
void *__fastcall OsGuiGetWindow(int inWindowType);
int __fastcall   OsGuiMessageBox(void *inParentWindow, int inStyle, const char *inMessage, const char *inTitle);
int __fastcall   OsGuiProcessMessage(void *inMsgData);
int __fastcall   OsGuiIsModifierKeyDown(int inKey);
void __fastcall  OsGuiSetGxWindow(void *window);
void __fastcall  OsGuiSetWindowTitle(void *inWindow, const char *inText);
