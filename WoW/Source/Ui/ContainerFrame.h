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
  static DWORDLONG GetContainer(UINT index) {
    if (index < 10) {
      return m_containers[index];
    }
    return 0;
  }
  static void OpenContainer(DWORDLONG container);
  static void UpdateItem(DWORDLONG item);

 protected:
  static DWORDLONG m_containers[10];
};

#endif
