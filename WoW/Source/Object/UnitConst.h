#ifndef WOW_SOURCE_OBJECT_UNITCONST_H
#define WOW_SOURCE_OBJECT_UNITCONST_H

#include <storm.h>

class PetAction {
 public:
  PetAction(UINT action = 0) : m_action(action) {
  }

  void SetAction(UINT action) {
    m_action = action;
  }
  UINT GetAction() const {
    return m_action;
  }
  void SetActionTypeAndID(UINT type, UINT id) {
    ASSERT(id <= 0xFFFF);
    m_action = (m_action & 0xC0000000) | (type << 24) | id;
  }
  UINT GetActionTypeAndID() const {
    return m_action & 0x3FFFFFFF;
  }
  void SetActionType(UINT);
  int GetActionType() const {
    return (m_action >> 24) & 0x3F;
  }
  void SetActionID(UINT);
  int GetActionID() const {
    return m_action & 0xFFFF;
  }
  void SetAutocastAllowed(BYTE allowed) {
    if (allowed) {
      m_action |= 0x80000000;
    } else {
      m_action &= ~0x80000000;
    }
  }
  BYTE GetAutocastAllowed() const {
    return (m_action >> 31) & 1;
  }
  void SetAutocastEnabled(BYTE enabled) {
    if (enabled) {
      m_action |= 0x40000000;
    } else {
      m_action &= ~0x40000000;
    }
  }
  BYTE GetAutocastEnabled() const {
    return (m_action >> 30) & 1;
  }
  BYTE operator==(const PetAction &) const;

  operator UINT &() {
    return m_action;
  }

  operator const UINT &() const {
    return m_action;
  }

 private:
  UINT m_action;
};

#endif
