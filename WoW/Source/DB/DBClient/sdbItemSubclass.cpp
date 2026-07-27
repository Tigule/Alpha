#include "AutoCode/ItemSubClassRec.h"

#include <string.h>
#include <storm.h>
#include <stpl.h>

const unsigned int NUM_ITEMCLASSES = 16;

static TSFixedArray<const ItemSubClassRec *> s_itemSubClassList[NUM_ITEMCLASSES];

void SDBItemSubclassInitialize() {
  int counts[NUM_ITEMCLASSES];
  int i;

  memset(counts, 0, sizeof(counts));

  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);

    if (rec->m_classID >= 0) {
      ASSERT(rec->m_classID < NUM_ITEMCLASSES);

      if (counts[rec->m_classID] <= rec->m_subClassID) {
        counts[rec->m_classID] = rec->m_subClassID + 1;
      }
    }
  }

  for (i = 0; i < NUM_ITEMCLASSES; ++i) {
    s_itemSubClassList[i].SetCount(counts[i]);
  }

  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);

    if (rec->m_classID >= 0) {
      ASSERT(s_itemSubClassList[rec->m_classID].Count() > (unsigned int)rec->m_subClassID);
      s_itemSubClassList[rec->m_classID][rec->m_subClassID] = rec;
    }
  }
}

void SDBItemSubclassDestroy() {
}

const ItemSubClassRec *SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID) {
  if (classID >= NUM_ITEMCLASSES) {
    return 0;
  }

  if (subClassID >= s_itemSubClassList[classID].Count()) {
    return 0;
  }

  return s_itemSubClassList[classID][subClassID];
}
