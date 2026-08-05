#include <storm.h>

#include <pthread.h>

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

void *SInterlockedExchangePointer(void **destPtr, void *exchange) {
  void *original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  *destPtr = exchange;
  pthread_mutex_unlock(&s_lock);
  return original;
}

void *SInterlockedCompareExchangePointer(void **destPtr, void *exchange, void *comperand) {
  void *original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  if (*destPtr == comperand) {
    *destPtr = exchange;
  }
  pthread_mutex_unlock(&s_lock);
  return original;
}

LONG SInterlockedIncrement(LONG *valuePtr) {
  LONG value;

  pthread_mutex_lock(&s_lock);
  value = *valuePtr + 1;
  *valuePtr = value;
  pthread_mutex_unlock(&s_lock);
  return value;
}

LONG SInterlockedDecrement(LONG *valuePtr) {
  LONG value;

  pthread_mutex_lock(&s_lock);
  value = *valuePtr - 1;
  *valuePtr = value;
  pthread_mutex_unlock(&s_lock);
  return value;
}

LONG SInterlockedExchangeAdd(LONG *valuePtr, LONG delta) {
  LONG original;

  pthread_mutex_lock(&s_lock);
  original = *valuePtr;
  *valuePtr += delta;
  pthread_mutex_unlock(&s_lock);
  return original;
}

LONG SInterlockedExchangeSub(LONG *valuePtr, LONG delta) {
  return SInterlockedExchangeAdd(valuePtr, -delta);
}

LONG SInterlockedExchange(LONG *destPtr, LONG exchange) {
  LONG original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  *destPtr = exchange;
  pthread_mutex_unlock(&s_lock);
  return original;
}

LONG SInterlockedCompareExchange(LONG *destPtr, LONG exchange, LONG comperand) {
  LONG original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  if (original == comperand) {
    *destPtr = exchange;
  }
  pthread_mutex_unlock(&s_lock);
  return original;
}

__int64 SInterlockedIncrement(__int64 *valuePtr) {
  return ++*valuePtr;
}

__int64 SInterlockedDecrement(__int64 *valuePtr) {
  return --*valuePtr;
}

__int64 SInterlockedExchangeAdd(__int64 *valuePtr, LONG delta) {
  __int64 original = *valuePtr;
  *valuePtr += delta;
  return original;
}

__int64 SInterlockedExchangeSub(__int64 *valuePtr, LONG delta) {
  __int64 original = *valuePtr;
  *valuePtr -= delta;
  return original;
}

__int64 SInterlockedExchangeAdd(__int64 *valuePtr, const __int64 &delta) {
  __int64 original = *valuePtr;
  *valuePtr += delta;
  return original;
}

__int64 SInterlockedExchangeSub(__int64 *valuePtr, const __int64 &delta) {
  __int64 original = *valuePtr;
  *valuePtr -= delta;
  return original;
}

__int64 SInterlockedRead(const __int64 *sourcePtr) {
  return *sourcePtr;
}

__int64 SInterlockedExchange(__int64 *destPtr, const __int64 &exchange) {
  __int64 original = *destPtr;
  *destPtr = exchange;
  return original;
}

__int64 SInterlockedCompareExchange(__int64 *destPtr, const __int64 &exchange, const __int64 &comperand) {
  __int64 original = *destPtr;
  if (original == comperand) {
    *destPtr = exchange;
  }
  return original;
}

void SInterlockedIncrementNonAtomic(__int64 *valuePtr) {
  *valuePtr += 1;
}

void SInterlockedDecrementNonAtomic(__int64 *valuePtr) {
  *valuePtr -= 1;
}

void SInterlockedAddNonAtomic(__int64 *valuePtr, LONG delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(__int64 *valuePtr, LONG delta) {
  *valuePtr -= delta;
}

void SInterlockedAddNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr -= delta;
}
