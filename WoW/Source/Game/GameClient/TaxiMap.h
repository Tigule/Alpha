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

void __fastcall            TaxiMapInitialize();
void __fastcall            TaxiMapShutdown();
HTEXTURE __fastcall        TaxiMapGetTexture();
int __fastcall             TaxiMapUpdatePosition(int currentTaxiNode, __int64 reachable, __int64 known, NTempest::CRect &rect);
unsigned int __fastcall    TaxiNodeCost(unsigned int srcNode, unsigned int dstNode);
NTempest::CRect __fastcall TaxiMapGetRect();
TAXNODE_TYPE __fastcall    TaxiNodeGetNodeType(int nodeID);
HMODEL __fastcall          TaxiGetRouteModel(float width, float height);
unsigned int __fastcall    TaxiRouteExists(int fromNode, int toNode);
