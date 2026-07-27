#pragma once

#include <Base/Handle.h>
#include <Tempest/c2vector.h>
#include <Tempest/cimvector.h>
#include <stpl.h>

struct CGxString;
struct HWORLDTEXT__;

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
  WORLDTEXTCREATEPARAMS();

  void Defaults();
  void Clear();

  float               ascendDistance;
  unsigned int        totalTime;
  unsigned int        fadeInTime;
  unsigned int        fadeOutTime;
  NTempest::CImVector fontColor;
  NTempest::C2Vector  shadowOffset;
  NTempest::CImVector shadowColor;
  float               charSpacing;
  float               heightScale;
  float               zOffset;
  NTempest::C2Vector  border;
  float               startFontHeight;
  float               endFontHeight;
  unsigned int        enlargeTime;
  unsigned int        shrinkTime;
  unsigned int        flags;
  char                fontName[260];
  float               fontHeight;
};

struct WORLDTEXTSTRING : public CHandleObject {
  WORLDTEXTSTRING();
  void Update(float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *basePosition);
  void Reset();
  void CalculateNewColor(unsigned int elapsed);
  void CalculateTextHeight(unsigned int elapsedTime);
  void CalculateNewPosition(
      const NTempest::C4Vector &worldPosition,
      unsigned int         elapsedTime,
      NTempest::C4Vector  &textPos,
      const NTempest::C44Matrix &matrix,
      int                  worldPositionSpecified
  );
  void UpdatePosition(const NTempest::C4Vector &worldPosition, unsigned int elapsedTime, NTempest::C4Vector &textPos);
  void UpdateStringHeight(float height);
  void RecreateString();
  void Hide(int hide);
  void Render() const;
  void InitTextFrame(const char *text);
  virtual ~WORLDTEXTSTRING();

  WORLDTEXTTYPE           worldTextType;
  WORLDTEXTCREATEPARAMS   params;
  unsigned int            elapsedTime;
  unsigned int            totalTime;
  unsigned __int64        object;
  float                   textWidth;
  float                   textHeight;
  float                   heightScale;
  float                   zOffset;
  TSLink<WORLDTEXTSTRING> link;
  int                     hidden;
  unsigned int            m_flags;
  CGxString              *string;
  float                   savedStringHeight;
  char                    savedStringText[64];
};

void WorldTextClearStrings();
HWORLDTEXT__ *WorldTextCreate(WORLDTEXTTYPE type, const char *text, unsigned __int64 object, const NTempest::CImVector *colorOverride);
void WorldTextShow(HWORLDTEXT__ *text, int show);
void WorldTextRender(HWORLDTEXT__ *text);
void WorldTextUpdate(HWORLDTEXT__ *text, float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *position);
int WorldTextIsTextDone(HWORLDTEXT__ *text);
