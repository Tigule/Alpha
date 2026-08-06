#ifndef ENGINE_SOURCE_FRAME_CSIMPLEHTML_H
#define ENGINE_SOURCE_FRAME_CSIMPLEHTML_H

#include "Frame/CSimpleHyperlinkedFrame.h"
#include "Frame/CSimpleRender.h"

#include <stpl.h>

class CSimpleHyperlinkButton;
struct REGIONNODE;

enum HTML_TEXT_TYPE {
  HTML_TEXT_NORMAL = 0,
  HTML_TEXT_HEADER1 = 1,
  HTML_TEXT_HEADER2 = 2,
  HTML_TEXT_HEADER3 = 3,
  NUM_HTML_TEXT_TYPES = 4
};

class CSimpleHTML : public CSimpleHyperlinkedFrame {
  friend int CSimpleHTML_SetText(lua_State *L);
  friend int CSimpleHTML_SetTextColor(lua_State *L);

 public:
  CSimpleHTML(CSimpleFrame *parent = 0);
  virtual ~CSimpleHTML();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  void         SetTextAttributes(const CSimpleFontStringAttributes &attrib, HTML_TEXT_TYPE textType) {
    ASSERT(textType >= HTML_TEXT_NORMAL && textType < NUM_HTML_TEXT_TYPES);
    m_attrib[textType] = attrib;
  }
  const CSimpleFontStringAttributes &GetTextAttributes(HTML_TEXT_TYPE textType) {
    ASSERT(textType >= HTML_TEXT_NORMAL && textType < NUM_HTML_TEXT_TYPES);
    return m_attrib[textType];
  }
  bool SetText(LPCSTR text, CStatus *status);

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  void ClearContent();
  void ParseBODY(const XMLNode *node, CStatus *status);
  void ParseP(const XMLNode *node, HTML_TEXT_TYPE textType, CStatus *status);
  void ParseIMG(const XMLNode *node, CStatus *status);
  void AddText(LPCSTR text, CSimpleFontStringAttributes &attrib);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  LISTDECL(REGIONNODE, m_content);
  CLayoutFrame               *m_layoutAnchor;
  float                       m_layoutOffset;
  CSimpleFontStringAttributes m_attrib[4];
  LISTDECLEX(CSimpleHyperlinkButton, m_link, m_hyperlinks);
};

#endif
