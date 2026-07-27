#include "Frame/CSimpleButton.h"
#include "Frame/CSimpleCheckbox.h"
#include "Frame/CSimpleEditBox.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleHTML.h"
#include "Frame/CSimpleMessageFrame.h"
#include "Frame/CSimpleMessageScrollFrame.h"
#include "Frame/CSimpleModel.h"
#include "Frame/CSimpleRender.h"
#include "Frame/CSimpleScrollFrame.h"
#include "Frame/CSimpleSlider.h"
#include "Frame/CSimpleStatusBar.h"

void RegisterSimpleFrameScriptMethods() {
  CSimpleTexture::RegisterScriptMethods();
  CSimpleFontString::RegisterScriptMethods();
  CSimpleFrame::RegisterScriptMethods();
  CSimpleButton::RegisterScriptMethods();
  CSimpleCheckbox::RegisterScriptMethods();
  CSimpleEditBox::RegisterScriptMethods();
  CSimpleHTML::RegisterScriptMethods();
  CSimpleMessageFrame::RegisterScriptMethods();
  CSimpleMessageScrollFrame::RegisterScriptMethods();
  CSimpleModel::RegisterScriptMethods();
  CSimpleScrollFrame::RegisterScriptMethods();
  CSimpleSlider::RegisterScriptMethods();
  CSimpleStatusBar::RegisterScriptMethods();
}

void UnregisterSimpleFrameScriptMethods() {
  CSimpleTexture::UnregisterScriptMethods();
  CSimpleFontString::UnregisterScriptMethods();
  CSimpleFrame::UnregisterScriptMethods();
  CSimpleButton::UnregisterScriptMethods();
  CSimpleCheckbox::UnregisterScriptMethods();
  CSimpleEditBox::UnregisterScriptMethods();
  CSimpleHTML::UnregisterScriptMethods();
  CSimpleMessageFrame::UnregisterScriptMethods();
  CSimpleMessageScrollFrame::UnregisterScriptMethods();
  CSimpleModel::UnregisterScriptMethods();
  CSimpleScrollFrame::UnregisterScriptMethods();
  CSimpleSlider::UnregisterScriptMethods();
  CSimpleStatusBar::UnregisterScriptMethods();
}
