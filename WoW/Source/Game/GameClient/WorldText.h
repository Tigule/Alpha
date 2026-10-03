#pragma once

#include <Base/Handle.h>
#include <Tempest/c2vector.h>
#include <Tempest/cimvector.h>
#include <stpl.h>

struct CGxString;
DECLARE_DERIVED_HANDLE(HWORLDTEXT, HOBJECT);

namespace NTempest {
  class C3Vector;
  class C4Vector;
  class C44Matrix;
}  // namespace NTempest

enum WORLDTEXTTYPE {
  WT_DAMAGE = 0,
  WT_ABSORB = 1,
  WT_CRIT = 2,
  WT_HEALSPELL = 3,
  WT_MISS = 4,
  WT_XPGAIN = 5,
  WT_QUEST = 6,
  NUM_WORLDTEXTTYPES = 7
};

struct WORLDTEXTCREATEPARAMS {
  void Defaults() {
    ascendDistance = 24.0f;
    totalTime = 3000;
    fadeInTime = 1000;
    fadeOutTime = 2000;
    fontColor = NTempest::CImVector(255, 255, 255, 255);
    shadowOffset = NTempest::C2Vector(0.0f, 0.0f);
    shadowColor = NTempest::CImVector(255, 0, 0, 0);
    charSpacing = 0.0f;
    heightScale = 1.0f;
    zOffset = 0.0f;
    border = NTempest::C2Vector(0.0f, 0.0f);
    startFontHeight = 0.0125f;
    endFontHeight = 0.025f;
    enlargeTime = 0;
    shrinkTime = 0;
    flags = 0;
  }
  void Clear();

  WORLDTEXTCREATEPARAMS();
  ~WORLDTEXTCREATEPARAMS() {
  }

  float               ascendDistance;
  UINT                totalTime;
  UINT                fadeInTime;
  UINT                fadeOutTime;
  NTempest::CImVector fontColor;
  NTempest::C2Vector  shadowOffset;
  NTempest::CImVector shadowColor;
  float               charSpacing;
  float               heightScale;
  float               zOffset;
  NTempest::C2Vector  border;
  float               startFontHeight;
  float               endFontHeight;
  UINT                enlargeTime;
  UINT                shrinkTime;
  UINT                flags;
  char                fontName[MAX_PATH];
  float               fontHeight;
};

void          WorldTextClearStrings();
HWORLDTEXT WorldTextCreate(WORLDTEXTTYPE type, LPCSTR text, DWORDLONG object, const NTempest::CImVector *colorOverride);
void          WorldTextShow(HWORLDTEXT text, int show);
void          WorldTextRender(HWORLDTEXT text);
void          WorldTextUpdate(HWORLDTEXT text, float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *position);
BOOL          WorldTextIsTextDone(HWORLDTEXT text);
