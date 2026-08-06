#ifndef ENGINE_SOURCE_FRAMEXML_XMLTREE_H
#define ENGINE_SOURCE_FRAMEXML_XMLTREE_H

#include <stddef.h>

struct XMLTree;

void __cdecl begin_element(LPVOID userData, LPCSTR name, LPCSTR *attributes);
void __cdecl end_element(LPVOID userData, LPCSTR name);
void __cdecl handle_body(LPVOID userData, LPCSTR body, int length);

class XMLNode {
 public:
  struct XMLAttribute {
    char *name;
    char *value;
  };

  XMLNode(XMLNode *parent, LPCSTR name);
  ~XMLNode();

  LPCSTR GetName() const {
    return m_name;
  }

  LPCSTR GetBody() const {
    return m_body;
  }
  int GetNumAttributes() const {
    return m_num_attributes;
  }
  LPCSTR GetAttributeNameByIndex(int index) const;
  LPCSTR GetAttributeValueByIndex(int index) const;
  LPCSTR GetAttributeByName(LPCSTR name) const;
  int    GetParentBodyOffset() const {
    return m_offset;
  }

  const XMLNode *GetChild() const {
    return m_child;
  }

  const XMLNode *GetChildByName(LPCSTR name) const;

  const XMLNode *GetSibling() const {
    return m_next;
  }

 private:
  friend void __cdecl begin_element(LPVOID userData, LPCSTR name, LPCSTR *attributes);
  friend void __cdecl end_element(LPVOID userData, LPCSTR name);
  friend void __cdecl handle_body(LPVOID userData, LPCSTR body, int length);

  XMLNode      *m_parent;
  XMLNode      *m_child;
  char         *m_name;
  char         *m_body;
  int           m_num_attributes;
  XMLAttribute *m_attributes;
  int           m_offset;
  XMLNode      *m_next;
};

XMLTree       *XMLTree_Load(LPCSTR buffer, UINT bytes);
void           XMLTree_Free(XMLTree *tree);
const XMLNode *XMLTree_GetRoot(XMLTree *tree);

#endif
