#include <windows.h>
#include <dsound.h>
#include <storm.h>
#include <stdio.h>
#include <time.h>

LPVOID SInterlockedExchangePointer(LPVOID *destPtr, LPVOID exchange) {
  return (LPVOID)InterlockedExchange((LPLONG)destPtr, (LONG)exchange);
}

LPVOID SInterlockedCompareExchangePointer(LPVOID *destPtr, LPVOID exchange, LPVOID comperand) {
  return InterlockedCompareExchangePointer(destPtr, exchange, comperand);
}

#if !BUILD_PORTABLE
__declspec(naked) long SInterlockedIncrement(long *valuePtr) {
  __asm {
    mov eax, 1
    lock xadd [ecx], eax
    inc eax
    ret
  }
}

__declspec(naked) long SInterlockedDecrement(long *valuePtr) {
  __asm {
    mov eax, -1
    lock xadd [ecx], eax
    dec eax
    ret
  }
}

__declspec(naked) long SInterlockedExchangeAdd(long *valuePtr, long delta) {
  __asm {
    mov eax, edx
    lock xadd [ecx], eax
    ret
  }
}

__declspec(naked) long SInterlockedExchangeSub(long *valuePtr, long delta) {
  __asm {
    sub eax, eax
    sub eax, edx
    lock xadd [ecx], eax
    ret
  }
}

__declspec(naked) long SInterlockedExchange(long *destPtr, long exchange) {
  __asm {
    mov eax, edx
    lock xchg [ecx], eax
    ret
  }
}

__declspec(naked) long SInterlockedCompareExchange(long *destPtr, long exchange, long comperand) {
  __asm {
    mov eax, [esp + 4]
    lock cmpxchg [ecx], edx
    ret 4
  }
}

__declspec(naked) LONGLONG SInterlockedIncrement(LONGLONG *valuePtr) {
  __asm {
    push ebx
    push edi
    mov edi, ecx
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    add ebx, 1
    adc ecx, 0
    lock cmpxchg8b [edi]
    jnz writeloop
    mov eax, ebx
    mov edx, ecx
    pop edi
    pop ebx
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedDecrement(LONGLONG *valuePtr) {
  __asm {
    push ebx
    push edi
    mov edi, ecx
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    sub ebx, 1
    sbb ecx, 0
    lock cmpxchg8b [edi]
    jnz writeloop
    mov eax, ebx
    mov edx, ecx
    pop edi
    pop ebx
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta) {
  __asm {
    push ebp
    push ebx
    push esi
    push edi
    mov edi, ecx
    mov ebp, 0x7fffffff
    mov esi, edx
    cmp ebp, esi
    sbb ebp, ebp
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    add ebx, esi
    adc ecx, ebp
    lock cmpxchg8b [edi]
    jnz writeloop
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta) {
  __asm {
    push ebp
    push ebx
    push esi
    push edi
    mov edi, ecx
    mov ebp, 0x7fffffff
    mov esi, edx
    cmp ebp, esi
    sbb ebp, ebp
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    sub ebx, esi
    sbb ecx, ebp
    lock cmpxchg8b [edi]
    jnz writeloop
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta) {
  __asm {
    push ebp
    push ebx
    push esi
    push edi
    mov edi, ecx
    mov esi, [edx]
    mov ebp, [edx + 4]
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    add ebx, esi
    adc ecx, ebp
    lock cmpxchg8b [edi]
    jnz writeloop
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta) {
  __asm {
    push ebp
    push ebx
    push esi
    push edi
    mov edi, ecx
    mov esi, [edx]
    mov ebp, [edx + 4]
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    mov ebx, eax
    mov ecx, edx
    sub ebx, esi
    sbb ecx, ebp
    lock cmpxchg8b [edi]
    jnz writeloop
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedRead(const LONGLONG *sourcePtr) {
  __asm {
    mov eax, ebx
    mov edx, ecx
    lock cmpxchg8b [ecx]
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange) {
  __asm {
    push ebx
    push edi
    mov edi, ecx
    mov ebx, [edx]
    mov ecx, [edx + 4]
    mov eax, [edi]
    mov edx, [edi + 4]
  writeloop:
    lock cmpxchg8b [edi]
    jnz writeloop
    pop edi
    pop ebx
    ret
  }
}

__declspec(naked) LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand) {
  __asm {
    push ebx
    push edi
    mov edi, ecx
    mov eax, [esp + 12]
    mov ebx, [edx]
    mov ecx, [edx + 4]
    mov edx, [eax + 4]
    mov eax, [eax]
    lock cmpxchg8b [edi]
    pop edi
    pop ebx
    ret 4
  }
}

__declspec(naked) void SInterlockedIncrementNonAtomic(LONGLONG *valuePtr) {
  __asm {
    lock add dword ptr [ecx], 1
    lock adc dword ptr [ecx + 4], 0
    ret
  }
}

__declspec(naked) void SInterlockedDecrementNonAtomic(LONGLONG *valuePtr) {
  __asm {
    lock sub dword ptr [ecx], 1
    lock sbb dword ptr [ecx + 4], 0
    ret
  }
}

__declspec(naked) void SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta) {
  __asm {
    mov eax, 0x7fffffff
    cmp eax, edx
    sbb eax, eax
    lock add [ecx], edx
    lock adc [ecx + 4], eax
    ret
  }
}

__declspec(naked) void SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta) {
  __asm {
    mov eax, 0x7fffffff
    cmp eax, edx
    sbb eax, eax
    lock sub [ecx], edx
    lock sbb [ecx + 4], eax
    ret
  }
}

__declspec(naked) void SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  __asm {
    mov eax, [edx]
    mov edx, [edx + 4]
    lock add [ecx], eax
    lock adc [ecx + 4], edx
    ret
  }
}

__declspec(naked) void SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  __asm {
    mov eax, [edx]
    mov edx, [edx + 4]
    lock sub [ecx], eax
    lock sbb [ecx + 4], edx
    ret
  }
}
#elif defined(_MSC_VER)
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
  return InterlockedIncrement64(valuePtr);
}

LONGLONG SInterlockedDecrement(LONGLONG *valuePtr) {
  return InterlockedDecrement64(valuePtr);
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta) {
  return InterlockedExchangeAdd64(valuePtr, delta);
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta) {
  return InterlockedExchangeAdd64(valuePtr, -(LONGLONG)delta);
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta) {
  return InterlockedExchangeAdd64(valuePtr, delta);
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta) {
  return InterlockedExchangeAdd64(valuePtr, -delta);
}

LONGLONG SInterlockedRead(const LONGLONG *sourcePtr) {
  return InterlockedCompareExchange64((LONGLONG *)sourcePtr, 0, 0);
}

LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange) {
  return InterlockedExchange64(destPtr, exchange);
}

LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand) {
  return InterlockedCompareExchange64(destPtr, exchange, comperand);
}

void SInterlockedIncrementNonAtomic(LONGLONG *valuePtr) {
  InterlockedIncrement64(valuePtr);
}

void SInterlockedDecrementNonAtomic(LONGLONG *valuePtr) {
  InterlockedDecrement64(valuePtr);
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta) {
  InterlockedExchangeAdd64(valuePtr, delta);
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta) {
  InterlockedExchangeAdd64(valuePtr, -(LONGLONG)delta);
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  InterlockedExchangeAdd64(valuePtr, delta);
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  InterlockedExchangeAdd64(valuePtr, -delta);
}
#else
long SInterlockedIncrement(long *valuePtr) {
  return __atomic_add_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

long SInterlockedDecrement(long *valuePtr) {
  return __atomic_sub_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

long SInterlockedExchangeAdd(long *valuePtr, long delta) {
  return __atomic_fetch_add(valuePtr, delta, __ATOMIC_SEQ_CST);
}

long SInterlockedExchangeSub(long *valuePtr, long delta) {
  return __atomic_fetch_sub(valuePtr, delta, __ATOMIC_SEQ_CST);
}

long SInterlockedExchange(long *destPtr, long exchange) {
  return __atomic_exchange_n(destPtr, exchange, __ATOMIC_SEQ_CST);
}

long SInterlockedCompareExchange(long *destPtr, long exchange, long comperand) {
  return __sync_val_compare_and_swap(destPtr, comperand, exchange);
}

LONGLONG SInterlockedIncrement(LONGLONG *valuePtr) {
  return __atomic_add_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedDecrement(LONGLONG *valuePtr) {
  return __atomic_sub_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, long delta) {
  return __atomic_fetch_add(valuePtr, delta, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, long delta) {
  return __atomic_fetch_sub(valuePtr, delta, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedExchangeAdd(LONGLONG *valuePtr, const LONGLONG &delta) {
  return __atomic_fetch_add(valuePtr, delta, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedExchangeSub(LONGLONG *valuePtr, const LONGLONG &delta) {
  return __atomic_fetch_sub(valuePtr, delta, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedRead(const LONGLONG *sourcePtr) {
  return __atomic_load_n(sourcePtr, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedExchange(LONGLONG *destPtr, const LONGLONG &exchange) {
  return __atomic_exchange_n(destPtr, exchange, __ATOMIC_SEQ_CST);
}

LONGLONG SInterlockedCompareExchange(LONGLONG *destPtr, const LONGLONG &exchange, const LONGLONG &comperand) {
  return __sync_val_compare_and_swap(destPtr, comperand, exchange);
}

void SInterlockedIncrementNonAtomic(LONGLONG *valuePtr) {
  __atomic_add_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

void SInterlockedDecrementNonAtomic(LONGLONG *valuePtr) {
  __atomic_sub_fetch(valuePtr, 1, __ATOMIC_SEQ_CST);
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, long delta) {
  __atomic_add_fetch(valuePtr, delta, __ATOMIC_SEQ_CST);
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, long delta) {
  __atomic_sub_fetch(valuePtr, delta, __ATOMIC_SEQ_CST);
}

void SInterlockedAddNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  __atomic_add_fetch(valuePtr, delta, __ATOMIC_SEQ_CST);
}

void SInterlockedSubNonAtomic(LONGLONG *valuePtr, const LONGLONG &delta) {
  __atomic_sub_fetch(valuePtr, delta, __ATOMIC_SEQ_CST);
}
#endif
