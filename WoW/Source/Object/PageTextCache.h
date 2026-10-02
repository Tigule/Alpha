#ifndef WOW_SOURCE_OBJECT_PAGETEXTCACHE_H
#define WOW_SOURCE_OBJECT_PAGETEXTCACHE_H

#include <string.h>

class CDataStore;

class PageTextCache {
 public:
  PageTextCache() {
    m_text[0] = 0;
  }

  char m_text[0x1F4];
  int  m_nextPage;

  static int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
};

class PageTextCache_C : public PageTextCache {
 public:
  void Unpack(CDataStore *msg);
};

#endif
