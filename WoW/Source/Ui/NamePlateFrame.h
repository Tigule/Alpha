#ifndef WOW_SOURCE_UI_NAMEPLATEFRAME_H
#define WOW_SOURCE_UI_NAMEPLATEFRAME_H

#include <Frame/CSimpleButton.h>

class CGSimpleHealthBar;
class CGUnit_C;

class CGNamePlateFrame : public CSimpleButton {
 public:
  CGNamePlateFrame(CSimpleFrame *parent);
  virtual ~CGNamePlateFrame();

  void         Initialize(CGUnit_C *unit);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual void OnClick(MOUSEBUTTON button);

 private:
  unsigned __int64   m_unit;
  CSimpleTexture    *m_highlight;
  CSimpleFontString *m_nameFrame;
  CGSimpleHealthBar *m_healthBar;
};

#endif
