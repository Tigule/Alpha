#include "DBClient.h"

#include "AutoCode/WMOAreaTableRec.h"

#include <stdlib.h>

static int __cdecl bscompare(LPCVOID e1, LPCVOID e2);

LPCSTR SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID) {
  WMOAreaTableRec key;
  key.m_WMOID = wmoID;
  key.m_NameSetID = nameSetID;
  key.m_WMOGroupID = wmoGroupID;

  const WMOAreaTableRec *rec = (const WMOAreaTableRec *)bsearch(&key, g_wMOAreaTableDB.GetRecordByIndex(0), g_wMOAreaTableDB.GetNumRecords(), sizeof(WMOAreaTableRec), bscompare);

  if (rec) {
    return rec->m_AreaName_lang[CURRENT_LANGUAGE];
  }
  return "";
}

static int __cdecl bscompare(LPCVOID e1, LPCVOID e2) {
  return ((const WMOAreaTableRec *)e1)->m_WMOID - ((const WMOAreaTableRec *)e2)->m_WMOID
             ? ((const WMOAreaTableRec *)e1)->m_WMOID - ((const WMOAreaTableRec *)e2)->m_WMOID
         : ((const WMOAreaTableRec *)e1)->m_NameSetID - ((const WMOAreaTableRec *)e2)->m_NameSetID
             ? ((const WMOAreaTableRec *)e1)->m_NameSetID - ((const WMOAreaTableRec *)e2)->m_NameSetID
             : ((const WMOAreaTableRec *)e1)->m_WMOGroupID - ((const WMOAreaTableRec *)e2)->m_WMOGroupID;
}

bool SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID, const WMOAreaTableRec *&rec) {
  WMOAreaTableRec key;
  key.m_WMOID = wmoID;
  key.m_NameSetID = nameSetID;
  key.m_WMOGroupID = wmoGroupID;

  rec = (const WMOAreaTableRec *)bsearch(&key, g_wMOAreaTableDB.GetRecordByIndex(0), g_wMOAreaTableDB.GetNumRecords(), sizeof(WMOAreaTableRec), bscompare);

  return rec != 0;
}
