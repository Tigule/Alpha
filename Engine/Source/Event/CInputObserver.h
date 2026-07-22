#ifndef ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H
#define ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H

#include "CObserver.h"

class CInputObserver : public CObserver {
 public:
  CInputObserver();
  CInputObserver(const CInputObserver &);
  virtual ~CInputObserver();

  CInputObserver &operator=(const CInputObserver &);
};

#endif
