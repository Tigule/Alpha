#include <Base/Base.h>

#include "Frame/CSimpleCheckbox.h"

#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <limits.h>

CSimpleCheckbox::CSimpleCheckbox(CSimpleFrame *parent) : CSimpleButton(parent), m_checked(0), m_checkedTexture(0), m_disabledTexture(0) {
}

CSimpleCheckbox::~CSimpleCheckbox() {
  SetCheckedTexture(static_cast<CSimpleTexture *>(0));
  SetDisabledCheckedTexture(static_cast<CSimpleTexture *>(0));
}

void CSimpleCheckbox::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleButton::LoadXML(node, status);

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "CheckedTexture", INT_MAX)) {
      SetCheckedTexture(LoadXML_Texture(child, this, status));
    } else if (!SStrCmpI(child->GetName(), "DisabledCheckedTexture", INT_MAX)) {
      SetDisabledCheckedTexture(LoadXML_Texture(child, this, status));
    }
  }
}

void CSimpleCheckbox::SetCheckedTexture(CSimpleTexture *texture) {
  if (m_checkedTexture) {
    DEL(m_checkedTexture);
  }

  if (texture) {
    texture->SetFrame(this, 3, 0);
  }

  m_checkedTexture = texture;
  SetChecked(m_checked, 1);
}

BOOL CSimpleCheckbox::SetCheckedTexture(LPCSTR texFile) {
  if (m_checkedTexture) {
    m_checkedTexture->SetTexture(texFile, 0);
    return 1;
  }

  CSimpleTexture *texture = NEW(CSimpleTexture)(0, 2, 1);
  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(GxBlend_Add);
    SetCheckedTexture(texture);
    return 1;
  }

  DEL(texture);
  return 0;
}

void CSimpleCheckbox::SetDisabledCheckedTexture(CSimpleTexture *texture) {
  if (m_disabledTexture) {
    DEL(m_disabledTexture);
  }

  if (texture) {
    texture->SetFrame(this, 3, 0);
  }

  m_disabledTexture = texture;
  SetChecked(m_checked, 1);
}

BOOL CSimpleCheckbox::SetDisabledCheckedTexture(LPCSTR texFile) {
  if (m_disabledTexture) {
    m_disabledTexture->SetTexture(texFile, 0);
    return 1;
  }

  CSimpleTexture *texture = NEW(CSimpleTexture)(0, 2, 1);
  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(GxBlend_Add);
    SetDisabledCheckedTexture(texture);
    return 1;
  }

  DEL(texture);
  return 0;
}

void CSimpleCheckbox::Enable(int enabled) {
  CSimpleButton::Enable(enabled);
  SetChecked(m_checked, 1);
}

void CSimpleCheckbox::SetChecked(int state, int force) {
  if (force || state != m_checked) {
    m_checked = state;

    if (m_checkedTexture) {
      m_checkedTexture->Hide();
    }
    if (m_disabledTexture) {
      m_disabledTexture->Hide();
    }

    if (m_checked) {
      if (m_disabledTexture && m_state == BUTTONSTATE_DISABLED) {
        m_disabledTexture->Show();
      } else if (m_checkedTexture) {
        m_checkedTexture->Show();
      }
    }
  }
}

void CSimpleCheckbox::OnClick(MOUSEBUTTON click) {
  SetChecked(!m_checked, 0);
  CSimpleButton::OnClick(click);
}
