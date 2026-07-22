#ifndef WOW_SOURCE_OBJECT_PAGETEXTCACHE_H
#define WOW_SOURCE_OBJECT_PAGETEXTCACHE_H

#include <string.h>

class CDataStore;

class PageTextCache {
 public:
  PageTextCache() {
    m_text[0] = 0;
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);

  char m_text[0x1F4];
  int  m_nextPage;
};

class PageTextCache_C : public PageTextCache {
 public:
  PageTextCache_C() {
    memset(this, 0, sizeof(*this));
  }

  void Unpack(CDataStore *msg);
};

#endif
