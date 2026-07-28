#ifndef WOW_SOURCE_UI_CONTAINERFRAME_H
#define WOW_SOURCE_UI_CONTAINERFRAME_H

#include "ObjectMgrClient/ObjectMgrClient.h"

class CGContainerInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void UpdateContainers();
  static void UpdateContents(unsigned __int64 guid);
  static void UpdateCooldowns();
  static unsigned __int64 GetContainer(int index) {
    if (!index) {
      return ClntObjMgrGetActivePlayer();
    }
    return index > 0 && index <= 10 ? m_containers[index - 1] : 0;
  }
  static void OpenContainer(unsigned __int64 container);
  static void UpdateItem(unsigned __int64 item);

 protected:
  static unsigned __int64 m_containers[10];
};

#endif
