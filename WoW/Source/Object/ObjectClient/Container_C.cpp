#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include "Container_C.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include <cstring>

void CGContainer_C::SetStorage(DWORD *storage) {
  CGItem_C::SetStorage(storage);
  CGContainer::SetStorage(storage + CGItem::TotalFields());
}

CGContainer_C::CGContainer_C(DWORD *storage, DWORD eventTime, CClientObjCreate *init)
    : CGItem_C(storage, eventTime, init), CGContainer(storage + CGItem::TotalFields()), m_bag(GetGUID(), &m_cont->m_numSlots, m_cont->m_slots, 0) {
}

CGContainer_C::~CGContainer_C() {
}

void CGContainer_C::Disable(int shutdown) {
  CGItem_C::Disable(shutdown);
}

void CGContainer_C::Reenable() {
  CGItem_C::Reenable();

  for (UINT i = 0; i < m_bag.NumSlots(); ++i) {
    DWORDLONG guid = m_bag.GetItem(i);
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
  return min(4U, m_bag.NumSlots());
}

int CGContainer_C::GetHeight() const {
  return (m_bag.NumSlots() + 3) / 4;
}

BOOL CGContainer_C::SetBlock(UINT, DWORD) {
  FATALASSERT(0);
  return 1;
}

void CGContainer_C::SetData(LPCVOID data, UINT bytes) {
  FATALASSERT(bytes <= sizeof(*m_cont));
  memcpy(m_cont, data, bytes);
}

UINT CGContainer_C::OffsetOf(OBJECT_TYPE_ID type) {
  switch (type) {
    case ID_OBJECT:
      return 0;
    case ID_ITEM:
      return CGObject::TotalFields() * sizeof(DWORD);
    case ID_CONTAINER:
      return CGItem::TotalFields() * sizeof(DWORD);
    default:
      FATALASSERT(0);
      return -1;
  }
}
