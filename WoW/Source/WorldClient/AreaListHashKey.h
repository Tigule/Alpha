#pragma once

#include <stpl.h>

class AreaTableRec;

class AREAHASHKEY {
 private:
  UINT cont;
  UINT area;
  UINT subArea;

 public:
  AREAHASHKEY() : cont(0), area(0), subArea(0) {
  }

  AREAHASHKEY(const AREAHASHKEY &rhs) : cont(rhs.cont), area(rhs.area), subArea(rhs.subArea) {
  }

  AREAHASHKEY(UINT continent, UINT areaID, UINT subAreaID) : cont(continent), area(areaID), subArea(subAreaID) {
  }

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

  UINT GetAreaID() const {
    return area << 16 | subArea;
  }
};

struct AREAHASHOBJECT : TSHashObject<AREAHASHOBJECT, AREAHASHKEY> {
  const AreaTableRec *rec;
  int                 midi;
  int                 midiUnderwater;
  int                 zoneMusic;
  int                 reverb;
  int                 reverbUnderwater;
  int                 zoneIntroID;
  int                 zoneIntroIDPriority;
  UINT                continent;
  UINT                area;
  UINT                subArea;
  AREAHASHOBJECT() {
  }
  AREAHASHOBJECT(const AREAHASHOBJECT &);

  AREAHASHOBJECT *GetParent() const;

};
