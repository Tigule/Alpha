#include "Container_C.h"

#include "ObjectMgrClient/ObjectMgrClient.h"

void CGContainer_C::SetStorage(unsigned long *storage) {
  CGItem_C::SetStorage(storage);
  m_cont = reinterpret_cast<CGContainerData *>(storage + 36);
}

CGContainer_C::CGContainer_C(unsigned long *storage, unsigned long eventTime, CClientObjCreate *init)
    : CGItem_C(storage, eventTime, init), CGContainer(storage), m_bag(GetGUID(), &m_cont->m_numSlots, m_cont->m_slots, 0) {
}

CGContainer_C::~CGContainer_C() {
}

void CGContainer_C::Disable(int shutdown) {
  CGItem_C::Disable(shutdown);

  for (unsigned int i = 0; i < m_bag.NumSlots(); ++i) {
    unsigned __int64 guid = m_bag.GetItem(i);
    if (guid) {
      ClntObjMgrObjectInRange(guid);
    }
  }
}

void CGContainer_C::Reenable() {
  CGItem_C::Reenable();

  for (unsigned int i = 0; i < m_bag.NumSlots(); ++i) {
    unsigned __int64 guid = m_bag.GetItem(i);
    if (guid) {
      ClntObjMgrHideObject(guid);
    }
  }
}
