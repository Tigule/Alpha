#pragma once

#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"

struct CGContainerData {
  unsigned int     m_numSlots;
  unsigned int     m_pad;
  unsigned __int64 m_slots[20];
};

class CGContainer {
 public:
  static unsigned int GetDataSize();
  static unsigned int GetBaseOffset();
  static __forceinline unsigned int TotalFields() {
    return 78;
  }
  static unsigned int GetUpdateMaskBytes();
  static unsigned int GetUpdateMaskBlocks();

  unsigned char *GetData(unsigned int index);
  void SetStorage(unsigned long *storage) {
    m_cont = reinterpret_cast<CGContainerData *>(storage);
  }

 protected:
  explicit CGContainer(unsigned long *storage) {
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
  CGContainer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init);
  ~CGContainer_C();

  void         SetStorage(unsigned long *storage);
  void PostMovementUpdate() {
  }
  virtual void                   Disable(int shutdown);
  virtual void                   Reenable();
  float                          GetCloseXOffset() const;
  float                          GetCloseYOffset() const;
  float                          GetSlotXOffset() const;
  float                          GetSlotYOffset() const;
  int                            GetWidth() const;
  int                            GetHeight() const;
  int                            SetBlock(unsigned int i, unsigned long data);
  void                           SetData(const void *data, unsigned int bytes);
  static unsigned int OffsetOf(OBJECT_TYPE_ID type);
  virtual CGBag_C               *GetBag() {
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
