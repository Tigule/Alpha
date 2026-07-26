#ifndef ENGINE_SOURCE_FRAMEXML_XMLTREE_H
#define ENGINE_SOURCE_FRAMEXML_XMLTREE_H

#include <stddef.h>

struct XMLTree;

void __cdecl begin_element(void *userData, const char *name, const char **attributes);
void __cdecl end_element(void *userData, const char *name);
void __cdecl handle_body(void *userData, const char *body, int length);

class XMLNode {
 public:
  struct XMLAttribute {
    char *name;
    char *value;
  };

  XMLNode(XMLNode *parent, const char *name);
  ~XMLNode();

  const char *GetName() const {
    return m_name;
  }

  const char *GetBody() const {
    return m_body;
  }
  int GetNumAttributes() const {
    return m_num_attributes;
  }
  const char *GetAttributeNameByIndex(int index) const;
  const char *GetAttributeValueByIndex(int index) const;
  const char *GetAttributeByName(const char *name) const;
  int         GetParentBodyOffset() const {
    return m_offset;
  }

  const XMLNode *GetChild() const {
    return m_child;
  }

  const XMLNode *GetChildByName(const char *name) const;

  const XMLNode *GetSibling() const {
    return m_next;
  }

 private:
  friend void __cdecl begin_element(void *userData, const char *name, const char **attributes);
  friend void __cdecl end_element(void *userData, const char *name);
  friend void __cdecl handle_body(void *userData, const char *body, int length);

  XMLNode      *m_parent;
  XMLNode      *m_child;
  char         *m_name;
  char         *m_body;
  int           m_num_attributes;
  XMLAttribute *m_attributes;
  int           m_offset;
  XMLNode      *m_next;
};

XMLTree *__fastcall       XMLTree_Load(const char *buffer, unsigned int bytes);
void __fastcall           XMLTree_Free(XMLTree *tree);
const XMLNode *__fastcall XMLTree_GetRoot(XMLTree *tree);

#endif
