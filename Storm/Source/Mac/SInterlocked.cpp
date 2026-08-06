#include <storm.h>

#include <pthread.h>

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

LPVOID SInterlockedExchangePointer(LPVOID *destPtr, LPVOID exchange) {
  LPVOID original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  *destPtr = exchange;
  pthread_mutex_unlock(&s_lock);
  return original;
}

LPVOID SInterlockedCompareExchangePointer(LPVOID *destPtr, LPVOID exchange, LPVOID comperand) {
  LPVOID original;

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

LONGLONG SInterlockedIncrement(LONGLONG *valuePtr) {
  return ++*valuePtr;
}

LONGLONG SInterlockedDecrement(LONGLONG *valuePtr) {
  return --*valuePtr;
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, LONG delta) {
  LONGLONG original = *valuePtr;
  *valuePtr += delta;
  return original;
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, LONG delta) {
  LONGLONG original = *valuePtr;
  *valuePtr -= delta;
  return original;
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta) {
  LONGLONG original = *valuePtr;
  *valuePtr += delta;
  return original;
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta) {
  LONGLONG original = *valuePtr;
  *valuePtr -= delta;
  return original;
}

LONGLONG SInterlockedRead(const LONGLONG *sourcePtr) {
  return *sourcePtr;
}

LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange) {
  LONGLONG original = *destPtr;
  *destPtr = exchange;
  return original;
}

LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand) {
  LONGLONG original = *destPtr;
  if (original == comperand) {
    *destPtr = exchange;
  }
  return original;
}

void SInterlockedIncrementNonAtomic(LONGLONG *valuePtr) {
  *valuePtr += 1;
}

void SInterlockedDecrementNonAtomic(LONGLONG *valuePtr) {
  *valuePtr -= 1;
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, LONG delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, LONG delta) {
  *valuePtr -= delta;
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  *valuePtr -= delta;
}
