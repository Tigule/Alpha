#include <storm.h>

void *__fastcall SInterlockedExchangePointer(void **destPtr, void *exchange) {
  return (void *)InterlockedExchange((LPLONG)destPtr, (LONG)exchange);
}

void *__fastcall SInterlockedCompareExchangePointer(void **destPtr, void *exchange, void *comperand) {
  return InterlockedCompareExchange(destPtr, exchange, comperand);
}

long __fastcall SInterlockedIncrement(long *valuePtr) {
  return InterlockedIncrement(valuePtr);
}

long __fastcall SInterlockedDecrement(long *valuePtr) {
  return InterlockedDecrement(valuePtr);
}

long __fastcall SInterlockedExchangeAdd(long *valuePtr, long delta) {
  return InterlockedExchangeAdd(valuePtr, delta);
}

long __fastcall SInterlockedExchangeSub(long *valuePtr, long delta) {
  return InterlockedExchangeAdd(valuePtr, -delta);
}

long __fastcall SInterlockedExchange(long *destPtr, long exchange) {
  return InterlockedExchange(destPtr, exchange);
}

long __fastcall SInterlockedCompareExchange(long *destPtr, long exchange, long comperand) {
  return (long)InterlockedCompareExchange((void **)destPtr, (void *)exchange, (void *)comperand);
}

__int64 __fastcall SInterlockedIncrement(__int64 *valuePtr) {
  __int64 result = ++*valuePtr;
  return result;
}

__int64 __fastcall SInterlockedDecrement(__int64 *valuePtr) {
  __int64 result = --*valuePtr;
  return result;
}

__int64 __fastcall SInterlockedExchangeAdd(__int64 *valuePtr, long delta) {
  __int64 result = *valuePtr;
  *valuePtr += delta;
  return result;
}

__int64 __fastcall SInterlockedExchangeSub(__int64 *valuePtr, long delta) {
  __int64 result = *valuePtr;
  *valuePtr -= delta;
  return result;
}

__int64 __fastcall SInterlockedExchangeAdd(__int64 *valuePtr, const __int64 &delta) {
  __int64 result = *valuePtr;
  *valuePtr += delta;
  return result;
}

__int64 __fastcall SInterlockedExchangeSub(__int64 *valuePtr, const __int64 &delta) {
  __int64 result = *valuePtr;
  *valuePtr -= delta;
  return result;
}

__int64 __fastcall SInterlockedRead(const __int64 *sourcePtr) {
  __int64 result = *sourcePtr;
  return result;
}

__int64 __fastcall SInterlockedExchange(__int64 *destPtr, const __int64 &exchange) {
  __int64 result = *destPtr;
  *destPtr = exchange;
  return result;
}

__int64 __fastcall SInterlockedCompareExchange(__int64 *destPtr, const __int64 &exchange, const __int64 &comperand) {
  __int64 result = *destPtr;
  if (result == comperand) {
    *destPtr = exchange;
  }
  return result;
}

void __fastcall SInterlockedIncrementNonAtomic(__int64 *valuePtr) {
  *valuePtr += 1;
}

void __fastcall SInterlockedDecrementNonAtomic(__int64 *valuePtr) {
  *valuePtr -= 1;
}

void __fastcall SInterlockedAddNonAtomic(__int64 *valuePtr, long delta) {
  *valuePtr += delta;
}

void __fastcall SInterlockedSubNonAtomic(__int64 *valuePtr, long delta) {
  *valuePtr -= delta;
}

void __fastcall SInterlockedAddNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr += delta;
}

void __fastcall SInterlockedSubNonAtomic(__int64 *valuePtr, const __int64 &delta) {
  *valuePtr -= delta;
}
