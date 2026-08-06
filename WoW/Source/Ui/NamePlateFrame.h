#ifndef WOW_SOURCE_UI_NAMEPLATEFRAME_H
#define WOW_SOURCE_UI_NAMEPLATEFRAME_H

#include <Frame/CSimpleButton.h>

class CGSimpleHealthBar;
class CGUnit_C;

class CGNamePlateFrame : public CSimpleButton {
 public:
  CGNamePlateFrame(CSimpleFrame *parent);

  void         Initialize(CGUnit_C *unit);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual void OnClick(MOUSEBUTTON button);

 private:
  DWORDLONG          m_unit;
  CSimpleTexture    *m_highlight;
  CSimpleFontString *m_nameFrame;
  CGSimpleHealthBar *m_healthBar;
};

#endif
