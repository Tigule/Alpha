#ifndef WOW_SOURCE_UI_TAXIMAPFRAME_H
#define WOW_SOURCE_UI_TAXIMAPFRAME_H

namespace NTempest {
  class CRect;
}

#include <stpl.h>

struct TaxiNode {
  int   id;
  float offsetx;
  float offsety;
};

class CGTaxiMap {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void SetupMap(const DWORDLONG &unit, UINT node, LONGLONG destNodes, LONGLONG knownNodes, const NTempest::CRect &visibleArea);
  static void BuildTaxiNodeLines(LONGLONG nodes, const NTempest::CRect &visibleArea);
  static void CloseMap();
  static UINT NumTaxiNodes() {
    return m_nodes.Count();
  }
  static LPCSTR TaxiNodeName(UINT slot);
  static LPCSTR TaxiNodeType(UINT slot);
  static void   TaxiNodePosition(UINT slot, float &x, float &y);
  static UINT   TaxiNodeCost(UINT slot);
  static void   TakeTaxiNode(UINT slot);
  static void   RegisterScriptFunctions();
  static void   UnregisterScriptFunctions();

  static DWORDLONG GetTaxiVendor() {
    return m_unit;
  }

 protected:
  static DWORDLONG              m_unit;
  static UINT                   m_startNode;
  static TSCArray<TaxiNode, 64> m_nodes;
};

#endif
