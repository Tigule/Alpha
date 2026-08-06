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
  int    type;
  int    subType;
  int    code;
  LPVOID user;
};

struct OsGuiMenuHotkey {
  int keyID;
  int modKeyID;
};

class COsMenu {
 public:
  COsMenu();
  COsMenu(BYTE inID, LPCSTR inTitle);
  ~COsMenu();

  void Clear();
  int  GetNumItems();
  void AddTextItem(int inPos, LPCSTR inText, OsGuiMenuHotkey *inHotkey);
  void AddSubMenu(int inPos, LPCSTR inTitle, COsMenu *inMenu);
  void AddSeparator(int inPos);
  int  GetHotkey(int inPos, OsGuiMenuHotkey *outHotkey);
  void EnableItem(int inPos, int inVal);
  void DisableItem(int inPos) {
    EnableItem(inPos, 0);
  }
  void CheckItem(int inPos, int inVal);
  void SetItemText(int inPos, LPCSTR inText);
  void RemoveItem(int inPos);
  BYTE GetID() {
    return mID;
  }
  LPVOID GetMenuHandle() {
    return mMenuHandle;
  }
  char *GetTitle() {
    return mTitle;
  }

 protected:
  void AddHotkey(int inPos);
  void RemoveHotkey(int inPos);

 public:
  static void AppendHotkeyText(char *inText, const OsGuiMenuHotkey &inHotkey);

 protected:
  BYTE                             mID;
  LPVOID                           mMenuHandle;
  char                             mTitle[32];
  TSGrowableArray<OsGuiMenuHotkey> mHotkeys;
};

struct OsGuiTVDDInfo {
  COsTreeView *treeView;
  int          action;
  LPVOID       dragItem;
  LPVOID       targItem;
  int          targX;
  int          targY;
};

struct OsGuiTreeItemParams {
  int                 used;
  NTempest::CImVector color;
  LPVOID              user;
};

struct OsGuiTVSelectionInfo {
  int    numSelected;
  LPVOID firstSelection;
  UINT   flags;
};

class COsControl {
  friend class COsDialog;

 public:
  COsControl(COsDialog *inDialog, int inType, short inID, UINT inFlags);
  COsControl(LPVOID inWindow, int inType, short inID, UINT inFlags);
  virtual ~COsControl();

  virtual void OnDestroy() {
  }

  virtual int OnEvent(int inItemID, int inNotifyCode, int inCode);

  virtual int OnDraw(LPVOID, UINT, NTempest::CiRect &) {
    return 0;
  }

  virtual LPVOID OnSetColors(LPVOID) {
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

  virtual int OnNotify(int inCode, LPVOID);
  virtual int OnCommand(int inParam);
  virtual int OnScroll(int inParam);

  virtual int OnMouseWheel(int) {
    return 0;
  }

  virtual int OnContextMenu(int inX, int inY);
  virtual int IsHandleFromControl(LPVOID inHandle);

  virtual int CanDoClipboardAction(int) {
    return 0;
  }

  virtual int DoClipboardAction(int) {
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

  LPVOID GetHandle() {
    return mHandle;
  }

  COsDialog *GetDialog() {
    return mDialog;
  }

  void SetRedraw(int inVal);
  void Refresh(int inErase);
  void SetCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam);
  void SetFont(int inFont);
  void SetInputFocus();
  void LoseInputFocus();
  int  HasInputFocus();
  void SetText(LPCSTR inText);
  void GetText(char *outText, int inBufSize);
  int  GetTextLength();
  void GetTextSize(LPCSTR inText, int *outW, int *outH);
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
  void SetTooltip(LPCSTR inText);
  void SetContextMenu(COsMenu *inMenu);
  void EnableContextMenu(int inVal) {
    mContextMenuEnabled = inVal;
  }
  void DisableContextMenu() {
    EnableContextMenu(0);
  }

 protected:
  void Initialize(LPVOID inWindow, int inType, short inID, UINT inFlags);
  int  SendEvent(int inEvent, int inCode);

  UINT       mFlags;
  COsDialog *mDialog;
  short      mID;
  int        mType;
  LPVOID     mHandle;
  void (*mCallback)(const OsGuiCallbackParams &);
  LPVOID   mCallbackParam;
  COsMenu *mContextMenu;
  int      mContextMenuEnabled;
  int      mRedrawLevel;
};

class COsDialog {
 public:
  COsDialog(LPVOID inWindowHandle, UINT inFlags);
  ~COsDialog();

  COsControl *FindControl(LPVOID inHandle);
  void        AddControl(COsControl *inControl);
  int         FindControl(COsControl *inControl);
  void        DeleteControl(COsControl *inControl);
  void        DetachControl(COsControl *inControl);
  int         ProcessMessage(LPVOID inMsgData);
  void        CheckEvents();
  LPVOID      GetHandle() {
    return mHandle;
  }
  LPVOID GetParentWindow();
  LPVOID GetTooltips();
  void   EnableTooltips(int inVal);
  void   DisableTooltips() {
    EnableTooltips(0);
  }
  void SetCancelButton(COsControl *inControl) {
    mCancelButton = inControl;
  }
  void SetTrackMouse(int inVal);
  int  IsMouseInside();
  void SetCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam);
  void BringToFront();
  int  IsInFront();
  void SetInputFocus();
  void Show(int inVal);
  void Hide() {
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
  void         SetTitle(LPCSTR inText);
  void         SetContextMenu(COsMenu *inMenu);
  void         EnableContextMenu(int inVal) {
    mContextMenuEnabled = inVal;
  }
  void DisableContextMenu() {
    EnableContextMenu(0);
  }
  int CanDoClipboardAction(int inAction);
  int DoClipboardAction(int inAction);
  int OnAccept();
  int OnCancel();
  int OnMouseUp();
  int OnMouseDown();
  int OnMouseLeave();
  int OnMouseMove(int inX, int inY);
  int OnContextMenu(int inX, int inY);
  int OnEvent(int inItemID, int inNotifyCode, int inCode);
  int OnControlTab();
  int HasFlag(UINT inFlag);

 protected:
  void ApplyModality(int inVal);

  LPVOID mHandle;
  LPVOID mTooltips;
  void (*mCallback)(const OsGuiCallbackParams &);
  LPVOID                        mCallbackParam;
  TSGrowableArray<COsControl *> mControls;
  COsControl                   *mCancelButton;
  int                           mTrackMouse;
  int                           mMouseInside;
  int                           mNeedNewTrack;
  int                           mTooltipsEnabled;
  int                           mContextMenuEnabled;
  COsMenu                      *mContextMenu;
  UINT                          mFlags;
  TSGrowableArray<LPVOID>       mDisabledWindows;
  NTempest::C2iVector           mMinSize;
};

class COsButton : public COsControl {
 public:
  COsButton(COsDialog *inDialog, short inID, UINT inFlags);

  void SetDefaultButton();
  void SetCancelButton();
  void SetHighlight(int inVal);
};

class COsImageButton : public COsControl {
 public:
  COsImageButton(COsDialog *inDialog, short inID, UINT inFlags);
  COsImageButton(LPVOID inWindow, short inID, UINT inFlags);
  virtual ~COsImageButton();

  virtual void OnDestroy();

  void SetImage(int inWidth, int inHeight, LPVOID inData);
  void SetHighlight(int inVal);
  int  IsPushed();
};

class COsTextButton : public COsControl {
 public:
  COsTextButton(COsDialog *inDialog, short inID);

  virtual int OnDraw(LPVOID inContext, UINT inState, NTempest::CiRect &inRect);

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
  COsStaticBox(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 12, inID, inFlags) {
  }
  COsStaticBox(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 12, inID, inFlags) {
  }
  virtual int OnDraw(LPVOID inContext, UINT inState, NTempest::CiRect &inRect);

  void ClearTransparentRects();
  void AddTransparentRect(const NTempest::CiRect &inRect);

 protected:
  TSGrowableArray<NTempest::CiRect> mTransRect;
};

class COsCheckbox : public COsControl {
 public:
  COsCheckbox(COsDialog *inDialog, short inID);
  COsCheckbox(LPVOID inWindow, short inID);

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
  COsStaticText(COsDialog *inDialog, short inID, UINT inFlags);
  COsStaticText(LPVOID inWindow, short inID, UINT inFlags);

  virtual LPVOID OnSetColors(LPVOID inContext);

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

  void SetImage(int inWidth, int inHeight, LPVOID inData);
  void ClearImage();
};

class COsEditBox : public COsControl {
 public:
  COsEditBox(COsDialog *inDialog, short inID, UINT inFlags);
  COsEditBox(LPVOID inWindow, short inID, UINT inFlags);

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
  void SetFilter(UINT inFilter, int inVal);
  int  IsCharacterAllowed(char inChar);
  void UpdateSelection();

 protected:
  void Initialize();

  int  mFiltersEnabled;
  UINT mFilters;
  int  mSelSize;
};

class COsListBox : public COsControl {
 public:
  COsListBox(COsDialog *inDialog, short inID, UINT inFlags);
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
  void InsertItem(LPCSTR inText, int inPos);
  void DeleteItem(int inPos);
  void SetItemText(int inPos, LPCSTR inText);
  int  GetItemTextLength(int inPos);
  void GetItemText(int inPos, char *inBuf, int inBufSize);
  void SetItemHeight(int inHeight);
  int  GetItemHeight();
};

class COsListView : public COsControl {
 public:
  COsListView(COsDialog *inDialog, short inID, UINT inFlags);
  virtual ~COsListView();

  virtual void OnSizeChange();
  virtual void SetValue(int inVal);
  virtual int  GetValue();
  virtual int  OnNotify(int inCode, LPVOID inParam);
  virtual int  OnReturn();

  void                InsertColumn(int inPos);
  void                DeleteColumn(int inPos);
  int                 GetNumColumns();
  void                InsertRow(int inPos);
  void                DeleteRow(int inPos);
  void                ClearRows();
  int                 GetNumRows();
  void                SetRowColor(int inPos, const NTempest::CImVector &inColor);
  NTempest::CImVector GetRowColor(int inPos);
  void                SetItemText(int inRow, int inCol, LPCSTR inText);
  void                GetItemText(int inRow, int inCol, char *inBuf, int inBufSize);
  void                SetColumnWidth(int inCol, int inWidth);
  int                 GetColumnWidth(int inCol);
  void                SetColumnTitle(int inCol, LPCSTR inText);
  void                GetColumnTitle(int inCol, char *inBuf, int inBufSize);
  void                SetColumnJustification(int inCol, int inJustify);
  void                EnsureRowVisible(int inRow);
  void                OnSelectionChange();
  void                OnColumnClick(int inCol);

 protected:
  int mNumCols;
};

class COsToolBar : public COsControl {
 public:
  COsToolBar(COsDialog *inDialog, short inID, UINT inFlags);
  COsToolBar(LPVOID inWindow, short inID, UINT inFlags);
  virtual ~COsToolBar();

  virtual int OnCommand(int inParam);

  void SetButtonSize(int inW, int inH);
  void GetButtonSize(int *outW, int *outH);
  void Clear();
  void AddButton(int inPos);
  void AddSeparator(int inPos);
  void RemoveButton(int inPos);
  int  GetNumButtons();
  void SetButtonImage(int inPos, int inWidth, int inHeight, LPVOID inData);
  void SetButtonText(int inPos, LPCSTR inText);
  void GetButtonText(int inPos, char *inBuf, int inBufSize);
  void EnableButton(int inPos, int inVal);
  void CheckButton(int inPos, int inVal);

 protected:
  void InitializeToolBar();

  LPVOID mImageList;
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
  void InsertItem(LPCSTR inText, int inPos);
  void SetItemHeight(int inHeight);
  int  GetItemHeight();
  void SetMaxHeight(int inHeight);
  void DeleteItem(int inPos);
  void SetItemText(int inPos, LPCSTR inText);

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
  COsRadioButton(COsDialog *inDialog, short inID, UINT inFlags);

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
  COsScrollBar(COsDialog *inDialog, short inID, UINT inFlags);
  COsScrollBar(LPVOID inWindow, short inID, UINT inFlags);

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
  COsDivider(COsDialog *inDialog, short inID, UINT inFlags);
  COsDivider(LPVOID inWindow, short inID, UINT inFlags);
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
  COsTreeView(COsDialog *inDialog, short inID, UINT inFlags);
  COsTreeView(LPVOID inWindow, short inID, UINT inFlags);
  virtual ~COsTreeView();

  virtual int  OnReturn();
  virtual int  OnEscape();
  virtual int  OnMouseDown();
  virtual int  OnMouseUp();
  virtual void OnMouseMove(int inX, int inY);
  virtual void OnSizeChange();
  virtual int  OnNotify(int inCode, LPVOID inParam);
  virtual int  IsHandleFromControl(LPVOID inHandle);

  void                SetBackgroundColor(const NTempest::CImVector &inColor);
  void                ClearItems();
  void                DeleteItem(LPVOID inItem);
  LPVOID              InsertItem(LPVOID inParent, LPVOID inAfter, LPCSTR inText);
  void                SetItemText(LPVOID inItem, LPCSTR inText);
  void                GetItemText(LPVOID inItem, char *inBuf, int inBufSize);
  void                SetItemParam(LPVOID inItem, LPVOID inParam);
  LPVOID              GetItemParam(LPVOID inItem);
  void                SetItemColor(LPVOID inItem, const NTempest::CImVector &inColor);
  void                ResetItemColor(LPVOID inItem);
  NTempest::CImVector GetItemColor(LPVOID inItem);
  LPVOID              GetItemParent(LPVOID inItem);
  int                 GetItemNumChildren(LPVOID inItem);
  LPVOID              GetItemChild(LPVOID inItem, int inIndex);
  void                SetItemImage(LPVOID inItem, int inWidth, int inHeight, LPVOID inData);
  void                ExpandItem(LPVOID inItem, int inVal);
  void                CollapseItem(LPVOID inItem) {
    ExpandItem(inItem, 0);
  }
  int              IsItemExpanded(LPVOID inItem);
  void             OnExpandedItem(LPVOID inItem);
  void             EnsureItemVisible(LPVOID inItem);
  void             EditItem(LPVOID inItem);
  NTempest::CiRect GetItemRect(LPVOID inItem);
  void             RefreshItem(LPVOID inItem);
  LPVOID           GetFirstVisibleItem();
  void             SetFirstVisibleItem(LPVOID inItem);
  void             EnumerateItems(LPVOID inParent, void (*inFunc)(COsTreeView *, LPVOID, LPVOID), LPVOID inParam);
  void             EnumerateAllItems(void (*inFunc)(COsTreeView *, LPVOID, LPVOID), LPVOID inParam);
  int              IsCharacterAllowed(char inChar);
  void             SelectItem(LPVOID inItem, int inVal);
  int              IsItemSelected(LPVOID inItem);
  LPVOID           GetSelectedItem();
  void             DeselectItem(LPVOID inItem) {
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
  void SetDragDropHandler(int (*inFunc)(const OsGuiTVDDInfo &, LPVOID), LPVOID inParam);
  void SetDropTarget(LPVOID inItem);
  void SetInsertionMark(LPVOID inItem, int inAfter);
  int  OnClick() {
    return SendEvent(2, 0);
  }
  void OnBeginDrag(LPVOID inItem, int inX, int inY);
  void OnEndDrag();
  int  OnBeginEdit(LPVOID inItem);
  int  OnEndEdit(LPVOID inItem, LPCSTR inNewText);
  void SetTextLimit(int inSize);
  void EnableFilters(int inVal);
  void DisableFilters() {
    EnableFilters(0);
  }
  void SetFilter(UINT inFilter, int inVal);
  void SetCanEditFunction(int (*inFunc)(LPVOID, LPVOID), LPVOID inParam);
  void SetExpandFunction(void (*inFunc)(LPVOID, LPVOID), LPVOID inParam);

 protected:
  void                 InitializeTreeView();
  LPVOID               GetEditControl();
  void                 CreateDragImage(LPVOID inItem);
  void                 DestroyDragImage();
  int                  RunDragHandler();
  LPVOID               FindItemUnderCursor();
  void                 OnDeleteItem(LPVOID inItem);
  int                  FindUnusedParams();
  void                 InitParams(LPVOID inItem);
  OsGuiTreeItemParams *GetParams(LPVOID inItem);

  LPVOID               mImages;
  TSGrowableArray<int> mUnusedImageIDs;
  int                  mDragging;
  OsGuiTVDDInfo        mDragInfo;
  LPVOID               mDragImage;
  int (*mDragHandler)(const OsGuiTVDDInfo &, LPVOID);
  LPVOID mDragHandlerParam;
  int (*mCanEditFunc)(LPVOID, LPVOID);
  LPVOID mCanEditParam;
  void (*mExpandFunc)(LPVOID, LPVOID);
  LPVOID                               mExpandParam;
  int                                  mTextLimit;
  int                                  mFiltersEnabled;
  UINT                                 mFilters;
  TSGrowableArray<OsGuiTreeItemParams> mItemParams;
};

class COsSpinButton : public COsControl {
 public:
  COsSpinButton(COsDialog *inDialog, short inID, UINT inFlags);
  COsSpinButton(LPVOID inWindow, short inID, UINT inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();

  void OnSpinMouseUp();
  void SetValueRange(int inMinVal, int inMaxVal);

 protected:
  void Initialize();
};

class COsTabControl : public COsControl {
 public:
  COsTabControl(COsDialog *inDialog, short inID, UINT inFlags);

  virtual void SetValue(int inVal);
  virtual int  GetValue();
  void         InsertItem(LPCSTR inText, int inPos);
  int          GetNumItems();
  int          OnControlTab();
};

class COsWindow {
 public:
  COsWindow(LPVOID inWindow);
  virtual ~COsWindow();

  virtual void OnResize();

  void SetMinSize(int inW, int inH);
  void GetMinSize(int *outW, int *outH);
  void SetCursor(int inCursor);
  void SetIcon(LPCSTR inName);
  void SetInputFocus();

 protected:
  LPVOID              mHandle;
  NTempest::C2iVector mMinSize;
};

class COsMenuBar {
 public:
  COsMenuBar(LPVOID inWindowHandle);
  ~COsMenuBar();

  LPVOID GetWindow() {
    return mWindowHandle;
  }

  LPVOID GetAccelerators() {
    return mAccelerators;
  }

  void Set(TSGrowableArray<COsMenu *> &inMenus);
  void UpdateAccelerators();
  void Refresh();

 protected:
  TSGrowableArray<COsMenu *> mMenus;
  LPVOID                     mWindowHandle;
  LPVOID                     mMenuBarHandle;
  LPVOID                     mAccelerators;
};

void OsGuiSetApplicationInfo(LPVOID inData);
void OsGuiMenuSelect(int menuID, int itemID);
void OsGuiInitialize();
void OsGuiDestroy();
void OsGuiSetMenuCommandCallback(void (*inCallback)(const OsGuiCallbackParams &), LPVOID inUser);
void OsGuiSetIdleCallback(void (*inCallback)(const OsGuiCallbackParams &), LPVOID inUser);
void OsGuiEnableTooltips(int inVal);
void OsGuiEnableMenuHotkeys(int inVal);
struct HICON__;
HICON__            *sWinCursor(int inCursor);
void                OsGuiSetCursor(int inCursor);
void                OsGuiShowCursor(int inVal);
void                OsGuiGetCursorPosition(int *outX, int *outY);
void                OsGuiBeep();
LPVOID              OsGuiGetWindow(int inWindowType);
int                 OsGuiMessageBox(LPVOID inParentWindow, int inStyle, LPCSTR inMessage, LPCSTR inTitle);
int                 OsGuiProcessMessage(LPVOID inMsgData);
int                 OsGuiIsModifierKeyDown(int inKey);
void                OsGuiSetGxWindow(LPVOID window);
void                OsGuiSetWindowTitle(LPVOID inWindow, LPCSTR inText);
void                OsGuiSetWindowIcon(LPVOID inWindow, LPCSTR inName);
void                OsGuiSetWindowRect(LPVOID inWindow, const NTempest::CiRect &inRect);
void                OsGuiBringWindowToFront(LPVOID inWindow);
void                OsGuiShowWindow(LPVOID inWindow, int inVal);
void                OsGuiEnableWindow(LPVOID inWindow, int inVal);
int                 OsGuiWindowEnabled(LPVOID inWindow);
void                OsGuiMaximizeWindow(LPVOID inWindow, int inVal);
int                 OsGuiWindowMaximized(LPVOID inWindow);
void                OsGuiMinimizeWindow(LPVOID inWindow, int inVal);
int                 OsGuiWindowMinimized(LPVOID inWindow);
NTempest::CiRect    OsGuiGetWindowRect(LPVOID inWindow, int inClientOnly);
NTempest::CiRect    OsGuiGetWindowRestoredRect(LPVOID inWindow);
void                OsGuiSetWindowRestoredRect(LPVOID inWindow, const NTempest::CiRect &inRect);
int                 OsGuiWindowIsCursorInside(LPVOID inWindow, int inClientOnly);
NTempest::CiRect    OsGuiGetScreenBounds();
NTempest::CImVector OsGuiGetColor(int inColor);
void                OsGuiGetHotkeyText(const OsGuiMenuHotkey &inHotkey, char *inBuf, int inBufSize);
long                OsGuiWindowProc(LPVOID inWindow, UINT inMessage, UINT inWParam, long inLParam);
