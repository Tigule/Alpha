#include "DBClient.h"

#include "AutoCode/WMOAreaTableRec.h"

#include <stdlib.h>

int bscompare(const void *e1, const void *e2) {
  return static_cast<const WMOAreaTableRec *>(e1)->m_WMOID != static_cast<const WMOAreaTableRec *>(e2)->m_WMOID
             ? static_cast<const WMOAreaTableRec *>(e1)->m_WMOID - static_cast<const WMOAreaTableRec *>(e2)->m_WMOID
         : static_cast<const WMOAreaTableRec *>(e1)->m_NameSetID != static_cast<const WMOAreaTableRec *>(e2)->m_NameSetID
             ? static_cast<const WMOAreaTableRec *>(e1)->m_NameSetID - static_cast<const WMOAreaTableRec *>(e2)->m_NameSetID
             : static_cast<const WMOAreaTableRec *>(e1)->m_WMOGroupID - static_cast<const WMOAreaTableRec *>(e2)->m_WMOGroupID;
}

const char *__fastcall SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID) {
  WMOAreaTableRec key;
  key.m_WMOID = wmoID;
  key.m_NameSetID = nameSetID;
  key.m_WMOGroupID = wmoGroupID;

  WMOAreaTableRec *rec = static_cast<WMOAreaTableRec *>(
      bsearch(&key, g_wMOAreaTableDB.GetRecordByIndex(0), g_wMOAreaTableDB.GetNumRecords(), sizeof(WMOAreaTableRec), bscompare)
  );

  return rec ? rec->m_AreaName_lang[CURRENT_LANGUAGE] : "";
}

bool __fastcall SDBWMOAreaTableLookup(int wmoID, int nameSetID, int wmoGroupID, const WMOAreaTableRec *&rec) {
  WMOAreaTableRec key;
  key.m_WMOID = wmoID;
  key.m_NameSetID = nameSetID;
  key.m_WMOGroupID = wmoGroupID;

  rec = static_cast<WMOAreaTableRec *>(
      bsearch(&key, g_wMOAreaTableDB.GetRecordByIndex(0), g_wMOAreaTableDB.GetNumRecords(), sizeof(WMOAreaTableRec), bscompare)
  );

  return rec != 0;
}
