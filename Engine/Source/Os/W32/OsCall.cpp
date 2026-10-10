#include <Base/Base.h>
#include <Gx/Gx.h>
#include <stpl.h>
#include <W32/ISThread.h>

#include <windows.h>
#include <stdio.h>

struct ThreadStack {
  DWORD m_retAddr;
  DWORD m_funcAddr;
  int   m_logExit;
};

#pragma pack(push, 1)
struct ContextCall {
  DWORD m_funcAddr;
  BYTE  m_depth;
};
#pragma pack(pop)

NODEDECL(ContextTurn) {
  DWORD m_turnId;
  DWORD m_callBufferHead;
};

struct ContextData;

NODEDECL(ThreadData) {
  DWORD        m_threadId;
  LPVOID       m_threadHandle;
  int          m_enabled;
  ContextData *m_contextData;
  ThreadStack  m_funcStack[0x200];
  DWORD        m_funcStackIndex;
  char         m_title[0x80];
};

NODEDECL(ContextData) {
  ThreadData *m_threadData;
  DWORD       m_checksum;
  DWORD       m_turnId;
  DWORD       m_turnIdComplete;
  ContextTurn m_turnBuffer[0x400];
  DWORD       m_turnBufferHead;
  DWORD       m_turnBufferTail;
  ContextCall m_callBuffer[0x100];
  DWORD       m_callBufferHead;
  DWORD       m_callBufferTail;
  char        m_title[0x80];
};

static const DWORD s_crcTable[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static CInitCritSect s_critsect;
static LISTDECL(ThreadData, s_threadDataList);
static LISTDECL(ContextData, s_contextDataList);
static DWORD s_tlsIndex;
static DWORD s_initCount;
static int   s_enable = 1;

int __cdecl   OsCallEnter(DWORD funcAddr, DWORD retAddr);
DWORD __cdecl OsCallExit();

extern "C" __declspec(naked) void __cdecl _pexit() {
  __asm {
    push eax
    push eax
    push ecx
    push edx
    call OsCallExit
    mov [esp + 12], eax
    pop edx
    pop ecx
    pop eax
    ret
  }
}

extern "C" __declspec(naked) void __cdecl _penter() {
  __asm {
    push eax
    push ecx
    push edx
    mov eax, [esp + 16]
    push eax
    mov eax, [esp + 16]
    sub eax, 5
    push eax
    call OsCallEnter
    add esp, 8
    cmp eax, 0
    je no_pexit
    lea eax, _pexit
    mov [esp + 16], eax
  no_pexit:
    pop edx
    pop ecx
    pop eax
    ret
  }
}

void OsCallInitialize(LPCSTR threadName) {
  s_critsect.Enter();

  if (!s_initCount++) {
    s_tlsIndex = OsTlsAlloc();
  }

  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (threadData) {
    DEL(threadData);
  }

  threadData = NEWZERO(ThreadData);

  threadData->m_threadId = GetCurrentThreadId();
  DuplicateHandle(
      GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &threadData->m_threadHandle, 0, FALSE,
      DUPLICATE_SAME_ACCESS
  );
  SStrPrintf(threadData->m_title, sizeof(threadData->m_title), "%s [%u]", threadName, threadData->m_threadId);
  OsTlsSetValue(s_tlsIndex, threadData);

  s_threadDataList.LinkNode(threadData, LIST_TAIL, 0);

  s_critsect.Leave();
}

void OsCallDestroy() {
  s_critsect.Enter();

  if (s_initCount) {
    ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
    if (threadData) {
      OsTlsSetValue(s_tlsIndex, 0);

      if (threadData->m_contextData) {
        threadData->m_contextData->m_threadData = 0;
      }

      CloseHandle(threadData->m_threadHandle);
      DEL(threadData);
    }
  }

  if (!--s_initCount) {
    OsTlsFree(s_tlsIndex);
  }

  s_critsect.Leave();
}

void OsCallGlobalEnable(int enable) {
  s_enable = enable;
}

void OsCallEnable(int enable) {
  if (s_initCount) {
    ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
    if (threadData) {
      threadData->m_enabled = enable;
    }
  }
}

LPVOID OsCallInitializeContext(LPCSTR contextName) {
  ContextData *contextData = NEWZERO(ContextData);

  SStrCopy(contextData->m_title, contextName, sizeof(contextData->m_title));
  contextData->m_checksum = -1;

  s_critsect.Enter();

  s_contextDataList.LinkNode(contextData, LIST_TAIL, 0);

  s_critsect.Leave();
  return contextData;
}

void OsCallDestroyContext(LPVOID contextDataPtr) {
  ContextData *contextData = (ContextData *)contextDataPtr;
  if (!contextData) {
    return;
  }

  s_critsect.Enter();

  if (contextData->m_threadData) {
    contextData->m_threadData->m_contextData = 0;
  }

  DEL(contextData);

  s_critsect.Leave();
}

void OsCallSetContext(LPVOID contextDataPtr) {
  ContextData *contextData = (ContextData *)contextDataPtr;
  if (!s_initCount || !contextData) {
    return;
  }

  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return;
  }

  ASSERT(contextData->m_threadData == 0);
  ASSERT(threadData->m_contextData == 0);

  s_critsect.Enter();
  contextData->m_threadData = threadData;
  threadData->m_contextData = contextData;
  s_critsect.Leave();
}

void OsCallResetContext(LPVOID contextDataPtr) {
  ThreadData  *threadData;
  ContextData *contextData = (ContextData *)contextDataPtr;
  if (!s_initCount) {
    return;
  }

  if (!contextData) {
    return;
  }

  threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return;
  }

  if (!threadData->m_contextData && !contextData->m_threadData) {
    return;
  }

  ASSERT(threadData->m_contextData == contextData);
  ASSERT(contextData->m_threadData == threadData);

  s_critsect.Enter();
  threadData->m_contextData = 0;
  contextData->m_threadData = 0;
  s_critsect.Leave();
}

void OsCallBeginTurn() {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  if (!contextData) {
    return;
  }

  ++contextData->m_turnId;
  DWORD        head = contextData->m_turnBufferTail++ & 0x3FF;
  ContextTurn &turn = contextData->m_turnBuffer[head];
  contextData->m_checksum = -1;
  turn.m_turnId = contextData->m_turnId;
  turn.m_callBufferHead = contextData->m_callBufferTail;
  if (contextData->m_turnBufferTail == contextData->m_turnBufferHead + 0x401) {
    ++contextData->m_turnBufferHead;
  }
}

DWORD OsCallEndTurn() {
  if (!s_initCount) {
    return 0;
  }
  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return 0;
  }

  ContextData *contextData = threadData->m_contextData;
  if (!contextData) {
    return 0;
  }

  return ~contextData->m_checksum;
}

void OsCallCompleteTurn() {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData || !threadData->m_contextData) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  ++contextData->m_turnIdComplete;
  ASSERT((int)(contextData->m_turnIdComplete - contextData->m_turnId) <= 0);

  ContextTurn &turn = contextData->m_turnBuffer[contextData->m_turnBufferHead & 0x3FF];
  if (turn.m_turnId == contextData->m_turnIdComplete) {
    ++contextData->m_turnBufferHead;
    if ((long)(turn.m_callBufferHead - contextData->m_callBufferHead) >= 0) {
      contextData->m_callBufferHead = turn.m_callBufferHead;
    }
  }
}

static void OsCallDumpContextData(_iobuf *file, const ContextData *contextData) {
  fprintf(file, "; Context: %s\r\n", contextData->m_title);

  DWORD              turnOffset;
  const ContextTurn *turn = 0;
  for (turnOffset = contextData->m_turnBufferHead; turnOffset != contextData->m_turnBufferTail; ++turnOffset) {
    turn = &contextData->m_turnBuffer[turnOffset & 0x3FF];
    if ((int)(turn->m_callBufferHead - contextData->m_callBufferHead) >= 0) {
      break;
    }
    turn = 0;
  }

  for (DWORD callOffset = contextData->m_callBufferHead; callOffset != contextData->m_callBufferTail; ++callOffset) {
    const ContextCall &call = contextData->m_callBuffer[callOffset & 0xFF];

    if (turn && callOffset == turn->m_callBufferHead) {
      fprintf(file, ";[TURN %05u]\r\n", turn->m_turnId);
      ++turnOffset;
      turn = turnOffset == contextData->m_turnBufferTail ? 0 : &contextData->m_turnBuffer[turnOffset & 0x3FF];
    }

    if (!(call.m_depth & 0x80)) {
      if (call.m_funcAddr & 0x80000000) {
        fprintf(file, "%*s%08x\r\n", call.m_depth, "", call.m_funcAddr & 0x7FFFFFFF);
      }
    } else {
      fprintf(file, "%*s;data = 0x%08x = %f\r\n", call.m_depth & ~0x80, "", call.m_funcAddr, *(const float *)&call.m_funcAddr);
    }
  }
  fprintf(file, "\r\n");
}

static void OsCallDumpProfileData(LPCSTR fileName, const ContextData *contextData) {
}

void OsCallDump(LPCSTR fileName) {
  s_critsect.Enter();

  if (!fileName) {
    fileName = "calldump.log";
  }

  FILE *file = fopen(fileName, "wb");
  if (file) {
    setvbuf(file, 0, _IOFBF, 0x7FFF);
    fprintf(file, ";Call Trace Log %s, %s\r\n", "Dec 11 2003", "17:58:40");

    ITERATELIST(ContextData, s_contextDataList, contextData) {
      ThreadData *threadData = contextData->m_threadData;
      if (threadData && threadData->m_threadId != GetCurrentThreadId() &&
          SuspendThread(threadData->m_threadHandle) == (DWORD)-1)
      {
        fprintf(file, ";Couldn't dump thread: %s\r\n", threadData->m_title);
      } else {
        OsCallDumpContextData(file, contextData);
        OsCallDumpProfileData(fileName, contextData);
        if (threadData && threadData->m_threadId != GetCurrentThreadId()) {
          ResumeThread(threadData->m_threadHandle);
        }
      }
    }
    fclose(file);
  }

  s_critsect.Leave();
}

int __cdecl OsCallEnter(DWORD funcAddr, DWORD retAddr) {
  if (!s_initCount) {
    return 0;
  }

  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return 0;
  }

  if (threadData->m_funcStackIndex >= 0x200) {
    return 0;
  }

  DWORD        stackIndex = threadData->m_funcStackIndex++;
  ThreadStack &stack = threadData->m_funcStack[stackIndex];
  stack.m_retAddr = retAddr;
  stack.m_funcAddr = funcAddr;

  ContextData *contextData = threadData->m_contextData;
  if (contextData) {
    stack.m_logExit = s_enable && threadData->m_enabled;
    if (stack.m_logExit) {
      DWORD        head = contextData->m_callBufferTail++ & 0xFF;
      ContextCall &call = contextData->m_callBuffer[head];
      call.m_funcAddr = funcAddr | 0x80000000;
      call.m_depth = stackIndex & 0x7F;
      if (contextData->m_callBufferTail == contextData->m_callBufferHead + 0x101) {
        ++contextData->m_callBufferHead;
      }
    }
  }
  return 1;
}

DWORD __cdecl OsCallExit() {
  if (!s_initCount) {
    return 0;
  }
  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return 0;
  }
  return threadData->m_funcStack[--threadData->m_funcStackIndex].m_retAddr;
}

void OsCallData(DWORD data) {
  if (!s_initCount) {
    return;
  }
  ThreadData *threadData = (ThreadData *)OsTlsGetValue(s_tlsIndex);
  if (!threadData) {
    return;
  }

  ContextData *contextData = threadData->m_contextData;
  DWORD        stackIndex = threadData->m_funcStackIndex;
  if (!contextData || !s_enable || !threadData->m_enabled) {
    return;
  }

  DWORD value = data;
  DWORD crc = contextData->m_checksum;
  crc = s_crcTable[(crc ^ value) & 0xFF] ^ (crc >> 8);
  value >>= 8;
  crc = s_crcTable[(crc ^ value) & 0xFF] ^ (crc >> 8);
  value >>= 8;
  crc = s_crcTable[(crc ^ value) & 0xFF] ^ (crc >> 8);
  value >>= 8;
  crc = s_crcTable[(crc ^ value) & 0xFF] ^ (crc >> 8);
  contextData->m_checksum = crc;

  DWORD        head = contextData->m_callBufferTail++ & 0xFF;
  ContextCall &call = contextData->m_callBuffer[head];
  call.m_funcAddr = data;
  call.m_depth = stackIndex | 0x80;
  if (contextData->m_callBufferTail == contextData->m_callBufferHead + 0x101) {
    ++contextData->m_callBufferHead;
  }
}

void OsCallData(float data) {
  OsCallData(*(DWORD *)&data);
}
