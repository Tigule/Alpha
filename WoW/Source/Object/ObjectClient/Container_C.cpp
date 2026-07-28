#include "Container_C.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include <cstring>

void CGContainer_C::SetStorage(unsigned long *storage) {
  CGItem_C::SetStorage(storage);
  CGContainer::SetStorage(storage + CGItem::TotalFields());
}

CGContainer_C::CGContainer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGItem_C(storage, eventTime, init),
      CGContainer(storage + CGItem::TotalFields()),
      m_bag(GetGUID(), &m_cont->m_numSlots, m_cont->m_slots, 0) {
}

CGContainer_C::~CGContainer_C() {
}

void CGContainer_C::Disable(int shutdown) {
  CGItem_C::Disable(shutdown);
}

void CGContainer_C::Reenable() {
  CGItem_C::Reenable();

  for (unsigned int i = 0; i < m_bag.NumSlots(); ++i) {
    unsigned __int64 guid = m_bag.GetItem(i);
    if (guid) {
      ClntObjMgrObjectInRange(guid);
    }
  }
}

float CGContainer_C::GetCloseXOffset() const {
  return 0.0f;
}

float CGContainer_C::GetCloseYOffset() const {
  return 0.037f;
}

float CGContainer_C::GetSlotXOffset() const {
  return 0.025f;
}

float CGContainer_C::GetSlotYOffset() const {
  return -0.035f;
}

int CGContainer_C::GetWidth() const {
  return m_bag.GetWidth(0);
}

int CGContainer_C::GetHeight() const {
  return m_bag.GetHeight(0);
}

int CGContainer_C::SetBlock(unsigned int, unsigned long) {
  FATALASSERT(0);
  return 1;
}

void CGContainer_C::SetData(const void *data, unsigned int bytes) {
  FATALASSERT(bytes <= sizeof(*m_cont));
  memcpy(m_cont, data, bytes);
}

unsigned int CGContainer_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_ITEM:
      return CGObject::TotalFields() * sizeof(unsigned long);
    case ID_CONTAINER:
      return CGItem::TotalFields() * sizeof(unsigned long);
    default:
      FATALASSERT(0);
      return -1;
  }
}
