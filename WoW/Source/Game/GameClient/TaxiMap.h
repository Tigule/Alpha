#pragma once

#include <Tempest/c2vector.h>

struct HMODEL__;
struct HTEXTURE__;
typedef HMODEL__   *HMODEL;
typedef HTEXTURE__ *HTEXTURE;

namespace NTempest {
  class CRect;
}

struct TAXILINE {
  NTempest::C2Vector src;
  NTempest::C2Vector dst;

  ~TAXILINE();
};

enum TAXNODE_TYPE {
  TAXNODE_NONE = 0,
  TAXNODE_CURRENT = 1,
  TAXNODE_REACHABLE = 2,
  TAXNODE_DISTANT = 3
};

void TaxiMapInitialize();
void TaxiMapShutdown();
HTEXTURE TaxiMapGetTexture();
int TaxiMapUpdatePosition(int currentTaxiNode, __int64 reachable, __int64 known, NTempest::CRect &rect);
unsigned int TaxiNodeCost(unsigned int srcNode, unsigned int dstNode);
NTempest::CRect TaxiMapGetRect();
TAXNODE_TYPE TaxiNodeGetNodeType(int nodeID);
HMODEL TaxiGetRouteModel(float width, float height);
unsigned int TaxiRouteExists(int fromNode, int toNode);
