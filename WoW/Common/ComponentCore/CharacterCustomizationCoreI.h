#pragma once

struct FACIALGEOSETS {
  UINT beardGeoset;
  UINT sideBurnGeoset;
  UINT moustacheGeoset;

  FACIALGEOSETS() : beardGeoset(0), sideBurnGeoset(0), moustacheGeoset(0) {
  }
};

struct FACIALVARIATIONS {
  TSFixedArray<FACIALGEOSETS> facialGeosets;

  void AddVariation(const CharacterFacialHairStylesRec *rec) {
    FACIALGEOSETS *facialGeoset;

    ASSERT(rec->m_VariationID < (int)facialGeosets.Count());

    facialGeoset = &facialGeosets[rec->m_VariationID];
    facialGeoset->beardGeoset = rec->m_BeardGeoset + 1;
    facialGeoset->sideBurnGeoset = rec->m_SideburnGeoset + 1;
    facialGeoset->moustacheGeoset = rec->m_MoustacheGeoset + 1;
  }

  ~FACIALVARIATIONS() {
    facialGeosets.Clear();
  }
  UINT NumVariations();
};

struct CAMERAFILENAMES {
  char fileName[UNITSEX_LAST][MAX_PATH];

  CAMERAFILENAMES() {
    UINT sex;

    for (sex = 0; sex < UNITSEX_LAST; ++sex) {
      fileName[sex][0] = 0;
    }
  }
};

struct STRINGWANNABE {
 private:
  LPCSTR string;
 public:
  LPCSTR GetString() const {
    return string;
  }
  void SetString(LPCSTR prefix, LPCSTR value) {
    string = value;
  }
  STRINGWANNABE() : string(NULL) {
  }
};

struct CHARACTERVARIATIONS {
  ~CHARACTERVARIATIONS() { color.Clear(); }

  const STRINGWANNABE &GetColor(int colorID) const {
    return color[colorID % color.Count()];
  }

  STRINGWANNABE &GetColor(int colorID) {
    return color[colorID % color.Count()];
  }

  int GetColorCount() const {
    return color.Count();
  }

  void SetColorCount(int count) {
    color.SetCount(count);
  }

 private:
  TSGrowableArray<STRINGWANNABE> color;
};

struct CHARACTERSEXVARIATIONS {
 private:
  TSGrowableArray<CHARACTERVARIATIONS> names[CHARTEXTURESECTION_NUM];
 public:
  int firstNPCVar[CHARTEXTURESECTION_NUM];
  int lastNPCVar[CHARTEXTURESECTION_NUM];
  CHARACTERSEXVARIATIONS() {
    UINT section;

    for (section = 0; section < CHARTEXTURESECTION_NUM; ++section) {
      firstNPCVar[section] = INT_MAX;
      lastNPCVar[section] = -1;
    }
  }

  ~CHARACTERSEXVARIATIONS() {
    UINT section;
    for (section = 0; section < CHARTEXTURESECTION_NUM; ++section) {
      names[section].Clear();
    }
  }

  void GetNumVariations(CHARTEXTURESECTIONID section, int *pcVars, int *npcVars);

  const CHARACTERVARIATIONS &GetNames(int section, int variation) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section][variation % names[section].Count()];
  }

  CHARACTERVARIATIONS &GetNames(int section, int variation) {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section][variation % names[section].Count()];
  }

  int NumVariations(int section) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section].Count();
  }

  const TSGrowableArray<CHARACTERVARIATIONS> &GetSectionData(int section) const {
    ASSERT(section < (sizeof(names) / sizeof(names[0])));
    return names[section];
  }

  TSGrowableArray<CHARACTERVARIATIONS> &GetSectionData(int section) {
    ASSERT(section< (sizeof(names) / sizeof(names[0])));
    return names[section];
  }
};

struct CHARACTERRACEVARIATIONS {
  CHARACTERSEXVARIATIONS sex[UNITSEX_LAST];
};
