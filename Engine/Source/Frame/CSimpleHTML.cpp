#include "Frame/CSimpleHTML.h"

#include "Base/Status.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"
#include "Services/TextBlock.h"

#include <storm.h>

static CStatus s_nullStatus;

CSimpleHTML::CSimpleHTML(CSimpleFrame *parent) : CSimpleHyperlinkedFrame(parent) {
}

CSimpleHTML::~CSimpleHTML() {
  ClearContent();
}

void CSimpleHTML::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  const XMLNode *child;
  for (child = node->GetChild(); child; child = child->GetSibling()) {
    CSimpleFontStringAttributes *attrib = 0;
    const char                  *name = child->GetName();

    if (!SStrCmpI(name, "FontString", 0x7FFFFFFF)) {
      attrib = &m_attrib[HTML_TEXT_NORMAL];
    } else if (!SStrCmpI(name, "FontStringHeader1", 0x7FFFFFFF)) {
      attrib = &m_attrib[HTML_TEXT_HEADER1];
    } else if (!SStrCmpI(name, "FontStringHeader2", 0x7FFFFFFF)) {
      attrib = &m_attrib[HTML_TEXT_HEADER2];
    } else if (!SStrCmpI(name, "FontStringHeader3", 0x7FFFFFFF)) {
      attrib = &m_attrib[HTML_TEXT_HEADER3];
    }

    if (attrib) {
      CSimpleFontString *string = LoadXML_String(child, this, status);
      *attrib = *string;
      DELIFUSED(string);
    }
  }
}

bool CSimpleHTML::SetText(const char *text, CStatus *status) {
  if (!status) {
    status = &s_nullStatus;
  }

  ClearContent();

  XMLTree *tree = XMLTree_Load(text, SStrLen(text));
  if (!tree) {
    AddText(text, m_attrib[HTML_TEXT_NORMAL]);
    return false;
  }

  const XMLNode *root = XMLTree_GetRoot(tree);
  if (SStrCmpI(root->GetName(), "HTML", 0x7FFFFFFF)) {
    status->Add(STATUS_WARNING, "Unknown element type: %s (expected HTML)", root->GetName());
    XMLTree_Free(tree);
    AddText(text, m_attrib[HTML_TEXT_NORMAL]);
    return false;
  }

  const XMLNode *child;
  for (child = root->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "BODY", 0x7FFFFFFF)) {
      ParseBODY(child, status);
    } else {
      status->Add(STATUS_WARNING, "Unknown element type: %s (expected BODY)", child->GetName());
    }
  }

  XMLTree_Free(tree);
  return true;
}

void CSimpleHTML::ClearContent() {
  REGIONNODE *node = m_content.Head();

  while (node) {
    DELIFUSED(node->region);
    node = m_content.DeleteNode(node);
  }

  m_layoutAnchor = 0;
  m_layoutOffset = 0.0f;

  CSimpleHyperlinkButton *button;
  while ((button = m_hyperlinks.Head()) != 0) {
    m_hyperlinks.UnlinkNode(button);
    ReleaseHyperlinkButton(button);
  }
}

void CSimpleHTML::ParseBODY(const XMLNode *node, CStatus *status) {
  const XMLNode *child;

  for (child = node->GetChild(); child; child = child->GetSibling()) {
    const char *name = child->GetName();

    if (!SStrCmpI(name, "H1", 0x7FFFFFFF)) {
      ParseP(child, HTML_TEXT_HEADER1, status);
    } else if (!SStrCmpI(name, "H2", 0x7FFFFFFF)) {
      ParseP(child, HTML_TEXT_HEADER2, status);
    } else if (!SStrCmpI(name, "H3", 0x7FFFFFFF)) {
      ParseP(child, HTML_TEXT_HEADER3, status);
    } else if (!SStrCmpI(name, "P", 0x7FFFFFFF)) {
      ParseP(child, HTML_TEXT_NORMAL, status);
    } else if (!SStrCmpI(name, "BR", 0x7FFFFFFF)) {
      AddText("\n", m_attrib[HTML_TEXT_NORMAL]);
    } else if (!SStrCmpI(name, "IMG", 0x7FFFFFFF)) {
      ParseIMG(child, status);
    } else {
      status->Add(STATUS_WARNING, "Unknown element type: %s", name);
    }
  }
}

void CSimpleHTML::ParseP(const XMLNode *node, HTML_TEXT_TYPE textType, CStatus *status) {
  const char                  *body;
  const char                  *link;
  CSimpleFontStringAttributes *attrib;
  int                          extralen;
  int                          offset;
  unsigned int                 flag;

  attrib = &m_attrib[textType];

  if (!static_cast<const char *>(attrib->m_font)) {
    attrib = &m_attrib[HTML_TEXT_NORMAL];
  }

  attrib->m_styleFlags = (attrib->m_styleFlags & ~0x7U) | 0x1;
  attrib->m_flags |= CSimpleFontStringAttributes::FLAG_STYLE_UPDATE;

  const char *value = node->GetAttributeByName("align");
  if (value && *value && StringToJustify(value, flag)) {
    attrib->m_styleFlags = (attrib->m_styleFlags & ~0x7U) | (flag & 0x7);
    attrib->m_flags |= CSimpleFontStringAttributes::FLAG_STYLE_UPDATE;
  }

  body = node->GetBody();
  char *text = SStrDupA(body ? body : "", __FILE__, __LINE__);
  offset = 0;
  const XMLNode *child;

  for (child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "BR", 0x7FFFFFFF)) {
      char        *newText = static_cast<char *>(ALLOC(SStrLen(text) + 2));
      unsigned int childOffset = child->GetParentBodyOffset() + offset;

      SStrCopy(newText, text, childOffset + 1);
      SStrPack(newText, "\n", 0x7FFFFFFF);
      if (text[childOffset]) {
        SStrPack(newText, text + childOffset + 1, 0x7FFFFFFF);
      }

      FREE(text);
      text = newText;
    } else if (!SStrCmpI(child->GetName(), "A", 0x7FFFFFFF)) {
      body = child->GetBody();
      link = child->GetAttributeByName("href");

      if (body && *body && link && *link) {
        extralen = SStrLen(link) + SStrLen(body) + 6;
        char        *newText = static_cast<char *>(ALLOC(SStrLen(text) + extralen + 1));
        unsigned int childOffset = child->GetParentBodyOffset() + offset;

        SStrCopy(newText, text, childOffset + 2);
        SStrPack(newText, "|H", 0x7FFFFFFF);
        SStrPack(newText, link, 0x7FFFFFFF);
        SStrPack(newText, "|h", 0x7FFFFFFF);
        SStrPack(newText, body, 0x7FFFFFFF);
        SStrPack(newText, "|h", 0x7FFFFFFF);
        if (text[childOffset]) {
          SStrPack(newText, text + childOffset + 1, 0x7FFFFFFF);
        }

        FREE(text);
        text = newText;
        offset += extralen;
      }
    } else {
      status->Add(STATUS_WARNING, "Unknown element type: %s", child->GetName());
    }
  }

  AddText(text, *attrib);
  FREE(text);
}

void CSimpleHTML::ParseIMG(const XMLNode *node, CStatus *status) {
  unsigned int align = 0x1;
  float        h = 0.0f;
  float        w = 0.0f;
  const char  *value = node->GetAttributeByName("align");
  if (value && *value) {
    StringToJustify(value, align);
  }

  value = node->GetAttributeByName("width");
  if (value && *value) {
    w = SStrToFloat(value) * 0.0009765625f * 0.8f;
  }

  value = node->GetAttributeByName("height");
  if (value && *value) {
    h = SStrToFloat(value) * 0.0009765625f * 0.8f;
  }

  CSimpleTexture *texture = NEW(CSimpleTexture)(this, 2, 1);
  texture->SetWidth(w);
  texture->SetHeight(h);

  switch (align) {
    case 0x1:
      texture->SetPoint(
          FRAMEPOINT_TOPLEFT, m_layoutAnchor ? m_layoutAnchor : this, m_layoutAnchor ? FRAMEPOINT_BOTTOMLEFT : FRAMEPOINT_TOPLEFT, 0.0f,
          m_layoutAnchor ? m_layoutOffset : 0.0f, 1
      );
      break;

    case 0x2:
      texture->SetPoint(
          FRAMEPOINT_TOP, m_layoutAnchor ? m_layoutAnchor : this, m_layoutAnchor ? FRAMEPOINT_BOTTOM : FRAMEPOINT_TOP, 0.0f,
          m_layoutAnchor ? m_layoutOffset : 0.0f, 1
      );
      break;

    case 0x4:
      texture->SetPoint(
          FRAMEPOINT_TOPRIGHT, m_layoutAnchor ? m_layoutAnchor : this, m_layoutAnchor ? FRAMEPOINT_BOTTOMRIGHT : FRAMEPOINT_TOPRIGHT, 0.0f,
          m_layoutAnchor ? m_layoutOffset : 0.0f, 1
      );
      break;
  }

  texture->SetTexture(node->GetAttributeByName("src"), 0);
  m_layoutOffset -= texture->GetHeight();

  REGIONNODE *regionNode = NEW(REGIONNODE);
  m_content.LinkNode(regionNode, LIST_TAIL, 0);
  regionNode->region = texture;
}

void CSimpleHTML::AddText(const char *text, CSimpleFontStringAttributes &attrib) {
  CSimpleFontString *string = NEW(CSimpleFontString)(this, 2, 1);

  if (m_layoutAnchor) {
    string->SetPoint(FRAMEPOINT_TOPLEFT, m_layoutAnchor, FRAMEPOINT_BOTTOMLEFT, 0.0f, m_layoutOffset, 1);
  } else {
    string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
  }

  string->SetWidth(GetWidth());
  attrib.UpdateString(string, 1);
  string->SetText(text);
  string->Resize(1);

  m_layoutAnchor = string;
  m_layoutOffset = 0.0f;

  REGIONNODE *regionNode = NEW(REGIONNODE);
  m_content.LinkNode(regionNode, LIST_TAIL, 0);
  regionNode->region = string;

  CGxString                  *gxString = string->m_string ? TextBlockGetStringPtr(string->m_string) : 0;
  const GXUFONTHYPERLINKINFO *links;
  unsigned int                linkCount = GxuFontStringHyperLinkInfo(gxString, links);

  for (unsigned int i = 0; i < linkCount; ++i) {
    CSimpleHyperlinkButton *button = CreateHyperlinkButton();
    m_hyperlinks.LinkNode(button, LIST_TAIL, 0);
    button->SetHyperlink(string, &links[i]);
  }
}
