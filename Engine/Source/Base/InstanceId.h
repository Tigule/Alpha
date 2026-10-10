#pragma once

#include "Base.h"
#include <stpl.h>

DECLARE_STRICT_HANDLE(INSTANCELOCK);

template <class T>
class TInstanceId : public TSLinkedNode<T> {
 public:
  TInstanceId() : m_id(0) {
  }

  virtual ~TInstanceId() {
  }

  void SetId(DWORD id) {
    m_id = id;
  }

  DWORD Id() const {
    return m_id;
  }

 private:
  DWORD m_id;
};

template <class T, UINT SLOTCOUNT>
class TInstanceIdTable {
 public:
  TInstanceIdTable() {
  }

  TInstanceIdTable(const TInstanceIdTable &);

  static int Slots() {
    return SLOTCOUNT;
  }

  DWORD Link(T *instance) {
    DWORD id;
    UINT  slot;
    int   found;

    m_idCritsect.Enter();
    for (;;) {
      id = ++m_id;
      if (!id) {
        m_idWrapped = 1;
        continue;
      }
      if (!m_idWrapped) {
        break;
      }

      slot = id & (SLOTCOUNT - 1);
      m_idLock[slot].Enter(0);
      found = 0;
      ITERATELIST(T, m_idList[slot], ptr) {
        if (ptr->Id() == id) {
          found = 1;
          break;
        }
      }
      m_idLock[slot].Leave(0);
      if (!found) {
        break;
      }
    }

    instance->SetId(id);
    slot = id & (SLOTCOUNT - 1);
    m_idLock[slot].Enter(1);
    m_idList[slot].LinkNode(instance, LIST_TAIL, 0);
    m_idLock[slot].Leave(1);
    m_idCritsect.Leave();
    return id;
  }

  void Unlink(T *instance) {
    DWORD id = instance->Id();
    UINT  slot;

    if (!id) {
      return;
    }

    slot = id & (SLOTCOUNT - 1);
    m_idLock[slot].Enter(1);
    instance->Unlink();
    instance->SetId(0);
    m_idLock[slot].Leave(1);
  }

  T *Lock(DWORD id, int forWriting, INSTANCELOCK &instanceLock, LPCSTR, DWORD) {
    int slot;
    if (id) {
      slot = id & (SLOTCOUNT - 1);
      m_idLock[slot].Enter(forWriting);
      ITERATELIST(T, m_idList[slot], instance) {
        if (instance->Id() == id) {
          instanceLock = (INSTANCELOCK)(forWriting ? slot + SLOTCOUNT : slot);
          return instance;
        }
      }
      m_idLock[slot].Leave(forWriting);
    }
    instanceLock = (INSTANCELOCK)-1;
    return 0;
  }

  void Unlock(INSTANCELOCK instanceLock, LPCSTR, DWORD) {
    UINT encoded = (UINT)instanceLock;
    UINT slot;
    int  forWriting;

    if (encoded == (UINT)-1) {
      return;
    }

    forWriting = encoded >= SLOTCOUNT;
    slot = encoded & (SLOTCOUNT - 1);
    m_idLock[slot].Leave(forWriting);
  }

 private:
  class Iterator {
    friend class TInstanceIdTable<T, SLOTCOUNT>;

    TInstanceIdTable<T, SLOTCOUNT> &m_table;
    int                             m_slot;
    T                              *m_next;

    Iterator(const Iterator &);

    Iterator(TInstanceIdTable<T, SLOTCOUNT> &table) : m_table(table) {
    }

    Iterator &operator=(const Iterator &);

   public:
    void SetSlot(int slot, int forWriting) {
      m_slot = slot;
      SlotBegin(forWriting);
    }

    T *Next(int forWriting) {
      T *instance = SlotNext();

      if (!instance) {
        SlotEnd(forWriting);
      }
      return instance;
    }

    void SlotBegin(int forWriting) {
      m_table.m_idLock[m_slot].Enter(forWriting);
      m_next = m_table.m_idList[m_slot].Head();
    }

    void SlotEnd(int forWriting) {
      m_table.m_idLock[m_slot].Leave(forWriting);
    }

    T *SlotNext() {
      T *instance = m_next;

      if (instance) {
        m_next = instance->Next();
      }
      return instance;
    }
  };

  friend class Iterator;

  SCritSect m_idCritsect;
  DWORD     m_id;
  BOOL      m_idWrapped;
  CSRWLock  m_idLock[SLOTCOUNT];
  LISTDECL(T, m_idList[SLOTCOUNT]);

  TInstanceIdTable &operator=(const TInstanceIdTable &);
};

template <class T, UINT SLOTCOUNT>
class TSingletonInstanceId : public TInstanceId<T> {
 public:
  TSingletonInstanceId() {
  }

  typedef TInstanceIdTable<T, SLOTCOUNT> Table;

  static Table &GetTable() {
    return s_idTable;
  }

 private:
  static Table s_idTable;
};

template <class T, UINT SLOTCOUNT>
TInstanceIdTable<T, SLOTCOUNT> TSingletonInstanceId<T, SLOTCOUNT>::s_idTable;
