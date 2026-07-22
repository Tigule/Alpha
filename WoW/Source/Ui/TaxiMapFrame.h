#ifndef WOW_SOURCE_UI_TAXIMAPFRAME_H
#define WOW_SOURCE_UI_TAXIMAPFRAME_H

namespace NTempest {
  class CRect;
}

#include <stpl.h>

struct TaxiNode {
  unsigned int id;
  float        x;
  float        y;
};

class CGTaxiMap {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall
  SetupMap(const unsigned __int64 &unit, unsigned int node, __int64 destNodes, __int64 knownNodes, NTempest::CRect &visibleArea);
  static void __fastcall         CloseMap();
  static unsigned int __fastcall NumTaxiNodes() {
    return m_nodes.Count();
  }
  static const char *__fastcall  TaxiNodeName(unsigned int slot);
  static const char *__fastcall  TaxiNodeType(unsigned int slot);
  static void __fastcall         TaxiNodePosition(unsigned int slot, float &x, float &y);
  static unsigned int __fastcall TaxiNodeCost(unsigned int slot);
  static void __fastcall         TakeTaxiNode(unsigned int slot);
  static void __fastcall         RegisterScriptFunctions();
  static void __fastcall         UnregisterScriptFunctions();

  static unsigned __int64 GetTaxiVendor() {
    return m_unit;
  }

 protected:
  static unsigned __int64       m_unit;
  static unsigned int           m_startNode;
  static TSCArray<TaxiNode, 64> m_nodes;
};

#endif
