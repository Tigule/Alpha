#include <storm.h>

LPVOID SInterlockedExchangePointer(LPVOID *destPtr, LPVOID exchange) {
  return (LPVOID)InterlockedExchange((LPLONG)destPtr, (LONG)exchange);
}

LPVOID SInterlockedCompareExchangePointer(LPVOID *destPtr, LPVOID exchange, LPVOID comperand) {
  return (LPVOID)InterlockedCompareExchange((LPLONG)destPtr, (LONG)exchange, (LONG)comperand);
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
  return InterlockedCompareExchange(destPtr, exchange, comperand);
}

LONGLONG SInterlockedIncrement(LONGLONG *valuePtr) {
  return ++*valuePtr;
}

LONGLONG SInterlockedDecrement(LONGLONG *valuePtr) {
  return --*valuePtr;
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta) {
  LONGLONG original = *valuePtr;
  *valuePtr += delta;
  return original;
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta) {
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

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta) {
  *valuePtr -= delta;
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  *valuePtr += delta;
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  *valuePtr -= delta;
}
