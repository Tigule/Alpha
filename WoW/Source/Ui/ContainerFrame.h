#ifndef WOW_SOURCE_UI_CONTAINERFRAME_H
#define WOW_SOURCE_UI_CONTAINERFRAME_H

#include "ObjectMgrClient/ObjectMgrClient.h"

class CGContainerInfo {
 public:
  static void      EnterWorld();
  static void      LeaveWorld();
  static void      UpdateContainers();
  static void      UpdateContents(DWORDLONG guid);
  static void      UpdateCooldowns();
  static DWORDLONG GetContainer(int index) {
    if (!index) {
      return ClntObjMgrGetActivePlayer();
    }
    return index > 0 && index <= 10 ? m_containers[index - 1] : 0;
  }
  static void OpenContainer(DWORDLONG container);
  static void UpdateItem(DWORDLONG item);

 protected:
  static DWORDLONG m_containers[10];
};

#endif
