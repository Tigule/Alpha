#ifndef WOW_SOURCE_UIUTIL_HEALTHBAR_H
#define WOW_SOURCE_UIUTIL_HEALTHBAR_H

#include <Frame/CSimpleStatusBar.h>

class CGUnit_C;

class CGSimpleHealthBar : public CSimpleStatusBar {
 public:
  CGSimpleHealthBar(CSimpleFrame *parent);
  virtual ~CGSimpleHealthBar();

  void         SetUnit(CGUnit_C *unit);
  virtual void SetValue(float value);
  virtual void SetStatusBarColor(const NTempest::CImVector &color);
  void         InstallMirrorHandlers();
  void         RemoveMirrorHandlers();

 private:
  unsigned __int64 m_unitGUID;
  int              m_scaleColor;
};

#endif
