#pragma once

#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"

struct CGContainerData {
  UINT      m_numSlots;
  UINT      m_pad;
  DWORDLONG m_slots[20];
};

class CGContainer {
 public:
  static UINT               GetDataSize();
  static UINT               GetBaseOffset();
  static __forceinline UINT TotalFields() {
    return 78;
  }
  static UINT GetUpdateMaskBytes();
  static UINT GetUpdateMaskBlocks();

  BYTE *GetData(UINT index);
  void  SetStorage(DWORD *storage) {
    m_cont = reinterpret_cast<CGContainerData *>(storage);
  }

 protected:
  explicit CGContainer(DWORD *storage) {
    SetStorage(storage);
  }

  ~CGContainer() {
  }

  CGContainerData *Container() {
    return m_cont;
  }

  const CGContainerData *Container() const {
    return m_cont;
  }

  CGContainerData *m_cont;
};

class CGContainer_C : public CGItem_C, public CGContainer {
 public:
  CGContainer_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init);
  ~CGContainer_C();

  void SetStorage(DWORD *storage);
  void PostMovementUpdate() {
  }
  virtual void     Disable(int shutdown);
  virtual void     Reenable();
  float            GetCloseXOffset() const;
  float            GetCloseYOffset() const;
  float            GetSlotXOffset() const;
  float            GetSlotYOffset() const;
  int              GetWidth() const;
  int              GetHeight() const;
  BOOL             SetBlock(UINT i, DWORD data);
  void             SetData(LPCVOID data, UINT bytes);
  static UINT      OffsetOf(OBJECT_TYPE_ID type);
  virtual CGBag_C *GetBag() {
    return &m_bag;
  }
  CGBag_C *Bag() {
    return &m_bag;
  }

 protected:
  CGBag_C m_bag;

 private:
  CGContainer_C &operator=(const CGContainer_C &);
};
