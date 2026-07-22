#include "DB/DBClient/DBCacheInstances.h"

#include <Base/CDataStore.h>
#include <Os/W32/OsFile.h>

#include "WowSvcs/WowSvcsClient/ClientServices.h"

template <class RECORD, class KEY, class HASHKEY>
DBCache<RECORD, KEY, HASHKEY>::DBCache(
    unsigned long fileTag,
    const char   *fileName,
    NETMESSAGE    singleQuery,
    NETMESSAGE    multiQuery,
    bool          requireGuids,
    bool          persistent
)
    : m_fileTag(fileTag),
      m_fileName(fileName),
      m_singleQueryMsg(singleQuery),
      m_multiQueryMsg(multiQuery),
      m_requireGuids(requireGuids),
      m_persistent(persistent) {
}

template <class RECORD, class KEY, class HASHKEY>
DBCache<RECORD, KEY, HASHKEY>::~DBCache() {
  Save();
  Clear();
}

template <class RECORD, class KEY, class HASHKEY>
const RECORD *DBCache<RECORD, KEY, HASHKEY>::GetRecord(KEY id, const unsigned __int64 &guid, DBCACHECALLBACKPROC cb, void *cbArg) {
  DBCACHEHASH     *entry;
  DBCACHECALLBACK *callbackEntry;

  if (!id) {
    return 0;
  }

  entry = m_table.Ptr(static_cast<unsigned int>(id), HASHKEY(id));
  if (entry) {
    if (entry->m_haveData) {
      return &entry->m_record;
    }

    if (cb) {
      callbackEntry = entry->m_callbacks.NewNode(LIST_TAIL, 0, 0);
      callbackEntry->m_callback = cb;
      callbackEntry->m_guid = guid;
      callbackEntry->m_cbArg = cbArg;
    }

    return 0;
  }

  if (!cb) {
    return 0;
  }

  entry = m_table.New(static_cast<unsigned int>(id), HASHKEY(id), 0, 0);
  callbackEntry = entry->m_callbacks.NewNode(LIST_TAIL, 0, 0);
  callbackEntry->m_callback = cb;
  callbackEntry->m_guid = guid;
  callbackEntry->m_cbArg = cbArg;

  {
    CDataStore queryMsg;

    queryMsg.Put(m_singleQueryMsg);
    queryMsg.Put(id);
    if (m_requireGuids) {
      queryMsg.Put(guid);
    }
    queryMsg.Finalize();
    ClientServices_Send(&queryMsg);
  }
  return 0;
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::VerifyPack(CGContainer_C *container, DBCACHECALLBACKPROC callback, void *arg) {
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::VerifyCache(CGPlayer_C *player, DBCACHECALLBACKPROC callback, void *arg) {
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::DenyItem(KEY key) {
  DBCACHEHASH     *entry;
  DBCACHECALLBACK *callbackEntry;

  entry = m_table.Ptr(static_cast<unsigned int>(key), HASHKEY(key));
  if (!entry) {
    return;
  }

  callbackEntry = entry->m_callbacks.Head();
  while (callbackEntry) {
    callbackEntry->m_callback(key, callbackEntry->m_guid, callbackEntry->m_cbArg, false);
    callbackEntry = callbackEntry->Next();
  }

  m_table.Delete(entry);
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::AddItem(RECORD *item, KEY key) {
  DBCACHEHASH     *obj;
  DBCACHECALLBACK *callbackEntry;

  obj = m_table.Ptr(static_cast<unsigned int>(key), HASHKEY(key));
  if (!obj) {
    obj = m_table.New(static_cast<unsigned int>(key), HASHKEY(key), 0, 0);
  }

  obj->m_record = *item;
  obj->m_haveData = true;
  obj->m_dbkey = key;

  callbackEntry = obj->m_callbacks.Head();
  while (callbackEntry) {
    callbackEntry->m_callback(key, callbackEntry->m_guid, callbackEntry->m_cbArg, true);
    callbackEntry = callbackEntry->Next();
  }

  obj->m_callbacks.Clear();
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::AddItems(CDataStore *msg, bool single) {
  KEY              id;
  unsigned int     invalid;
  unsigned int     count;
  DBCACHEHASH     *entry;
  DBCACHECALLBACK *callbackEntry;

  if (single) {
    count = 1;
  } else {
    msg->Get(count);
  }

  if (!count) {
    return;
  }

  while (count--) {
    msg->Get(id);
    invalid = (id & ~0x7FFFFFFF) != 0;
    id &= 0x7FFFFFFF;

    entry = m_table.Ptr(static_cast<unsigned int>(id), HASHKEY(id));
    if (invalid) {
      if (entry) {
        callbackEntry = entry->m_callbacks.Head();
        while (callbackEntry) {
          callbackEntry->m_callback(id, callbackEntry->m_guid, callbackEntry->m_cbArg, false);
          callbackEntry = callbackEntry->Next();
        }

        m_table.Delete(entry);
      }

      continue;
    }

    if (!entry) {
      entry = m_table.New(static_cast<unsigned int>(id), HASHKEY(id), 0, 0);
    }

    entry->m_record.Unpack(msg);
    entry->m_haveData = true;
    entry->m_dbkey = id;

    callbackEntry = entry->m_callbacks.Head();
    while (callbackEntry) {
      callbackEntry->m_callback(id, callbackEntry->m_guid, callbackEntry->m_cbArg, true);
      callbackEntry = callbackEntry->Next();
    }

    entry->m_callbacks.Clear();
  }
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::CancelCallback(KEY id, DBCACHECALLBACKPROC cb, void *cbArg) {
  DBCACHEHASH     *entry;
  DBCACHECALLBACK *callbackEntry;
  DBCACHECALLBACK *next;

  entry = m_table.Ptr(static_cast<unsigned int>(id), HASHKEY(id));
  if (!entry) {
    return;
  }

  callbackEntry = entry->m_callbacks.Head();
  while (callbackEntry) {
    next = callbackEntry->Next();
    if (callbackEntry->m_callback == cb && callbackEntry->m_cbArg == cbArg) {
      entry->m_callbacks.DeleteNode(callbackEntry);
    }
    callbackEntry = next;
  }
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::Load() {
  const unsigned long HeaderSize = 16;
  unsigned int        data[0x800];
  char                fileName[260];
  int                 recVersion;
  unsigned long       recSize;
  int                 build;
  HOSFILE             file;
  unsigned long       tag;
  unsigned long       itemSize;
  KEY                 itemId;
  unsigned long       bytesRead;
  DBCACHEHASH        *entry;

  if (!m_persistent) {
    return;
  }

  OsCreateDirectory("WDB", 0);
  SStrPrintf(fileName, sizeof(fileName), "%s/%s", "WDB", m_fileName);
  file = OsCreateFile(fileName, 0x80000000, 1, 3, 0x80, 0x3F3F3F3F);
  if (file == HOSFILE_INVALID) {
    return;
  }

  OsReadFile(file, data, HeaderSize, &bytesRead);
  ASSERT(bytesRead == HeaderSize);
  if (bytesRead != HeaderSize) {
    OsCloseFile(file);
    return;
  }

  CDataStore header(reinterpret_cast<unsigned char *>(data), HeaderSize);
  header.Get(tag);
  ASSERT(tag == m_fileTag);
  if (tag != m_fileTag) {
    OsCloseFile(file);
    return;
  }

  header.Get(build);
  if (build != 3368) {
    OsCloseFile(file);
    return;
  }

  header.Get(recSize);
  if (recSize != sizeof(RECORD)) {
    OsCloseFile(file);
    return;
  }

  header.Get(recVersion);
  if (recVersion == 1) {
    for (;;) {
      OsReadFile(file, data, sizeof(DWORD) + sizeof(KEY), &bytesRead);
      ASSERT(bytesRead == sizeof(DWORD) + sizeof(KEY));

      CDataStore itemHdr(reinterpret_cast<unsigned char *>(data), sizeof(DWORD) + sizeof(KEY));
      itemHdr.Get(itemId);
      itemHdr.Get(itemSize);
      if (!itemId) {
        break;
      }

      ASSERT(itemSize <= (sizeof(data) / sizeof(data[0])));
      OsReadFile(file, data, itemSize, &bytesRead);
      if (bytesRead != itemSize) {
        m_table.Clear();
        break;
      }

      CDataStore rec(reinterpret_cast<unsigned char *>(data), itemSize);
      entry = m_table.Ptr(static_cast<unsigned int>(itemId), HASHKEY(itemId));
      if (!entry) {
        entry = m_table.New(static_cast<unsigned int>(itemId), HASHKEY(itemId), 0, 0);
      }

      entry->m_record.Unpack(&rec);
      entry->m_haveData = true;
      entry->m_dbkey = itemId;
    }
  }

  OsCloseFile(file);
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::Save() {
  char          fileName[0x104];
  unsigned long endMarker;
  KEY           endMarkerKey;
  HOSFILE       file;
  DBCACHEHASH  *entry;
  unsigned long bytesWritten;

  if (!m_persistent) {
    return;
  }

  OsCreateDirectory("WDB", 0);
  SStrPrintf(fileName, sizeof(fileName), "%s/%s", "WDB", m_fileName);
  file = OsCreateFile(fileName, 0x40000000, 1, 2, 0x80, 0x3F3F3F3F);
  FATALASSERT(file != HOSFILE_INVALID);

  CDataStore store;
  void      *ptr;

  store.Put(m_fileTag);
  store.Put(3368);
  store.Put(static_cast<unsigned long>(sizeof(RECORD)));
  store.Put(1);
  store.Finalize();
  store.GetDataInSitu(ptr, store.Size());
  OsWriteFile(file, ptr, store.Size(), &bytesWritten);
  ASSERT(bytesWritten == store.Size());

  CDataStore r;

  entry = m_table.Head();
  while (entry) {
    if (entry->m_haveData && !entry->m_temp) {
      r.Reset();
      r.Put(entry->m_dbkey);
      r.Put(static_cast<unsigned long>(0));
      entry->m_record.Pack(&r);
      r.Set(sizeof(KEY), r.Size() - sizeof(KEY) - sizeof(unsigned long));
      r.Finalize();
      r.GetDataInSitu(ptr, r.Size());
      OsWriteFile(file, ptr, r.Size(), &bytesWritten);
      ASSERT(bytesWritten == r.Size());
    }

    entry = m_table.Next(entry);
  }

  endMarkerKey = 0;
  OsWriteFile(file, &endMarkerKey, sizeof(endMarkerKey), &bytesWritten);
  ASSERT(bytesWritten == sizeof(endMarkerKey));

  endMarker = 0;
  OsWriteFile(file, &endMarker, sizeof(endMarker), &bytesWritten);
  ASSERT(bytesWritten == sizeof(endMarker));
  OsCloseFile(file);
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::Invalidate(KEY id) {
  DBCACHEHASH *entry;

  entry = m_table.Ptr(static_cast<unsigned int>(id), HASHKEY(id));
  if (!entry) {
    return;
  }

  if (!entry->m_haveData) {
    DenyItem(id);
    return;
  }

  m_table.Delete(entry);
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::Clear() {
  m_table.Clear();
}

template <class RECORD, class KEY, class HASHKEY>
void DBCache<RECORD, KEY, HASHKEY>::SetTemporary(KEY id) {
  DBCACHEHASH *entry;

  entry = m_table.Ptr(static_cast<unsigned int>(id), HASHKEY(id));
  if (entry) {
    entry->m_temp = true;
  }
}

template class DBCache<CreatureStats_C, int, HASHKEY_INT>;
template class DBCache<GameObjectStats_C, int, HASHKEY_INT>;
template class DBCache<ItemStats_C, int, HASHKEY_INT>;
template class DBCache<NPCText, int, HASHKEY_INT>;
template class DBCache<NameCache, unsigned __int64, CHashKeyGUID>;
template class DBCache<GuildStats_C, int, HASHKEY_INT>;
template class DBCache<QuestCache, int, HASHKEY_INT>;
template class DBCache<PageTextCache_C, int, HASHKEY_INT>;
template class DBCache<PetNameCache, int, HASHKEY_INT>;
template class DBCache<CGPetition, int, HASHKEY_INT>;
