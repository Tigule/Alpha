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

LONG SInterlockedExchange(LONG *destPtr, LONG exchange) {
  LONG original;

  pthread_mutex_lock(&s_lock);
  original = *destPtr;
  *destPtr = exchange;
  pthread_mutex_unlock(&s_lock);
  return original;
}
