#include <storm.h>
#include <stpl.h>
#include "W32/ISThread.h"

typedef struct _IDHASHENTRY {
  DWORD         id;
  DWORD         sequence;
  SEVTHANDLER   handler;
  _IDHASHENTRY *next;
} IDHASHENTRY, *IDHASHENTRYPTR;

typedef struct _IDHASHTABLE {
  IDHASHENTRYPTR *data;
  DWORD           size;
  DWORD           used;
  _IDHASHTABLE   *next;
} IDHASHTABLE, *IDHASHTABLEPTR;

typedef struct _TYPEHASHENTRY {
  DWORD           type;
  DWORD           subtype;
  DWORD           sequence;
  IDHASHTABLEPTR  idhashtable;
  _TYPEHASHENTRY *next;
} TYPEHASHENTRY, *TYPEHASHENTRYPTR;

NODEDECL(BREAKCMD) {
  LPVOID data;
};
typedef BREAKCMD *BREAKCMDPTR;

static LISTDECL(BREAKCMD, s_breakcmdlist);
static CCritSect         s_critsect;
static LONG              s_dispatchesinprogress;
static int               s_modified;
static TYPEHASHENTRYPTR *s_typehashtable;
static DWORD             s_typehashtablesize;
static DWORD             s_typehashtableused;

static DWORD ComputeNewTableSize(DWORD currentused) {
  DWORD result = 1;

  currentused = currentused * 2 + 2;
  while (result <= currentused) {
    result <<= 1;
  }

  return result;
}

static void CopyIdHashTable(IDHASHTABLEPTR dest, IDHASHTABLEPTR source) {
  dest->size = source->size;
  dest->used = source->used;
  dest->data = (IDHASHENTRYPTR *)ALLOC(dest->size * sizeof(IDHASHENTRYPTR));

  for (DWORD id = 0; id < source->size; ++id) {
    IDHASHENTRYPTR  entry = source->data[id];
    IDHASHENTRYPTR *tail = &dest->data[id];
    while (entry) {
      *tail = NEW(IDHASHENTRY);
      **tail = *entry;
      tail = &(*tail)->next;
      entry = entry->next;
    }
    *tail = NULL;
  }
}

static void DeleteIdHashTable(IDHASHTABLEPTR idhashtable) {
  IDHASHENTRYPTR entry;
  IDHASHENTRYPTR next;
  DWORD          loop;

  for (loop = 0; loop < idhashtable->size; ++loop) {
    entry = idhashtable->data[loop];
    while (entry) {
      next = entry->next;
      idhashtable->data[loop] = next;
      delete entry;
      entry = idhashtable->data[loop];
    }
  }

  SMemFree(idhashtable->data, __FILE__, __LINE__, 0);
  delete idhashtable;
}

static TYPEHASHENTRYPTR FindTypeHashEntry(DWORD type, DWORD subtype) {
  if (s_typehashtable && s_typehashtablesize) {
    TYPEHASHENTRYPTR entry = s_typehashtable[(type ^ subtype) & (s_typehashtablesize - 1)];
    while (entry && (entry->type != type || entry->subtype != subtype)) {
      entry = entry->next;
    }
    return entry;
  }

  return NULL;
}

extern "C" BOOL APIENTRY SEvtBreakHandlerChain(LPVOID data) {
  BREAKCMDPTR node;

  s_critsect.Enter();
  node = s_breakcmdlist.NewNode(LIST_TAIL, 0, 0);
  node->data = data;
  s_critsect.Leave();

  return TRUE;
}

extern "C" BOOL APIENTRY SEvtDestroy() {
  s_critsect.Enter();

  for (DWORD loop = 0; loop < s_typehashtablesize; ++loop) {
    TYPEHASHENTRYPTR entry;
    while ((entry = s_typehashtable[loop]) != NULL) {
      while (entry->idhashtable) {
        IDHASHTABLEPTR oldtable = entry->idhashtable;
        entry->idhashtable = oldtable->next;
        DeleteIdHashTable(oldtable);
      }
      s_typehashtable[loop] = entry->next;
      DEL(entry);
    }
  }
  DELIFUSED(s_typehashtable);

  s_typehashtable = NULL;
  s_typehashtablesize = 0;
  s_typehashtableused = 0;
  s_modified = TRUE;

  s_critsect.Leave();
  return TRUE;
}

extern "C" BOOL APIENTRY SEvtDispatch(DWORD type, DWORD subtype, DWORD id, LPVOID data) {
  DWORD          currsequence;
  BOOL           success;
  IDHASHENTRYPTR currptr;

  InterlockedIncrement((LPLONG)&s_dispatchesinprogress);
  success = FALSE;
  currsequence = 0xFFFFFFFF;
  currptr = NULL;

  for (;;) {
    SEVTHANDLER handler;
    s_critsect.Enter();
    ITERATELIST(BREAKCMD, s_breakcmdlist, breakcmd) {
      if (breakcmd->data == data) {
        s_breakcmdlist.DeleteNode(breakcmd);
        s_critsect.Leave();
        goto dispatchdone;
      }
    }

    if (!currptr || s_modified) {
      TYPEHASHENTRYPTR typeentry;
      IDHASHTABLEPTR   table;

      currptr = NULL;
      typeentry = FindTypeHashEntry(type, subtype);
      if (typeentry && typeentry->idhashtable) {
        table = typeentry->idhashtable;
        if (table->data && table->size) {
          currptr = table->data[id & (table->size - 1)];
          while (currptr && (currptr->id != id || currptr->sequence >= currsequence)) {
            currptr = currptr->next;
          }

          if (s_dispatchesinprogress == 1) {
            s_modified = FALSE;
          }
        }
      }
    }

    handler = NULL;
    if (currptr) {
      handler = currptr->handler;
      currsequence = currptr->sequence;
      do {
        currptr = currptr->next;
      } while (currptr && currptr->id != id);
    }
    s_critsect.Leave();

    if (handler) {
      success = TRUE;
      handler(data);
    }

    if (!currptr) {
      break;
    }
  }

dispatchdone:
  InterlockedDecrement((LPLONG)&s_dispatchesinprogress);

  if (s_breakcmdlist.Head()) {
    s_critsect.Enter();
    ITERATELIST(BREAKCMD, s_breakcmdlist, breakcmd) {
      if (breakcmd->data == data) {
        ITERATE_DELETE;
      }
    }
    s_critsect.Leave();
  }

  return success;
}

extern "C" BOOL APIENTRY SEvtPopState(DWORD type, DWORD subtype) {
  BOOL result = FALSE;

  s_critsect.Enter();
  TYPEHASHENTRYPTR typeentry = FindTypeHashEntry(type, subtype);
  if (typeentry) {
    IDHASHTABLEPTR next = typeentry->idhashtable->next;
    if (!next) {
      result = SEvtUnregisterType(type, subtype);
    } else {
      DeleteIdHashTable(typeentry->idhashtable);
      typeentry->idhashtable = next;
    }
  }
  s_modified = TRUE;
  s_critsect.Leave();

  return result;
}

extern "C" BOOL APIENTRY SEvtPushState(DWORD type, DWORD subtype) {
  BOOL result = FALSE;

  s_critsect.Enter();
  TYPEHASHENTRYPTR typeentry = FindTypeHashEntry(type, subtype);
  if (typeentry) {
    IDHASHTABLEPTR copy = NEW(IDHASHTABLE);
    CopyIdHashTable(copy, typeentry->idhashtable);
    copy->next = typeentry->idhashtable;
    typeentry->idhashtable = copy;
    result = TRUE;
    s_modified = TRUE;
  }
  s_critsect.Leave();

  return result;
}

extern "C" BOOL APIENTRY SEvtRegisterHandler(DWORD type, DWORD subtype, DWORD id, DWORD flags, SEVTHANDLER handler) {
  TYPEHASHENTRYPTR typeentry;
  IDHASHTABLEPTR   table;
  IDHASHENTRYPTR   entry;
  DWORD            bucket;

  VALIDATEBEGIN;
  VALIDATE(handler);
  VALIDATE(!flags);
  VALIDATEEND;

  s_critsect.Enter();

  typeentry = FindTypeHashEntry(type, subtype);
  if (!typeentry) {
    if (s_typehashtableused >= s_typehashtablesize / 2) {
      DWORD             newsize;
      DWORD             loop;
      TYPEHASHENTRYPTR *newtable;

      newsize = ComputeNewTableSize(s_typehashtableused);
      newtable = (TYPEHASHENTRYPTR *)SMemAlloc(newsize * sizeof(TYPEHASHENTRYPTR), __FILE__, __LINE__, 8);

      if (s_typehashtable) {
        for (loop = 0; loop < s_typehashtablesize; ++loop) {
          TYPEHASHENTRYPTR current;
          TYPEHASHENTRYPTR next;

          current = s_typehashtable[loop];
          while (current) {
            DWORD bucket;

            next = current->next;
            bucket = (current->type ^ current->subtype) & (newsize - 1);
            current->next = newtable[bucket];
            newtable[bucket] = current;
            current = next;
          }
        }
        SMemFree(s_typehashtable, __FILE__, __LINE__, 0);
      }

      s_typehashtable = newtable;
      s_typehashtablesize = newsize;
    }

    typeentry = new (SMemAlloc(sizeof(TYPEHASHENTRY), __FILE__, __LINE__, 8)) TYPEHASHENTRY;
    typeentry->type = type;
    typeentry->subtype = subtype;
    typeentry->idhashtable = new (SMemAlloc(sizeof(IDHASHTABLE), __FILE__, __LINE__, 8)) IDHASHTABLE;
    bucket = (type ^ subtype) & (s_typehashtablesize - 1);
    typeentry->next = s_typehashtable[bucket];
    s_typehashtable[bucket] = typeentry;
    ++s_typehashtableused;
  }

  table = typeentry->idhashtable;
  if (table->used >= table->size / 2) {
    DWORD            newsize;
    IDHASHENTRYPTR **newtail;
    DWORD            loop;
    IDHASHENTRYPTR  *newtable;

    newsize = ComputeNewTableSize(table->size);
    newtable = (IDHASHENTRYPTR *)SMemAlloc(newsize * sizeof(IDHASHENTRYPTR), __FILE__, __LINE__, 8);
    newtail = (IDHASHENTRYPTR **)SMemAlloc(newsize * sizeof(IDHASHENTRYPTR *), __FILE__, __LINE__, 8);

    for (loop = 0; loop < newsize; ++loop) {
      newtail[loop] = &newtable[loop];
    }

    if (table->data) {
      for (loop = 0; loop < table->size; ++loop) {
        IDHASHENTRYPTR current;

        current = table->data[loop];
        while (current) {
          IDHASHENTRYPTR next;
          DWORD          bucket;

          next = current->next;
          bucket = current->id & (newsize - 1);
          current->next = NULL;
          *newtail[bucket] = current;
          newtail[bucket] = &current->next;
          current = next;
        }
      }
    }

    SMemFree(newtail, __FILE__, __LINE__, 0);
    if (table->data) {
      SMemFree(table->data, __FILE__, __LINE__, 0);
    }

    table->data = newtable;
    table->size = newsize;
  }

  entry = new (SMemAlloc(sizeof(IDHASHENTRY), __FILE__, __LINE__, 8)) IDHASHENTRY;

  entry->id = id;
  entry->sequence = ++typeentry->sequence;
  entry->handler = handler;
  bucket = id & (table->size - 1);
  entry->next = table->data[bucket];
  table->data[bucket] = entry;
  ++table->used;
  s_modified = TRUE;

  s_critsect.Leave();
  return TRUE;
}

extern "C" BOOL APIENTRY SEvtUnregisterHandler(DWORD type, DWORD subtype, DWORD id, SEVTHANDLER handler) {
  BOOL result = FALSE;

  s_critsect.Enter();
  TYPEHASHENTRYPTR typeentry = FindTypeHashEntry(type, subtype);
  if (typeentry) {
    IDHASHTABLEPTR table = typeentry->idhashtable;
    if (table->data && table->size) {
      IDHASHENTRYPTR *link = &table->data[(table->size - 1) & id];
      IDHASHENTRYPTR  entry;
      while ((entry = *link) != NULL) {
        if (entry->id == id && (!handler || entry->handler == handler)) {
          *link = entry->next;
          DEL(entry);
          result = TRUE;
          s_modified = TRUE;
          --typeentry->idhashtable->used;
        } else {
          link = &entry->next;
        }
      }
    }
  }
  s_critsect.Leave();

  return result;
}

extern "C" BOOL APIENTRY SEvtUnregisterType(DWORD type, DWORD subtype) {
  BOOL result = FALSE;

  s_critsect.Enter();
  TYPEHASHENTRYPTR typeentry = FindTypeHashEntry(type, subtype);
  if (typeentry) {
    while (typeentry->idhashtable) {
      IDHASHTABLEPTR oldtable = typeentry->idhashtable;
      typeentry->idhashtable = oldtable->next;
      DeleteIdHashTable(oldtable);
    }

    TYPEHASHENTRYPTR *link = &s_typehashtable[(type ^ subtype) & (s_typehashtablesize - 1)];
    TYPEHASHENTRYPTR  entry;
    while ((entry = *link) != NULL) {
      if (entry == typeentry) {
        *link = entry->next;
        DEL(entry);
        --s_typehashtableused;
      } else {
        link = &entry->next;
      }
    }
    result = TRUE;
    s_modified = TRUE;
  }
  s_critsect.Leave();

  return result;
}
