#pragma once

#include <stpl.h>
#include <Tempest/c2ivector.h>
#include <Tempest/cimvector.h>
#include <Tempest/cirect.h>

class COsControl;
class COsDialog;
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

class COsMenu {
 public:
  COsMenu();
  COsMenu(unsigned char inID, const char *inTitle);
  ~COsMenu();

  void Clear();
  int  GetNumItems();
  void AddTextItem(int inPos, const char *inText, OsGuiMenuHotkey *inHotkey);
  void AddSubMenu(int inPos, const char *inTitle, COsMenu *inMenu);
  void AddSeparator(int inPos);
  int  GetHotkey(int inPos, OsGuiMenuHotkey *outHotkey);
  void EnableItem(int inPos, int inVal);
  void DisableItem(int inPos) {
    EnableItem(inPos, 0);
  }
  void CheckItem(int inPos, int inVal);
  void SetItemText(int inPos, const char *inText);
  void RemoveItem(int inPos);
  unsigned char GetID() {
    return mID;
  }
  void *GetMenuHandle() {
    return mMenuHandle;
  }
  char *GetTitle() {
    return mTitle;
  }

 protected:
  void AddHotkey(int inPos);
  void RemoveHotkey(int inPos);
  static void AppendHotkeyText(char *inText, const OsGuiMenuHotkey &inHotkey);

  unsigned char                      mID;
  void                              *mMenuHandle;
  char                               mTitle[32];
  TSGrowableArray<OsGuiMenuHotkey>   mHotkeys;
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

  OsGuiTreeItemParams();
};

struct OsGuiTVSelectionInfo {
  int          numSelected;
  void        *firstSelection;
  unsigned int flags;
};

class COsControl {
  friend class COsDialog;

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

  void SetRedraw(int inVal);
  void Refresh(int inErase);
  void SetCallback(void(*inFunc)(const OsGuiCallbackParams &), void *inParam);
  void SetFont(int inFont);
  void SetInputFocus();
  void LoseInputFocus();
  int  HasInputFocus();
  void SetText(const char *inText);
  void GetText(char *outText, int inBufSize);
  int  GetTextLength();
  void GetTextSize(const char *inText, int *outW, int *outH);
  void GetTextSize(int *outW, int *outH);
  void Show(int inVal);
  void Hide() {
    Show(0);
  }
  int  IsShowing();
  void Enable(int inVal);
  void Disable() {
    Enable(0);
  }
  int  IsEnabled();
  void SetPosition(int inX, int inY);
  void GetPosition(int *outX, int *outY, int inParentRelative);
  void SetSize(int inW, int inH);
  void GetSize(int *outW, int *outH);
  void SetTooltip(const char *inText);
  void SetContextMenu(COsMenu *inMenu);
  void EnableContextMenu(int inVal) {
    mContextMenuEnabled = inVal;
  }
  void DisableContextMenu() {
    EnableContextMenu(0);
  }

 protected:
  void Initialize(void *inWindow, int inType, short inID, unsigned int inFlags);
  int  SendEvent(int inEvent, int inCode);

  unsigned int mFlags;
  COsDialog   *mDialog;
  short        mID;
  int          mType;
  void        *mHandle;
  void(*mCallback)(const OsGuiCallbackParams &);
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
  void         AddControl(COsControl *inControl);
  int          FindControl(COsControl *inControl);
  void         DeleteControl(COsControl *inControl);
  void         DetachControl(COsControl *inControl);
  int          ProcessMessage(void *inMsgData);
  void         CheckEvents();
  void        *GetHandle() {
    return mHandle;
  }
  void        *GetParentWindow();
  void        *GetTooltips();
  void         EnableTooltips(int inVal);
  void         DisableTooltips() {
    EnableTooltips(0);
  }
  void         SetCancelButton(COsControl *inControl) {
    mCancelButton = inControl;
  }
  void         SetTrackMouse(int inVal);
  int          IsMouseInside();
  void         SetCallback(void(*inFunc)(const OsGuiCallbackParams &), void *inParam);
  void         BringToFront();
  int          IsInFront();
  void         SetInputFocus();
  void         Show(int inVal);
  void         Hide() {
    Show(0);
  }
  int          IsShowing();
  int          IsEnabled();
  void         SetRedraw(int inVal);
  void         Refresh(int inErase);
  void         SetPosition(int inX, int inY);
  void         GetPosition(int *outX, int *outY, int inClient);
  virtual void SetSize(int inW, int inH);
  void         GetSize(int *outW, int *outH, int inClientOnly);
  void         SetMinSize(int inW, int inH);
  int          GetMinSize(int *outW, int *outH);
  void         SetTitle(const char *inText);
  void         SetContextMenu(COsMenu *inMenu);
  void         EnableContextMenu(int inVal) {
    mContextMenuEnabled = inVal;
  }
  void         DisableContextMenu() {
    EnableContextMenu(0);
  }
  int          CanDoClipboardAction(int inAction);
  int          DoClipboardAction(int inAction);
  int          OnAccept();
  int          OnCancel();
  int          OnMouseUp();
  int          OnMouseDown();
  int          OnMouseLeave();
  int          OnMouseMove(int inX, int inY);
  int          OnContextMenu(int inX, int inY);
  int          OnEvent(int inItemID, int inNotifyCode, int inCode);
  int          OnControlTab();
  int          HasFlag(unsigned int inFlag);

 protected:
  void ApplyModality(int inVal);

  void *mHandle;
  void *mTooltips;
  void(*mCallback)(const OsGuiCallbackParams &);
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

class COsButton : public COsControl {
 public:
  COsButton(COsDialog *inDialog, short inID, unsigned int inFlags);

  void SetDefaultButton();
  void SetCancelButton();
  void SetHighlight(int inVal);
};

class COsImageButton : public COsControl {
 public:
  COsImageButton(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsImageButton(void *inWindow, short inID, unsigned int inFlags);
  virtual ~COsImageButton();

  virtual void OnDestroy();

  void SetImage(int inWidth, int inHeight, void *inData);
  void SetHighlight(int inVal);
  int  IsPushed();
};

class COsTextButton : public COsControl {
 public:
  COsTextButton(COsDialog *inDialog, short inID);
  virtual ~COsTextButton() {
  }

  virtual int OnDraw(void *inContext, unsigned int inState, NTempest::CiRect &inRect);

  void SetActiveColor(const NTempest::CImVector &inColor) {
    mActiveColor = inColor;
  }
  void SetPushedColor(const NTempest::CImVector &inColor) {
    mPushedColor = inColor;
  }
  void SetGreyedColor(const NTempest::CImVector &inColor) {
    mGreyedColor = inColor;
  }
  void SetUnderline(int inVal) {
    mUnderline = inVal;
  }

 protected:
  NTempest::CImVector mActiveColor;
  NTempest::CImVector mPushedColor;
  NTempest::CImVector mGreyedColor;
  int                 mUnderline;
};

class COsStaticBox : public COsControl {
 public:
  COsStaticBox(COsDialog *inDialog, short inID, unsigned int inFlags)
      : COsControl(inDialog, 12, inID, inFlags) {
  }
  COsStaticBox(void *inWindow, short inID, unsigned int inFlags)
      : COsControl(inWindow, 12, inID, inFlags) {
  }
  virtual ~COsStaticBox() {
  }

  virtual int OnDraw(void *inContext, unsigned int inState, NTempest::CiRect &inRect);

  void ClearTransparentRects();
  void AddTransparentRect(const NTempest::CiRect &inRect);

 protected:
  TSGrowableArray<NTempest::CiRect> mTransRect;
};

class COsCheckbox : public COsControl {
 public:
  COsCheckbox(COsDialog *inDialog, short inID);
  COsCheckbox(void *inWindow, short inID);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  virtual int  OnEvent(int inItemID, int inNotifyCode, int inCode);
  virtual void OnTextChange();
  virtual void OnSizeChange();

  void SetMaxWidth(int inWidth);
  void ClearValue();
  int  HasValue();

 protected:
  int mSettingSize;
  int mMaxWidth;
};

class COsStaticText : public COsControl {
 public:
  COsStaticText(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsStaticText(void *inWindow, short inID, unsigned int inFlags);

  virtual void *OnSetColors(void *inContext);

  void SetJustification(int inJust);
  void SetTextColor(const NTempest::CImVector &inColor);

 protected:
  void Initialize();

  NTempest::CImVector mTextColor;
};

class COsStaticImage : public COsControl {
 public:
  COsStaticImage(COsDialog *inDialog, short inID);
  virtual ~COsStaticImage();

  virtual void OnDestroy();

  void SetImage(int inWidth, int inHeight, void *inData);
  void ClearImage();
};

class COsEditBox : public COsControl {
 public:
  COsEditBox(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsEditBox(void *inWindow, short inID, unsigned int inFlags);

  virtual int OnReturn();
  virtual int CanDoClipboardAction(int inAction);
  virtual int DoClipboardAction(int inAction);
  void        SetTextLimit(int inSize);
  void        SelectAll();
  int         GetSelectionSize();
  void        EnableFilters(int inVal);
  void        DisableFilters() {
    EnableFilters(0);
  }
  void        SetFilter(unsigned int inFilter, int inVal);
  int         IsCharacterAllowed(char inChar);
  void        UpdateSelection();

 protected:
  void Initialize();

  int          mFiltersEnabled;
  unsigned int mFilters;
  int          mSelSize;
};

class COsListBox : public COsControl {
 public:
  COsListBox(COsDialog *inDialog, short inID, unsigned int inFlags);
  virtual ~COsListBox();

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  virtual int  OnContextMenu(int inX, int inY);
  virtual int  OnReturn();

  void SelectItem(int inPos, int inVal);
  void DeselectItem(int inPos) {
    SelectItem(inPos, 0);
  }
  int  IsItemSelected(int inPos);
  void SelectAll(int inVal);
  void DeselectAll() {
    SelectAll(0);
  }
  void ClearItems();
  int  GetNumItems();
  void InsertItem(const char *inText, int inPos);
  void DeleteItem(int inPos);
  void SetItemText(int inPos, const char *inText);
  int  GetItemTextLength(int inPos);
  void GetItemText(int inPos, char *inBuf, int inBufSize);
  void SetItemHeight(int inHeight);
  int  GetItemHeight();
};

class COsListView : public COsControl {
 public:
  COsListView(COsDialog *inDialog, short inID, unsigned int inFlags);
  virtual ~COsListView();

  virtual void OnSizeChange();
  virtual void SetValue(int inVal);
  virtual int  GetValue();
  virtual int  OnNotify(int inCode, void *inParam);
  virtual int  OnReturn();

  void InsertColumn(int inPos);
  void DeleteColumn(int inPos);
  int  GetNumColumns();
  void InsertRow(int inPos);
  void DeleteRow(int inPos);
  void ClearRows();
  int  GetNumRows();
  void SetRowColor(int inPos, const NTempest::CImVector &inColor);
  NTempest::CImVector GetRowColor(int inPos);
  void SetItemText(int inRow, int inCol, const char *inText);
  void GetItemText(int inRow, int inCol, char *inBuf, int inBufSize);
  void SetColumnWidth(int inCol, int inWidth);
  int  GetColumnWidth(int inCol);
  void SetColumnTitle(int inCol, const char *inText);
  void GetColumnTitle(int inCol, char *inBuf, int inBufSize);
  void SetColumnJustification(int inCol, int inJustify);
  void EnsureRowVisible(int inRow);
  void OnSelectionChange();
  void OnColumnClick(int inCol);

 protected:
  int mNumCols;
};

class COsToolBar : public COsControl {
 public:
  COsToolBar(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsToolBar(void *inWindow, short inID, unsigned int inFlags);
  virtual ~COsToolBar();

  virtual int OnCommand(int inParam);

  void SetButtonSize(int inW, int inH);
  void GetButtonSize(int *outW, int *outH);
  void Clear();
  void AddButton(int inPos);
  void AddSeparator(int inPos);
  void RemoveButton(int inPos);
  int  GetNumButtons();
  void SetButtonImage(int inPos, int inWidth, int inHeight, void *inData);
  void SetButtonText(int inPos, const char *inText);
  void GetButtonText(int inPos, char *inBuf, int inBufSize);
  void EnableButton(int inPos, int inVal);
  void CheckButton(int inPos, int inVal);

 protected:
  void InitializeToolBar();

  void *mImageList;
};

class COsPopupMenu : public COsControl {
 public:
  COsPopupMenu(COsDialog *inDialog, short inID);
  virtual ~COsPopupMenu();

  virtual void SetValue(int inVal);
  virtual int  GetValue();

  void SetSize(int inW, int inH);
  void ClearItems();
  int  GetNumItems();
  void InsertItem(const char *inText, int inPos);
  void SetItemHeight(int inHeight);
  int  GetItemHeight();
  void SetMaxHeight(int inHeight);
  void DeleteItem(int inPos);
  void SetItemText(int inPos, const char *inText);

 protected:
  void AdjustHeight();

  int mBaseHeight;
  int mMaxHeight;
};

class COsProgressBar : public COsControl {
 public:
  COsProgressBar(COsDialog *inDialog, short inID);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
};

class COsRadioButton : public COsControl {
 public:
  COsRadioButton(COsDialog *inDialog, short inID, unsigned int inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
};

class COsSlider : public COsControl {
 public:
  COsSlider(COsDialog *inDialog, short inID);

  virtual void SetValue(int inVal);
  virtual int  GetValue();

  void SetMinValue(int inVal);
  void SetMaxValue(int inVal);
};

class COsScrollBar : public COsControl {
 public:
  COsScrollBar(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsScrollBar(void *inWindow, short inID, unsigned int inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  virtual int  OnScroll(int inParam);
  virtual int  OnMouseWheel(int inDelta);

  void SetRange(int inMin, int inMax);
  void SetPageSize(int inVal);

 protected:
  void Initialize();
  void UpdateRangeValues();

  int mRealMin;
  int mRealMax;
  int mPageSize;
};

class COsDivider : public COsControl {
 public:
  COsDivider(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsDivider(void *inWindow, short inID, unsigned int inFlags);
  virtual ~COsDivider();

  void SetPositionRange(int inMin, int inMax);
  void OnDivMouseDown();
  void OnDivMouseMove(int inX, int inY);
  void OnDivMouseUp();
  void OnDivMouseLeave();

 protected:
  void Initialize();
  void UpdateCursor();

  int              mMaxPos;
  int              mMinPos;
  int              mTracking;
  int              mDragging;
  int              mDragStartMouseX;
  int              mDragStartMouseY;
  NTempest::CiRect mDragStartPos;
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

  void SetBackgroundColor(const NTempest::CImVector &inColor);
  void ClearItems();
  void DeleteItem(void *inItem);
  void *InsertItem(void *inParent, void *inAfter, const char *inText);
  void SetItemText(void *inItem, const char *inText);
  void GetItemText(void *inItem, char *inBuf, int inBufSize);
  void SetItemParam(void *inItem, void *inParam);
  void *GetItemParam(void *inItem);
  void SetItemColor(void *inItem, const NTempest::CImVector &inColor);
  void ResetItemColor(void *inItem);
  NTempest::CImVector GetItemColor(void *inItem);
  void *GetItemParent(void *inItem);
  int GetItemNumChildren(void *inItem);
  void *GetItemChild(void *inItem, int inIndex);
  void SetItemImage(void *inItem, int inWidth, int inHeight, void *inData);
  void ExpandItem(void *inItem, int inVal);
  void CollapseItem(void *inItem) {
    ExpandItem(inItem, 0);
  }
  int IsItemExpanded(void *inItem);
  void OnExpandedItem(void *inItem);
  void EnsureItemVisible(void *inItem);
  void EditItem(void *inItem);
  NTempest::CiRect GetItemRect(void *inItem);
  void RefreshItem(void *inItem);
  void *GetFirstVisibleItem();
  void SetFirstVisibleItem(void *inItem);
  void EnumerateItems(
      void *inParent,
      void(*inFunc)(COsTreeView *, void *, void *),
      void *inParam
  );
  void EnumerateAllItems(
      void(*inFunc)(COsTreeView *, void *, void *),
      void *inParam
  );
  int          IsCharacterAllowed(char inChar);
  void         SelectItem(void *inItem, int inVal);
  int IsItemSelected(void *inItem);
  void *GetSelectedItem();
  void DeselectItem(void *inItem) {
    SelectItem(inItem, 0);
  }
  void SelectAll(int inVal);
  void DeselectAll() {
    SelectAll(0);
  }
  void GetSelectionInfo(OsGuiTVSelectionInfo *outInfo);
  void EnableDragDrop(int inVal);
  void DisableDragDrop() {
    EnableDragDrop(0);
  }
  void SetDragDropHandler(
      int(*inFunc)(const OsGuiTVDDInfo &, void *),
      void *inParam
  );
  void SetDropTarget(void *inItem);
  void SetInsertionMark(void *inItem, int inAfter);
  int OnClick() {
    return SendEvent(2, 0);
  }
  void OnBeginDrag(void *inItem, int inX, int inY);
  void OnEndDrag();
  int OnBeginEdit(void *inItem);
  int OnEndEdit(void *inItem, const char *inNewText);
  void SetTextLimit(int inSize);
  void EnableFilters(int inVal);
  void DisableFilters() {
    EnableFilters(0);
  }
  void SetFilter(unsigned int inFilter, int inVal);
  void SetCanEditFunction(int(*inFunc)(void *, void *), void *inParam);
  void SetExpandFunction(void(*inFunc)(void *, void *), void *inParam);

 protected:
  void InitializeTreeView();
  void *GetEditControl();
  void CreateDragImage(void *inItem);
  void DestroyDragImage();
  int RunDragHandler();
  void *FindItemUnderCursor();
  void OnDeleteItem(void *inItem);
  int FindUnusedParams();
  void InitParams(void *inItem);
  OsGuiTreeItemParams *GetParams(void *inItem);

  void                *mImages;
  TSGrowableArray<int> mUnusedImageIDs;
  int                  mDragging;
  OsGuiTVDDInfo        mDragInfo;
  void                *mDragImage;
  int(*mDragHandler)(const OsGuiTVDDInfo &, void *);
  void *mDragHandlerParam;
  int(*mCanEditFunc)(void *, void *);
  void *mCanEditParam;
  void(*mExpandFunc)(void *, void *);
  void                                *mExpandParam;
  int                                  mTextLimit;
  int                                  mFiltersEnabled;
  unsigned int                         mFilters;
  TSGrowableArray<OsGuiTreeItemParams> mItemParams;
};

class COsSpinButton : public COsControl {
 public:
  COsSpinButton(COsDialog *inDialog, short inID, unsigned int inFlags);
  COsSpinButton(void *inWindow, short inID, unsigned int inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();

  void OnSpinMouseUp();
  void SetValueRange(int inMinVal, int inMaxVal);

 protected:
  void Initialize();
};

class COsTabControl : public COsControl {
 public:
  COsTabControl(COsDialog *inDialog, short inID, unsigned int inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  void         InsertItem(const char *inText, int inPos);
  int          GetNumItems();
  int          OnControlTab();
};

class COsWindow {
 public:
  COsWindow(void *inWindow);
  virtual ~COsWindow();

  virtual void OnResize();

  void SetMinSize(int inW, int inH);
  void GetMinSize(int *outW, int *outH);
  void SetCursor(int inCursor);
  void SetIcon(const char *inName);
  void SetInputFocus();

 protected:
  void                *mHandle;
  NTempest::C2iVector mMinSize;
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

  void Set(TSGrowableArray<COsMenu *> &inMenus);
  void UpdateAccelerators();
  void Refresh();

 protected:
  TSGrowableArray<COsMenu *> mMenus;
  void                      *mWindowHandle;
  void                      *mMenuBarHandle;
  void                      *mAccelerators;
};

void OsGuiSetApplicationInfo(void *inData);
void OsGuiMenuSelect(int menuID, int itemID);
void OsGuiInitialize();
void OsGuiDestroy();
void OsGuiSetMenuCommandCallback(void(*inCallback)(const OsGuiCallbackParams &), void *inUser);
void OsGuiSetIdleCallback(void(*inCallback)(const OsGuiCallbackParams &), void *inUser);
void OsGuiEnableTooltips(int inVal);
void OsGuiEnableMenuHotkeys(int inVal);
struct HICON__;
HICON__ *sWinCursor(int inCursor);
void OsGuiSetCursor(int inCursor);
void OsGuiShowCursor(int inVal);
void OsGuiGetCursorPosition(int *outX, int *outY);
void OsGuiBeep();
void *OsGuiGetWindow(int inWindowType);
int OsGuiMessageBox(void *inParentWindow, int inStyle, const char *inMessage, const char *inTitle);
int OsGuiProcessMessage(void *inMsgData);
int OsGuiIsModifierKeyDown(int inKey);
void OsGuiSetGxWindow(void *window);
void OsGuiSetWindowTitle(void *inWindow, const char *inText);
void OsGuiSetWindowIcon(void *inWindow, const char *inName);
void OsGuiSetWindowRect(void *inWindow, const NTempest::CiRect &inRect);
void OsGuiBringWindowToFront(void *inWindow);
void OsGuiShowWindow(void *inWindow, int inVal);
void OsGuiEnableWindow(void *inWindow, int inVal);
int OsGuiWindowEnabled(void *inWindow);
void OsGuiMaximizeWindow(void *inWindow, int inVal);
int OsGuiWindowMaximized(void *inWindow);
void OsGuiMinimizeWindow(void *inWindow, int inVal);
int OsGuiWindowMinimized(void *inWindow);
NTempest::CiRect OsGuiGetWindowRect(void *inWindow, int inClientOnly);
NTempest::CiRect OsGuiGetWindowRestoredRect(void *inWindow);
void OsGuiSetWindowRestoredRect(void *inWindow, const NTempest::CiRect &inRect);
int OsGuiWindowIsCursorInside(void *inWindow, int inClientOnly);
NTempest::CiRect OsGuiGetScreenBounds();
NTempest::CImVector OsGuiGetColor(int inColor);
void OsGuiGetHotkeyText(const OsGuiMenuHotkey &inHotkey, char *inBuf, int inBufSize);
long OsGuiWindowProc(void *inWindow, unsigned int inMessage, unsigned int inWParam, long inLParam);
