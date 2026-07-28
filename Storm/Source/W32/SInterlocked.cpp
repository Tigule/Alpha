#include <storm.h>

void *SInterlockedExchangePointer(void **destPtr, void *exchange) {
  return (void *)InterlockedExchange((LPLONG)destPtr, (LONG)exchange);
}

void *SInterlockedCompareExchangePointer(void **destPtr, void *exchange, void *comperand) {
  return InterlockedCompareExchange(destPtr, exchange, comperand);
}

long SInterlockedIncrement(long *valuePtr) {
  return InterlockedIncrement(valuePtr);
}

long SInterlockedDecrement(long *valuePtr) {
  return InterlockedDecrement(valuePtr);
}

long SInterlockedExchangeAdd(long *valuePtr, long delta) {
  return InterlockedExchangeAdd(valuePtr, delta);
}

long SInterlockedExchangeSub(long *valuePtr, long delta) {
  return InterlockedExchangeAdd(valuePtr, -delta);
}

long SInterlockedExchange(long *destPtr, long exchange) {
  return InterlockedExchange(destPtr, exchange);
}

long SInterlockedCompareExchange(long *destPtr, long exchange, long comperand) {
  return (long)InterlockedCompareExchange((void **)destPtr, (void *)exchange, (void *)comperand);
}

__int64 SInterlockedIncrement(__int64 *valuePtr) {
  return ++*valuePtr;
}

__int64 SInterlockedDecrement(__int64 *valuePtr) {
  return --*valuePtr;
}

__int64 SInterlockedExchangeAdd(__int64 *valuePtr, long delta) {
  __int64 original = *valuePtr;
  *valuePtr += delta;
  return original;
}

__int64 SInterlockedExchangeSub(__int64 *valuePtr, long delta) {
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

void SInterlockedAddNonAtomic(__int64 *valuePtr, long delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(__int64 *valuePtr, long delta) {
  *valuePtr -= delta;
}

void SInterlockedAddNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr -= delta;
}
