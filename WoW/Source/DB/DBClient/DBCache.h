#ifndef WOW_SOURCE_DB_DBCLIENT_DBCACHE_H
#define WOW_SOURCE_DB_DBCLIENT_DBCACHE_H

#include <stpl.h>

#include "Net/NetClient/NetClient.h"

class CDataStore;
class CGContainer_C;
class CGPlayer_C;

class HASHKEY_INT {
 public:
  HASHKEY_INT(UINT) {
  }

  HASHKEY_INT(int) {
  }

  HASHKEY_INT() {
  }

  bool operator==(const HASHKEY_INT &) const {
    return true;
  }

  HASHKEY_INT &operator=(const HASHKEY_INT &) {
    return *this;
  }
};

typedef void (*DBCACHECALLBACKPROC)(int result, const DWORDLONG &guid, LPVOID arg, bool haveData);

NODEDECL(DBCACHECALLBACK) {
  DBCACHECALLBACKPROC m_callback;
  DWORDLONG           m_guid;
  LPVOID              m_cbArg;
};

template <class RECORD, class KEY, class HASHKEY>
class DBCache {
 public:
  struct DBCACHEHASH : public TSHashObject<DBCACHEHASH, HASHKEY> {
    DBCACHEHASH(const DBCACHEHASH &entry);
    DBCACHEHASH() : m_haveData(false), m_temp(false) {
    }

    ~DBCACHEHASH() {
      m_callbacks.Clear();
    }

    RECORD m_record;
    KEY    m_dbkey;
    bool   m_haveData;
    LISTDECL(DBCACHECALLBACK, m_callbacks);
    bool m_temp;
  };

  typedef DBCACHEHASH       *PDBCACHEHASH;
  typedef const DBCACHEHASH *PCDBCACHEHASH;

  DBCache(const DBCache<RECORD, KEY, HASHKEY> &cache);
  DBCache(DWORD fileTag, LPCSTR fileName, NETMESSAGE singleQuery, NETMESSAGE multiQuery, bool requireGuids, bool persistent);
  ~DBCache();

  const RECORD *GetRecord(KEY id, const DWORDLONG &guid, DBCACHECALLBACKPROC cb, LPVOID cbArg);
  void          VerifyPack(CGContainer_C *container, DBCACHECALLBACKPROC callback, LPVOID arg);
  void          VerifyCache(CGPlayer_C *player, DBCACHECALLBACKPROC callback, LPVOID arg);
  void          AddItem(RECORD *item, KEY key);
  void          AddItems(CDataStore *msg, bool single);
  void          DenyItem(KEY key);
  void          CancelCallback(KEY id, DBCACHECALLBACKPROC cb, LPVOID cbArg);
  void          Load();
  void          Save();
  void          Clear();
  void          Invalidate(KEY id);
  void          SetTemporary(KEY id);

 private:
  TSHashTable<DBCACHEHASH, HASHKEY> m_table;
  DWORD                             m_fileTag;
  LPCSTR                            m_fileName;
  NETMESSAGE                        m_singleQueryMsg;
  NETMESSAGE                        m_multiQueryMsg;
  bool                              m_requireGuids;
  bool                              m_persistent;
};

#endif
