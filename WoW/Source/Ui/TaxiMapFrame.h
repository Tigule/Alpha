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
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void
  SetupMap(const unsigned __int64 &unit, unsigned int node, __int64 destNodes, __int64 knownNodes, NTempest::CRect &visibleArea);
  static void CloseMap();
  static unsigned int NumTaxiNodes() {
    return m_nodes.Count();
  }
  static const char *TaxiNodeName(unsigned int slot);
  static const char *TaxiNodeType(unsigned int slot);
  static void TaxiNodePosition(unsigned int slot, float &x, float &y);
  static unsigned int TaxiNodeCost(unsigned int slot);
  static void TakeTaxiNode(unsigned int slot);
  static void RegisterScriptFunctions();
  static void UnregisterScriptFunctions();

  static unsigned __int64 GetTaxiVendor() {
    return m_unit;
  }

 protected:
  static unsigned __int64       m_unit;
  static unsigned int           m_startNode;
  static TSCArray<TaxiNode, 64> m_nodes;
};

#endif
