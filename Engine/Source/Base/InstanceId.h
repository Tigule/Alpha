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
    this->Unlink();
  }

  void SetId(unsigned long id) {
    m_id = id;
  }

  unsigned long Id() {
    return m_id;
  }

 private:
  unsigned long m_id;
};

template <class T, unsigned int SLOTCOUNT>
class TInstanceIdTable {
  class Iterator {
    friend class TInstanceIdTable<T, SLOTCOUNT>;

    TInstanceIdTable<T, SLOTCOUNT> &m_table;
    int                             m_slot;
    T                              *m_next;

    Iterator(TInstanceIdTable<T, SLOTCOUNT> &table) : m_table(table) {
    }

    Iterator(const Iterator &);
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

 public:
  TInstanceIdTable() : m_id(0), m_idWrapped(0) {
  }

  TInstanceIdTable(const TInstanceIdTable &);

  static int Slots() {
    return SLOTCOUNT;
  }

  unsigned long Link(T *instance) {
    unsigned long id;
    unsigned int  slot;
    int           found;
    T            *cursor;
    Iterator      iterator(*this);

    m_idCritsect.Enter();
    for (;;) {
      do {
        ++m_id;
        if (!m_id) {
          m_idWrapped = 1;
        }
      } while (!m_id);

      id = m_id;
      if (!m_idWrapped) {
        break;
      }

      found = 0;
      slot = id & (SLOTCOUNT - 1);
      iterator.SetSlot(slot, 0);
      while ((cursor = iterator.SlotNext()) != 0) {
        if (cursor->Id() == id) {
          found = 1;
          break;
        }
      }
      iterator.SlotEnd(0);
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
    unsigned long id = instance->Id();
    unsigned int  slot;

    if (!id) {
      return;
    }

    slot = id & (SLOTCOUNT - 1);
    m_idLock[slot].Enter(1);
    instance->Unlink();
    instance->SetId(0);
    m_idLock[slot].Leave(1);
  }

  T *Lock(unsigned long id, int forWriting, INSTANCELOCK &instanceLock, const char *, unsigned long) {
    int slot;
    instanceLock = reinterpret_cast<INSTANCELOCK>(-1);
    if (!id) {
      return 0;
    }

    slot = id & (SLOTCOUNT - 1);
    m_idLock[slot].Enter(forWriting);
    ITERATELIST(T, m_idList[slot], instance) {
      if (instance->Id() == id) {
        instanceLock = reinterpret_cast<INSTANCELOCK>(forWriting ? slot + SLOTCOUNT : slot);
        return instance;
      }
    }
    m_idLock[slot].Leave(forWriting);
    return 0;
  }

  void Unlock(INSTANCELOCK instanceLock, const char *, unsigned long) {
    long         encoded = reinterpret_cast<long>(instanceLock);
    unsigned int slot;
    int          forWriting;

    if (encoded == -1) {
      return;
    }

    forWriting = encoded >= static_cast<long>(SLOTCOUNT);
    slot = static_cast<unsigned int>(encoded) & (SLOTCOUNT - 1);
    m_idLock[slot].Leave(forWriting);
  }

 private:
  TInstanceIdTable &operator=(const TInstanceIdTable &);

  SCritSect                m_idCritsect;
  unsigned long            m_id;
  int                      m_idWrapped;
  CSRWLock                 m_idLock[SLOTCOUNT];
  LISTDECL(T, m_idList[SLOTCOUNT]);
};

template <class T, unsigned int SLOTCOUNT>
class TSingletonInstanceId : public TInstanceId<T> {
 public:
  static TInstanceIdTable<T, SLOTCOUNT> &GetTable() {
    return s_idTable;
  }

  static TInstanceIdTable<T, SLOTCOUNT> s_idTable;
};

template <class T, unsigned int SLOTCOUNT>
TInstanceIdTable<T, SLOTCOUNT> TSingletonInstanceId<T, SLOTCOUNT>::s_idTable;
