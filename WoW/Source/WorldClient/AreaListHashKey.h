#pragma once

#include <stpl.h>

class AreaTableRec;

class AREAHASHKEY {
 public:
  AREAHASHKEY &operator=(const AREAHASHKEY &rhs) {
    if (this != &rhs) {
      cont = rhs.cont;
      area = rhs.area;
      subArea = rhs.subArea;
    }

    return *this;
  }

  int operator==(const AREAHASHKEY &rhs) const {
    return cont == rhs.cont && area == rhs.area && subArea == rhs.subArea;
  }

  unsigned int cont;
  unsigned int area;
  unsigned int subArea;
};

struct AREAHASHOBJECT : TSHashObject<AREAHASHOBJECT, AREAHASHKEY> {
  AREAHASHOBJECT *GetParent() const;

  const AreaTableRec *rec;
  int                 midi;
  int                 midiUnderwater;
  int                 zoneMusic;
  int                 reverb;
  int                 reverbUnderwater;
  int                 zoneIntroID;
  int                 zoneIntroIDPriority;
  unsigned int        continent;
  unsigned int        area;
  unsigned int        subArea;
};
