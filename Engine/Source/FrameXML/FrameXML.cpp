#include <storm.h>
#include <FrameXML/FrameXML.h>

#include "Base/Status.h"
#include "Frame/CSimpleButton.h"
#include "Frame/CSimpleCheckbox.h"
#include "Frame/CSimpleEditBox.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleHTML.h"
#include "Frame/CSimpleMessageFrame.h"
#include "Frame/CSimpleMessageScrollFrame.h"
#include "Frame/CSimpleModel.h"
#include "Frame/CSimpleScrollFrame.h"
#include "Frame/CSimpleSlider.h"
#include "Frame/CSimpleStatusBar.h"
#include "Frame/SimpleFrameRegistry.h"
#include "FrameScript/FrameScript.h"
#include "FrameXML/XMLTree.h"
#include "Os/W32/Debugging.h"

#include <stdarg.h>
#include <stpl.h>

void __cdecl operator delete(void *, void *) {
}

class CFrameXMLStatus : public CStatus {
 public:
  CFrameXMLStatus();
  virtual ~CFrameXMLStatus();

  virtual void Add(int severity, const char *format, ...);
};

struct HashedNode : public TSHashObject<HashedNode, HASHKEY_STRI> {
  const XMLNode *node;
};

struct FrameFactoryNode : public TSHashObject<FrameFactoryNode, HASHKEY_STRI> {
  FRAMEFACTORY factory;
};

NODEDECL(TREENODE) {
  TREENODE();
  ~TREENODE();

  XMLTree *tree;
};

static TSHashTable<HashedNode, HASHKEY_STRI>       s_hashedNodes;
static TSHashTable<FrameFactoryNode, HASHKEY_STRI> s_frameFactories;
static CFrameXMLStatus                             s_defaultStatus;
static int                                         s_debugLevel;
static LISTDECL(TREENODE, s_treeList);
static int                                         s_loadNesting;
static FRAMELOADPROGRESSCALLBACK                   s_loadProgressCallback;

CFrameXMLStatus::CFrameXMLStatus() {
}

CFrameXMLStatus::~CFrameXMLStatus() {
}

void CFrameXMLStatus::Add(int, const char *format, ...) {
  char    buffer[0x200];
  va_list arguments;

  va_start(arguments, format);
  SStrVPrintf(buffer, sizeof(buffer), format, arguments);
  OsOutputDebugString(buffer);
}

TREENODE::TREENODE() {
}

TREENODE::~TREENODE() {
  XMLTree_Free(tree);
}

static int GuessNumFiles(const char *string);
static XMLTree *FrameXML_LoadXML(const char *filename, CStatus *status);
int FrameXML_ProcessFile(const char *filename, CStatus *status);
void FrameXML_StoreHashNode(const XMLNode *node, const char *name, CStatus *status);

void FrameXML_SetDebugLevel(int level) {
  s_debugLevel = level;
}

int FrameXML_GetDebugLevel() {
  return s_debugLevel;
}

int FrameXML_CreateFrames(const char *path, CStatus *status) {
  void       *buffer;
  const char *string;
  int         total;

  if (!status) {
    status = &s_defaultStatus;
  }

  if (FrameXML_GetDebugLevel() > 0) {
    status->Add(STATUS_WARNING, "** Loading table of contents %s", path);
  }

  if (!SFile::LoadFile(path, &buffer, 0, 1, 0)) {
    status->Add(STATUS_ERROR, "Couldn't open %s", path);
    return 0;
  }

  string = static_cast<const char *>(buffer);
  total = GuessNumFiles(string);

  char filename[0x104] = "";

  ++s_loadNesting;
  int index = 0;

  for (;;) {
    const char *suffix;

    SStrTokenize(&string, filename, sizeof(filename), " \r\n\"", 0);

    if (!*string) {
      break;
    }

    if (filename[0] && filename[0] != '#') {
      suffix = SStrChrR(filename, '.');
      if (suffix && !SStrCmpI(suffix, ".toc", 0x7FFFFFFF)) {
        FrameXML_CreateFrames(filename, status);
      } else {
        FrameXML_ProcessFile(filename, status);
      }
    }

    if (s_loadProgressCallback) {
      s_loadProgressCallback(index, total);
    }

    ++index;
    if (!filename[0]) {
      break;
    }

    if (!*string) {
      break;
    }
  }

  --s_loadNesting;
  SFile::Unload(buffer);

  if (!s_loadNesting) {
    s_hashedNodes.Clear();
    s_treeList.Clear();
  }

  return 1;
}

static int GuessNumFiles(const char *string) {
  const char *line;
  int         count;

  if (!string || !*string) {
    return 0;
  }

  count = 0;
  line = SStrChr(string, '\r');
  while (line) {
    ++count;
    if (!*line || !line[1]) {
      break;
    }

    line = SStrChr(line + 1, '\r');
  }

  return count;
}

int FrameXML_ProcessFile(const char *filename, CStatus *status) {
  XMLTree       *tree;
  const XMLNode *node;

  if (FrameXML_GetDebugLevel() > 0) {
    status->Add(STATUS_WARNING, "++ Loading file %s", filename);
  }

  tree = FrameXML_LoadXML(filename, status);
  if (!tree) {
    return 0;
  }

  s_treeList.NewNode(LIST_TAIL, 0, 0)->tree = tree;
  node = XMLTree_GetRoot(tree)->GetChild();

  while (node) {
    if (!SStrCmpI(node->GetName(), "Script", 0x7FFFFFFF)) {
      const char *scriptFile = node->GetAttributeByName("file");

      if (scriptFile) {
        char scriptPath[0x104];

        SStrCopy(scriptPath, scriptFile, sizeof(scriptPath));
        if (!SStrChrR(scriptFile, '\\')) {
          const char *slash = SStrChrR(filename, '\\');

          if (slash) {
            int directoryLength = slash - filename + 1;

            if (directoryLength < 0x104) {
              SStrCopy(scriptPath, filename, sizeof(scriptPath));
              scriptPath[directoryLength] = 0;
              SStrPack(scriptPath, scriptFile, sizeof(scriptPath));
            }
          }
        }

        if (!FrameScript_ExecuteFile(scriptPath)) {
          status->Add(STATUS_ERROR, "Couldn't open %s", scriptPath);
        }
      }

      {
        const char *body = node->GetBody();

        if (body && *body) {
          char description[0x110];

          SStrPrintf(description, 0x10F, "%s:<Scripts>", filename);
          FrameScript_Execute(body, description);
        }
      }
    } else {
      const char *isVirtual = node->GetAttributeByName("virtual");

      if (isVirtual && !SStrCmpI(isVirtual, "true", 0x7FFFFFFF)) {
        const char *name = node->GetAttributeByName("name");

        if (name && *name) {
          FrameXML_StoreHashNode(node, name, status);
        } else {
          status->Add(STATUS_WARNING, "Unnamed virtual node at top level");
        }
      } else {
        FrameXML_CreateFrame(node, 0, status);
      }
    }

    node = node->GetSibling();
  }

  return 1;
}

static XMLTree *FrameXML_LoadXML(const char *filename, CStatus *status) {
  void         *buffer;
  unsigned long bytes;
  XMLTree      *tree;

  if (!SFile::LoadFile(filename, &buffer, &bytes, 0, 0)) {
    status->Add(STATUS_ERROR, "Couldn't open %s", filename);
    return 0;
  }

  tree = XMLTree_Load(static_cast<const char *>(buffer), bytes);
  if (!tree) {
    status->Add(STATUS_ERROR, "Couldn't parse XML in %s", filename);
  }

  SFile::Unload(buffer);
  return tree;
}

CSimpleFrame *FrameXML_CreateFrame(const XMLNode *node, CSimpleFrame *parent, CStatus *status) {
  const char       *name;
  FrameFactoryNode *factoryNode;
  CSimpleFrame     *frame;

  if (FrameXML_GetDebugLevel() > 0) {
    name = node->GetAttributeByName("name");
    if (name && *name) {
      status->Add(STATUS_WARNING, "-- Creating %s named %s", node->GetName(), name);
    } else {
      status->Add(STATUS_WARNING, "-- Creating unnamed %s", node->GetName());
    }
  }

  factoryNode = s_frameFactories.Ptr(node->GetName());
  if (!factoryNode) {
    status->Add(STATUS_WARNING, "Unknown frame type: %s", node->GetName());
    return 0;
  }

  name = node->GetAttributeByName("parent");
  if (name && *name) {
    parent = SimpleFrameRegistryGetEntry(name, 0);
    if (!parent) {
      status->Add(STATUS_WARNING, "Couldn't find frame parent: %s", name);
    }
  }

  frame = factoryNode->factory(parent);
  if (!frame) {
    status->Add(STATUS_WARNING, "Unable to create frame type: %s", node->GetName());
    return 0;
  }

  frame->PreLoadXML(node, status);
  static_cast<CLayoutFrame *>(frame)->LoadXML(node, status);
  frame->PostLoadXML(node, status);
  CLayoutFrame::ResizePending();
  return frame;
}

void FrameXML_StoreHashNode(const XMLNode *node, const char *name, CStatus *status) {
  HashedNode *hashedNode;

  if (FrameXML_GetDebugLevel() > 0) {
    status->Add(STATUS_WARNING, "-- Added virtual frame %s", name);
  }

  if (s_hashedNodes.Ptr(name)) {
    status->Add(STATUS_WARNING, "Virtual object named %s already exists");
    return;
  }

  hashedNode = s_hashedNodes.New(name, 0, 0);
  hashedNode->node = node;
}

const XMLNode *FrameXML_FindHashNode(const char *name) {
  HashedNode *hashedNode = s_hashedNodes.Ptr(name);

  if (hashedNode) {
    return hashedNode->node;
  }

  return 0;
}

void FrameXML_ClearFactories() {
  s_frameFactories.Clear();
}

int FrameXML_RegisterFactory(const char *type, FRAMEFACTORY factory) {
  FrameFactoryNode *node;

  if (s_frameFactories.Ptr(type)) {
    ASSERT(!"Frame type is already registered");
    return 0;
  }

  node = s_frameFactories.New(type, 0, 0);
  node->factory = factory;
  return 1;
}

CSimpleFrame *Create_SimpleButton(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleButton))) CSimpleButton(parent);
}

CSimpleFrame *Create_SimpleCheckButton(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleCheckbox))) CSimpleCheckbox(parent);
}

CSimpleFrame *Create_SimpleEditBox(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleEditBox))) CSimpleEditBox(parent);
}

CSimpleFrame *Create_SimpleFrame(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleFrame))) CSimpleFrame(parent);
}

CSimpleFrame *Create_SimpleMessageFrame(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleMessageFrame))) CSimpleMessageFrame(parent);
}

CSimpleFrame *Create_SimpleModel(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleModel))) CSimpleModel(parent);
}

CSimpleFrame *Create_SimpleScrollFrame(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleScrollFrame))) CSimpleScrollFrame(parent);
}

CSimpleFrame *Create_SimpleScrollingMessageFrame(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleMessageScrollFrame))) CSimpleMessageScrollFrame(parent, 8);
}

CSimpleFrame *Create_SimpleSlider(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleSlider))) CSimpleSlider(parent);
}

CSimpleFrame *Create_SimpleHTML(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleHTML))) CSimpleHTML(parent);
}

CSimpleFrame *Create_SimpleStatusBar(CSimpleFrame *parent) {
  return new (ALLOC(sizeof(CSimpleStatusBar))) CSimpleStatusBar(parent);
}

int FrameXML_RegisterDefault() {
  FrameXML_RegisterFactory("Button", Create_SimpleButton);
  FrameXML_RegisterFactory("CheckButton", Create_SimpleCheckButton);
  FrameXML_RegisterFactory("EditBox", Create_SimpleEditBox);
  FrameXML_RegisterFactory("Frame", Create_SimpleFrame);
  FrameXML_RegisterFactory("MessageFrame", Create_SimpleMessageFrame);
  FrameXML_RegisterFactory("Model", Create_SimpleModel);
  FrameXML_RegisterFactory("ScrollFrame", Create_SimpleScrollFrame);
  FrameXML_RegisterFactory("ScrollingMessageFrame", Create_SimpleScrollingMessageFrame);
  FrameXML_RegisterFactory("Slider", Create_SimpleSlider);
  FrameXML_RegisterFactory("SimpleHTML", Create_SimpleHTML);
  FrameXML_RegisterFactory("StatusBar", Create_SimpleStatusBar);
  return 1;
}

void FrameXML_RegisterLoadProgressCallback(FRAMELOADPROGRESSCALLBACK callback) {
  s_loadProgressCallback = callback;
}
