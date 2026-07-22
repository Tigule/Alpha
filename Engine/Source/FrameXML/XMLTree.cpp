#define STORM_SAPIBASE_DECLARATION
#include <storm.h>

#include "XMLTree.h"

#include "Expat/lib/expat.h"

#include <ctype.h>
#include <new>

struct XMLTree {
  XMLNode *root;
  XMLNode *current;
};

void __cdecl begin_element(void *userData, const char *name, const char **attributes) {
  XMLTree     *tree = static_cast<XMLTree *>(userData);
  XMLNode     *node = new (ALLOC(sizeof(XMLNode))) XMLNode(tree->current, name);
  XMLNode     *child;
  const char **attribute;
  int          index;

  if (tree->current) {
    child = tree->current->m_child;
    if (child) {
      while (child->m_next) {
        child = child->m_next;
      }
      child->m_next = node;
    } else {
      tree->current->m_child = node;
    }
  } else {
    tree->root = node;
  }

  tree->current = node;

  attribute = attributes;
  while (*attribute) {
    ++node->m_num_attributes;
    attribute += 2;
  }

  node->m_attributes = static_cast<XMLNode::XMLAttribute *>(ALLOC(sizeof(XMLNode::XMLAttribute) * node->m_num_attributes));

  for (index = 0; index < node->m_num_attributes; ++index) {
    node->m_attributes[index].name = SStrDupA(attributes[index * 2], __FILE__, __LINE__);
    node->m_attributes[index].value = SStrDupA(attributes[index * 2 + 1], __FILE__, __LINE__);
  }
}

void __cdecl end_element(void *userData, const char *) {
  XMLTree *tree = static_cast<XMLTree *>(userData);
  tree->current = tree->current->m_parent;
}

void __cdecl handle_body(void *userData, const char *body, int length) {
  XMLTree *tree = static_cast<XMLTree *>(userData);
  XMLNode *node = tree->current;
  int      bodyLength;
  char    *output;

  while (length > 0 && isspace(*body)) {
    ++body;
    --length;
  }

  while (length > 0 && isspace(body[length - 1])) {
    --length;
  }

  if (!length) {
    return;
  }

  bodyLength = node->m_body ? SStrLen(node->m_body) : 0;
  node->m_body = static_cast<char *>(SMemReAlloc(node->m_body, bodyLength + length + 2, __FILE__, __LINE__, 0));
  output = node->m_body + bodyLength;

  if (bodyLength > 0) {
    *output++ = ' ';
  }

  do {
    *output++ = *body++;
  } while (--length);

  *output = 0;
}

XMLTree *__fastcall XMLTree_Load(const char *buffer, unsigned int bytes) {
  XML_Parser parser = XML_ParserCreate(0);
  XMLTree   *tree;

  if (!parser) {
    return 0;
  }

  tree = static_cast<XMLTree *>(ALLOC(sizeof(XMLTree)));
  tree->root = 0;
  tree->current = 0;

  XML_SetElementHandler(parser, begin_element, end_element);
  XML_SetCharacterDataHandler(parser, handle_body);
  XML_SetUserData(parser, tree);

  if (!XML_Parse(parser, buffer, bytes, 1)) {
    XMLTree_Free(tree);
    return 0;
  }

  return tree;
}

void __fastcall XMLTree_Free(XMLTree *tree) {
  if (tree->root) {
    DEL(tree->root);
  }

  FREE(tree);
}

const XMLNode *__fastcall XMLTree_GetRoot(XMLTree *tree) {
  return tree->root;
}

XMLNode::XMLNode(XMLNode *parent, const char *name) {
  m_parent = parent;
  m_child = 0;
  m_name = SStrDupA(name, __FILE__, __LINE__);
  m_body = 0;
  m_num_attributes = 0;
  m_attributes = 0;
  m_offset = 0;
  m_next = 0;

  if (m_parent && m_parent->m_body) {
    m_offset = SStrLen(m_parent->m_body);
  }
}

XMLNode::~XMLNode() {
  int index;

  if (m_next) {
    DEL(m_next);
  }

  if (m_child) {
    DEL(m_child);
  }

  FREE(m_name);

  if (m_body) {
    FREE(m_body);
  }

  for (index = 0; index < m_num_attributes; ++index) {
    FREE(m_attributes[index].name);
    FREE(m_attributes[index].value);
  }
}

const XMLNode *XMLNode::GetChildByName(const char *name) const {
  const XMLNode *node = GetChild();

  while (node) {
    if (!SStrCmpI(node->GetName(), name, 0x7FFFFFFF)) {
      return node;
    }

    node = node->GetSibling();
  }

  return 0;
}

const char *XMLNode::GetAttributeNameByIndex(int index) const {
  if (index < 0 || index >= m_num_attributes) {
    return 0;
  }

  return m_attributes[index].name;
}

const char *XMLNode::GetAttributeValueByIndex(int index) const {
  if (index < 0 || index >= m_num_attributes) {
    return 0;
  }

  return m_attributes[index].value;
}

const char *XMLNode::GetAttributeByName(const char *name) const {
  int index;

  for (index = 0; index < m_num_attributes; ++index) {
    if (!SStrCmpI(m_attributes[index].name, name, 0x7FFFFFFF)) {
      return m_attributes[index].value;
    }
  }

  return 0;
}
