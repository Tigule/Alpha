#include <Base/Base.h>

#include <storm.h>
#include <stpl.h>

#include <pthread.h>

NODEDECL(TLSData) {
  TSGrowableArray<LPVOID> m_values;
};

static SCritSect s_critsect;
static int       s_initialized;
static DWORD     s_nextIndex;
static LISTDECL(TLSData, s_tlsList);
static pthread_key_t s_key;

static TLSData *GetTlsData() {
  TLSData *data = static_cast<TLSData *>(pthread_getspecific(s_key));

  if (!data) {
    s_critsect.Enter();

    data = NEW(TLSData);
    s_tlsList.LinkNode(data, LIST_TAIL, 0);

    s_critsect.Leave();

    pthread_setspecific(s_key, data);
  }

  return data;
}

DWORD OsTlsAlloc() {
  DWORD index;

  s_critsect.Enter();

  if (!s_initialized) {
    s_initialized = 1;
    s_nextIndex = 0;
    pthread_key_create(&s_key, 0);
  }

  index = s_nextIndex++;

  s_critsect.Leave();
  return index;
}

void OsTlsFree(DWORD index) {
}

LPVOID OsTlsGetValue(DWORD index) {
  TLSData *data = GetTlsData();

  if (index < data->m_values.Count()) {
    return data->m_values[index];
  }

  return 0;
}

BOOL OsTlsSetValue(DWORD index, LPVOID value) {
  TLSData *data = GetTlsData();

  if (index >= data->m_values.Count()) {
    data->m_values.SetCount(index + 1);
  }

  data->m_values[index] = value;
  return TRUE;
}
