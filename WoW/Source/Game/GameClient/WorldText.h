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
  ~WORLDTEXTCREATEPARAMS();

  void Defaults();
  void Clear();

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

struct WORLDTEXTSTRING : public CHandleObject {
  WORLDTEXTSTRING();
  void Update(float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *basePosition);
  void Reset();
  void CalculateNewColor(UINT elapsed);
  void CalculateTextHeight(UINT elapsedTime);
  void CalculateNewPosition(
      const NTempest::C4Vector  &worldPosition,
      UINT                       elapsedTime,
      NTempest::C4Vector        &textPos,
      const NTempest::C44Matrix &matrix,
      int                        worldPositionSpecified
  );
  void UpdatePosition(const NTempest::C4Vector &worldPosition, UINT elapsedTime, NTempest::C4Vector &textPos);
  void UpdateStringHeight(float height);
  void RecreateString();
  void Hide(int hide);
  void Render() const;
  void InitTextFrame(LPCSTR text);
  virtual ~WORLDTEXTSTRING();

  WORLDTEXTTYPE         worldTextType;
  WORLDTEXTCREATEPARAMS params;
  UINT                  elapsedTime;
  UINT                  totalTime;
  DWORDLONG             object;
  float                 textWidth;
  float                 textHeight;
  float                 heightScale;
  float                 zOffset;
  LINKDECLEX(WORLDTEXTSTRING, link);
  int        hidden;
  UINT       m_flags;
  CGxString *string;
  float      savedStringHeight;
  char       savedStringText[64];
};

void          WorldTextClearStrings();
HWORLDTEXT__ *WorldTextCreate(WORLDTEXTTYPE type, LPCSTR text, DWORDLONG object, const NTempest::CImVector *colorOverride);
void          WorldTextShow(HWORLDTEXT__ *text, int show);
void          WorldTextRender(HWORLDTEXT__ *text);
void          WorldTextUpdate(HWORLDTEXT__ *text, float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *position);
BOOL          WorldTextIsTextDone(HWORLDTEXT__ *text);
