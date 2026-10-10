#include <Base/Base.h>

#include "Scrn.h"

#include <Base/Activity.h>
#include <Base/Handle.h>
#include <Gx/Gx.h>
#include <Gxu/IGxuFont.h>
#include <Os/OsNet.h>
#include <Os/OsTime.h>
#include <Os/W32/OSSystem.h>
#include <Os/W32/Debugging.h>
#include <Services/TextBlock.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>

#include <string.h>
static UINT memBytesComm;
static UINT memBytesAlloc;
static int  loggedFull;

#pragma pack(push, 4)
struct SimplePerfHdr {
  BYTE     signature;
  BYTE     message;
  WORD     len;
  LONGLONG timestamp;
  LONGLONG frame;
  DWORD    updateCount;
  WORD     id;
  WORD     size;
};
#pragma pack(pop)

UINT SndGetCPUPerformance();

static void PaintFunc(LPVOID, const RECTF *, const RECTF *visible, float elapsed);
static void IPerfGetPerfTimes(float *perfTime);
static void IPerfRemoteQueuePerfData(float *perfTime);
void        IPerfRemoteQueueOnRingBuffer(LPVOID dataToQueue, DWORD size);
BOOL        IPerfRemoteSpaceAvail(DWORD size);
void        IPerfRemoteInitSimpleHdr(SimplePerfHdr *h, DWORD packetSize, WORD id);
void        IPerfRemoteQueuePerfLabels();
void        IPerfRemoteQueueLogString(LPCSTR prompt);
void        IPerfResetTimePeaks();

static int s_enabled;
static struct {
  float (*func)(ACTIVITY);
  LPCSTR title;
} s_activityModes[3] = {
    {       ActivityGetTime,    "ms per frame"},
    {ActivityGetTimePercent,  "percentage CPU"},
    {      ActivityGetCalls, "calls per frame"}
};
enum {
  PERF_EVENTS = 0,
  PERF_EVENTIDLE = 1,
  PERF_EVENTPAINT = 2,
  PERF_ANIMATE = 3,
  PERF_ANIM_EVENTS = 4,
  PERF_ANIM_SEQ_END = 5,
  PERF_MODEL = 6,
  PERF_PARTICLE = 7,
  PERF_LIGHTING = 8,
  PERF_RENDER = 9,
  PERF_OBJ_MGR = 10,
  PERF_FRAMEMGR = 11,
  PERF_WORLD = 12,
  PERF_SOUND = 13,
  PERF_PERFDISPLAY = 14,
  PERF_PROG0 = 15,
  PERF_PROG1 = 16,
  PERF_PROG2 = 17,
  PERF_PROG3 = 18,
  PERF_OTHER = 19,
  PERF_MAX = 20
};

static int       s_activityMode;
static HLAYER__ *s_layer;
static void (*sTextFunc)(char *, int, LPVOID);
static LPVOID          sTextFuncParam;
static float           s_avgPerfTime[PERF_MAX];
static float           s_peakPerfTime[PERF_MAX];
static int             s_peakSustain[PERF_MAX];
static float           s_avgPerfAccum[PERF_MAX][8];
static int             s_avgPerfIndex;
static float           s_perfFontHeight;
static CGxStringBatch *s_stringBatch;

enum {
  MSG_OPEN = 0,
  MSG_DATA = 1,
  MSG_MAX = 2
};

enum {
  PROF_PERFTIME_DATA = 1,
  PROF_PERFTIME_LABELS = 2,
  PROF_LOG_STRING = 3
};

char *s_perfPrompts[PERF_MAX] = {
    "Events",
    "Event Idle",
    "Event Paint",
    "Animate",
    "Animation Event Handlers",
    "Animation Done Handlers",
    "Model",
    "Particle",
    "Lighting",
    "Render",
    "Object Manager",
    "Frame Manager",
    "World",
    "Sound",
    "Perf Display",
    "Custom 0",
    "Custom 1",
    "Custom 2",
    "Custom 3",
    "Other"
};

static PERF_REMOTE_MODE s_PerfRemoteMode;
static CSRWLock         s_ProfLock;
static SCritSect        s_ringLock;
static int              s_HasProfServer;
static int              s_HasProfConn;
static SEvent          *s_ProfRequestEnqueuedEvent;
static char            *s_ProfRingBuf;
static int              s_ProfRingBufHead;
static int              s_ProfRingBufTail;
static LONGLONG         s_ProfRingFrame;
static NETADDR          s_ProfAddr;
static SEvent          *s_ProfStartupEvent;
static SEvent          *s_ProfShutdownEvent;
static HNETCONN__      *s_ProfConn;
static SThread         *s_ProfThreadPtr;
static long             s_ProfStartup;
static char             s_hostNameList[64];

void __stdcall HookRenderBegin(LPVOID) {
  ActivityBegin(ACTIVITY_RENDER);
}

void __stdcall HookRenderEnd(LPVOID) {
  ActivityEnd(ACTIVITY_RENDER);
}

static void PaintFunc(LPVOID, const RECTF *, const RECTF *visible, float elapsed) {
  float              perfTime[PERF_MAX];
  float              perfTimeTotal;
  float              peakTimeTotal;
  UINT               fps;
  UINT               vertices;
  UINT               primitives;
  UINT               batches;
  UINT               textures;
  UINT               textureBytes;
  UINT               uploads;
  UINT               uploadBytes;
  UINT               binds;
  UINT               bindBytes;
  UINT               vertexBytes;
  UINT               indexBytes;
  LPCSTR             api;
  char               line[11][0x200];
  CGxFont           *theFont;
  CGxString         *s_stringArray[11];
  int                i;
  int                j;
  UINT               index;

  if (!s_enabled) {
    if (ScrnPerfRemoteGetMode() == PERF_REMOTE_MODE__ONPAINT) {
      IPerfGetPerfTimes(perfTime);
      IPerfRemoteQueuePerfData(perfTime);
    }

    return;
  }

  ActivityBegin(ACTIVITY_PERFDISPLAY);
  fps = GxPerfCounter(GxPerf_FrameRate);
  vertices = GxPerfCounter(GxPerf_Vertices);
  primitives = GxPerfCounter(GxPerf_Primitives);
  batches = GxPerfCounter(GxPerf_Batches);
  textures = GxPerfCounter(GxPerf_Textures);
  textureBytes = GxPerfCounter(GxPerf_TextureBytes);
  uploads = GxPerfCounter(GxPerf_TexUploads);
  uploadBytes = GxPerfCounter(GxPerf_TexUploadBytes);
  binds = GxPerfCounter(GxPerf_TexBinds);
  bindBytes = GxPerfCounter(GxPerf_TexBindBytes);
  vertexBytes = GxPerfCounter(GxPerf_VertexBytes);
  indexBytes = GxPerfCounter(GxPerf_IndexBytes);

  switch (GxDevApi()) {
    case GxApi_OpenGl:
      api = "OpenGL";
      break;
    case GxApi_Direct3d:
      api = "Direct3D";
      break;
    default:
      api = "<unknown 3D API>";
      break;
  }

  memBytesAlloc = 0;
  memBytesComm = 0;
  {
    SMEMHEAPDETAILS details;
    HSHEAP          heap;

    details.size = sizeof(details);
    heap = 0;

    while (SMemFindNextHeap(heap, &heap, &details)) {
      memBytesAlloc += details.allocatedbytes;
      memBytesComm += details.committedbytes;
    }
  }

  ActivityEnd(ACTIVITY_PERFDISPLAY);
  IPerfGetPerfTimes(perfTime);
  ActivityBegin(ACTIVITY_PERFDISPLAY);

  if (ScrnPerfRemoteGetMode() == PERF_REMOTE_MODE__ONPAINT) {
    IPerfRemoteQueuePerfData(perfTime);
  }

  perfTimeTotal = 0.0f;
  peakTimeTotal = 0.0f;
  ++s_avgPerfIndex;
  if (s_avgPerfIndex >= 8) {
    s_avgPerfIndex = 0;
  }

  for (i = 0; i < PERF_MAX; ++i) {
    if (perfTime[i] > s_peakPerfTime[i]) {
      s_peakPerfTime[i] = perfTime[i];
      s_peakSustain[i] = 64;
    } else if (!s_peakSustain[i]) {
      s_peakPerfTime[i] = perfTime[i] * 0.1f + s_peakPerfTime[i] * 0.9f;
    } else {
      --s_peakSustain[i];
    }

    s_avgPerfAccum[i][s_avgPerfIndex] = perfTime[i];
    for (j = 0; j < 8; ++j) {
      s_avgPerfTime[i] += s_avgPerfAccum[i][j];
    }

    s_avgPerfTime[i] *= 0.125f;
    perfTimeTotal += s_avgPerfTime[i];
    peakTimeTotal += s_peakPerfTime[i];
  }

  SStrPrintf(
      line[0], sizeof(line[0]), "%3u FPS, %5.1f/%5.1fMB mem,  Displaying: %s", fps, memBytesAlloc * 0.00000095367432f,
      memBytesComm * 0.00000095367432f, s_activityModes[s_activityMode].title
  );
  SStrPrintf(line[1], sizeof(line[1]), "%2u batches, %5u verts, %5u prims, %s", batches, vertices, primitives, api);
  SStrPrintf(line[2], sizeof(line[2]), "%6u VB, %6u IB", vertexBytes, indexBytes);
  SStrPrintf(
      line[3], sizeof(line[3]), "tex:%3u, %4.2fMB   ren:%3u, %3.3fMB   upd:%3u, %2.4fMB", textures,
      textureBytes * 0.00000095367432f, binds, bindBytes * 0.00000095367432f, uploads,
      uploadBytes * 0.00000095367432f
  );
  SStrPrintf(
      line[4], sizeof(line[4]), "%6.1f idle @%4.0f, %6.1f evnt @%4.0f, %6.1f snd  @%4.0f, %6.1f pnt  @%4.0f", s_avgPerfTime[PERF_EVENTIDLE], s_peakPerfTime[PERF_EVENTIDLE],
      s_avgPerfTime[PERF_EVENTS], s_peakPerfTime[PERF_EVENTS], s_avgPerfTime[PERF_SOUND], s_peakPerfTime[PERF_SOUND], s_avgPerfTime[PERF_EVENTPAINT], s_peakPerfTime[PERF_EVENTPAINT]
  );
  SStrPrintf(
      line[5], sizeof(line[5]), "%6.1f fmgr @%4.0f, %6.1f wrld @%4.0f, %6.1f lght @%4.0f, %6.1f modl @%4.0f", s_avgPerfTime[PERF_FRAMEMGR], s_peakPerfTime[PERF_FRAMEMGR],
      s_avgPerfTime[PERF_WORLD], s_peakPerfTime[PERF_WORLD], s_avgPerfTime[PERF_LIGHTING], s_peakPerfTime[PERF_LIGHTING], s_avgPerfTime[PERF_MODEL], s_peakPerfTime[PERF_MODEL]
  );
  SStrPrintf(
      line[6], sizeof(line[6]), "%6.1f part @%4.0f, %6.1f rndr @%4.0f, %6.1f omgr @%4.0f", s_avgPerfTime[PERF_PARTICLE], s_peakPerfTime[PERF_PARTICLE], s_avgPerfTime[PERF_RENDER],
      s_peakPerfTime[PERF_RENDER], s_avgPerfTime[PERF_OBJ_MGR], s_peakPerfTime[PERF_OBJ_MGR]
  );
  SStrPrintf(
      line[7], sizeof(line[7]), "%6.1f anim @%4.0f, %6.1f aevt @%4.0f, %6.1f aseq @%4.0f", s_avgPerfTime[PERF_ANIMATE], s_peakPerfTime[PERF_ANIMATE], s_avgPerfTime[PERF_ANIM_EVENTS],
      s_peakPerfTime[PERF_ANIM_EVENTS], s_avgPerfTime[PERF_ANIM_SEQ_END], s_peakPerfTime[PERF_ANIM_SEQ_END]
  );
  SStrPrintf(
      line[8], sizeof(line[8]), "%6.1f p0   @%4.0f, %6.1f p1   @%4.0f, %6.1f p2   @%4.0f, %6.1f p3   @%4.0f", s_avgPerfTime[PERF_PROG0], s_peakPerfTime[PERF_PROG0],
      s_avgPerfTime[PERF_PROG1], s_peakPerfTime[PERF_PROG1], s_avgPerfTime[PERF_PROG2], s_peakPerfTime[PERF_PROG2], s_avgPerfTime[PERF_PROG3], s_peakPerfTime[PERF_PROG3]
  );
  SStrPrintf(
      line[9], sizeof(line[9]), "%6.1f perf @%4.0f, %6.1f othr @%4.0f, %6.1f totl @%4.0f,   miles %d%%", s_avgPerfTime[PERF_PERFDISPLAY], s_peakPerfTime[PERF_PERFDISPLAY],
      s_avgPerfTime[PERF_OTHER], s_peakPerfTime[PERF_OTHER], perfTimeTotal, peakTimeTotal, SndGetCPUPerformance()
  );

  line[10][0] = 0;
  if (sTextFunc) {
    sTextFunc(line[10], sizeof(line[10]), sTextFuncParam);
  }

  theFont = TextBlockGetFontPtr(ScrnGetStockFont(STOCK_PERFFONT));
  ASSERT(theFont);

  for (index = 0; index < 11; ++index) {
    NTempest::C3Vector  pos(visible->left, visible->top - (index + 2) * 0.025f, 0.0f);
    NTempest::CImVector color(0xFFFFFF00);
    GxuFontCreateString(
        theFont, line[index], 0.0f, pos, 1.0f, s_perfFontHeight, 0.0f, s_stringArray[index], GxVJ_Middle, GxHJ_Left, 0x54, color, -0.45f
    );
    GxuFontAddToBatch(s_stringBatch, s_stringArray[index]);
  }

  GxuFontRenderBatch(s_stringBatch);
  for (index = 0; index < 11; ++index) {
    GxuFontDestroyString(s_stringArray[index]);
    s_stringArray[index] = 0;
  }

  ActivityEnd(ACTIVITY_PERFDISPLAY);
}

void IPerfDestroy() {
  if (s_stringBatch) {
    GxuFontDestroyBatch(s_stringBatch);
  }

  s_stringBatch = 0;
  HandleClose((HOBJECT)s_layer);
}

static void IPerfGetPerfTimes(float *perfTime) {
  float (*activityGet)(ACTIVITY) = s_activityModes[s_activityMode].func;

  perfTime[PERF_ANIMATE] = activityGet(ACTIVITY_ANIMATE);
  perfTime[PERF_ANIM_EVENTS] = activityGet(ACTIVITY_ANIMEVENTS);
  perfTime[PERF_ANIM_SEQ_END] = activityGet(ACTIVITY_ANIMSEQEND);
  perfTime[PERF_MODEL] = activityGet(ACTIVITY_MODEL);
  perfTime[PERF_PARTICLE] = activityGet(ACTIVITY_PARTICLE);
  perfTime[PERF_LIGHTING] = activityGet(ACTIVITY_LIGHTING);
  perfTime[PERF_RENDER] = activityGet(ACTIVITY_RENDER);
  perfTime[PERF_EVENTS] = activityGet(ACTIVITY_EVENTPOLL) + activityGet(ACTIVITY_EVENTNET) + activityGet(ACTIVITY_EVENTMOUSE) +
                activityGet(ACTIVITY_EVENTKEYANDCHAR) + activityGet(ACTIVITY_EVENTHANDLERS);
  perfTime[PERF_EVENTIDLE] = activityGet(ACTIVITY_EVENTIDLE);
  perfTime[PERF_EVENTPAINT] = activityGet(ACTIVITY_EVENTPAINT);
  perfTime[PERF_OBJ_MGR] = activityGet(ACTIVITY_OBJMGR);
  perfTime[PERF_FRAMEMGR] = activityGet(ACTIVITY_FRAMEMANAGER);
  perfTime[PERF_SOUND] = activityGet(ACTIVITY_SOUND);
  perfTime[PERF_WORLD] = activityGet(ACTIVITY_WORLD);
  perfTime[PERF_PERFDISPLAY] = activityGet(ACTIVITY_PERFDISPLAY);
  perfTime[PERF_PROG0] = activityGet(ACTIVITY_PROG0);
  perfTime[PERF_PROG1] = activityGet(ACTIVITY_PROG1);
  perfTime[PERF_PROG2] = activityGet(ACTIVITY_PROG2);
  perfTime[PERF_PROG3] = activityGet(ACTIVITY_PROG3);
  perfTime[PERF_OTHER] = activityGet(ACTIVITY_OTHER);
  ActivityResetTimes();
}

void IPerfInitialize() {
  if (s_stringBatch) {
    GxuFontDestroyBatch(s_stringBatch);
  }

  s_stringBatch = GxuFontCreateBatch();
  ScrnLayerCreate(0, 2.0f, 3, 0, PaintFunc, &s_layer);
  s_perfFontHeight = ScrnGetStockFontHeight(STOCK_PERFFONT);
}

void IPerfResetTimePeaks() {
  s_avgPerfIndex = 0;
  memset(s_avgPerfAccum, 0, sizeof(s_avgPerfAccum));
  memset(s_avgPerfTime, 0, sizeof(s_avgPerfTime));
}

void __stdcall IPerfHostAddrProc(const NETADDR *addrs, DWORD addrCount, LPVOID user) {
  if (!addrCount) {
    s_ProfShutdownEvent->Set();
    return;
  }
  s_ProfAddr = addrs[0];
  s_ProfStartupEvent->Set();
}

int __stdcall
ChatEventProc(HNETCONN__ *conn, const NETCONNADDR *conAddr, NETNOTE note, LPVOID user, LPCVOID data, DWORD bytes, DWORD *bytesProcessed) {
  BOOL result = 0;

  switch (note) {
    case NETNOTE_CONNECT:
      s_ProfLock.Enter(1);
      s_ProfConn = OsNetConnCopyHandle(conn);
      s_ProfLock.Leave(1);
      s_ProfStartupEvent->Set();
      result = 1;
      s_HasProfConn = 1;
      break;
    case NETNOTE_DISCONNECT: {
      if (s_ProfShutdownEvent) {
        s_ProfShutdownEvent->Set();
      }
      s_ProfLock.Enter(1);
      HNETCONN__ *old = (HNETCONN__ *)SInterlockedExchange((long *)&s_ProfConn, 0);
      s_ProfLock.Leave(1);
      if (old) {
        OsNetConnFreeHandle(old);
      }
      result = 1;
      s_HasProfConn = 0;
      break;
    }
    case NETNOTE_CANTCONNECT:
      s_ProfShutdownEvent->Set();
      break;
  }
  return result;
}

BOOL IPerfRemoteLoggingIni() {
  SSyncObject *waitObjectPtrs[2] = {s_ProfStartupEvent, s_ProfShutdownEvent};
  s_ProfStartupEvent->Reset();
  if (!OsNetGetHostAddrs(s_hostNameList, 0x17EB, IPerfHostAddrProc, 0)) {
    return 0;
  }
  if (WaitMultiplePtr(2, waitObjectPtrs, 0, INFINITE) != 0) {
    return 0;
  }

  WORD  port;
  DWORD address = OsNetAddrGetAddress(&s_ProfAddr, &port);
  BYTE  connByte = 0x8E;
  s_ProfStartupEvent->Reset();
  OsTcpConnect(address, port, ChatEventProc, 0, &connByte, 1);
  if (WaitMultiplePtr(2, waitObjectPtrs, 0, INFINITE) != 0) {
    return 0;
  }

  s_HasProfServer = 1;
  char  username[16];
  char  computer[32];
  DWORD usernameSize = sizeof(username);
  DWORD computerSize = sizeof(computer);
  OsGetUserName(username, &usernameSize);
  OsGetComputerName(computer, &computerSize);
  struct {
    BYTE signature;
    BYTE message;
    WORD len;
    char strings[256];
  } msg;
  UINT usernameLength = SStrLen(username);
  UINT computerLength = SStrLen(computer);
  msg.signature = 0xAB;
  msg.message = 0;
  SStrCopy(msg.strings, username, 0x7FFFFFFF);
  SStrCopy(msg.strings + usernameLength + 1, computer, 0x7FFFFFFF);
  UINT messageBytes = usernameLength + computerLength + 6;
  msg.len = messageBytes;
  s_ProfLock.Enter(0);
  if (s_ProfConn) {
    OsTcpConnSend(s_ProfConn, &msg, messageBytes);
  }
  s_ProfLock.Leave(0);
  IPerfRemoteQueuePerfLabels();
  return 1;
}

void IPerfGetFromBuffer(char *&buf1, long &len1, char *&buf2, long &len2) {
  if (0x10000 - s_ProfRingBufTail < 4) {
    s_ProfRingBufTail = 0;
  }
  buf1 = s_ProfRingBuf + s_ProfRingBufTail;
  UINT recordBytes = *(UINT *)buf1;
  UINT remain = 0x10000 - s_ProfRingBufTail;
  buf1 += 4;
  len1 = recordBytes - 4;
  if (recordBytes <= remain) {
    buf2 = 0;
    len2 = 0;
  } else if (remain <= 4) {
    ASSERT(remain == 4);
    buf1 = s_ProfRingBuf;
    buf2 = 0;
    len2 = 0;
  } else {
    len1 = remain - 4;
    buf2 = s_ProfRingBuf;
    len2 = recordBytes - remain;
  }
}

void IPerfRemoteSendQueued() {
  while (s_ProfRingBufHead != s_ProfRingBufTail) {
    char *buf1;
    char *buf2;
    long  len1;
    long  len2;
    IPerfGetFromBuffer(buf1, len1, buf2, len2);
    s_ProfLock.Enter(0);
    if (s_ProfConn) {
      OsTcpConnSend(s_ProfConn, buf1, len1);
    }
    s_ProfLock.Leave(0);
    if (buf2) {
      ASSERT(len2 > 0);
      s_ProfLock.Enter(0);
      if (s_ProfConn) {
        OsTcpConnSend(s_ProfConn, buf2, len2);
      }
      s_ProfLock.Leave(0);
    }
    s_ProfRingBufTail = (s_ProfRingBufTail + len1 + len2 + 4) % 0x10000;
  }
}

UINT __stdcall IPerfRemoteLoggingThread(LPVOID) {
  if (IPerfRemoteLoggingIni()) {
    SSyncObject *waitObjectPtrs[2] = {s_ProfShutdownEvent, s_ProfRequestEnqueuedEvent};
    DWORD        waitResult;
    while ((waitResult = WaitMultiplePtr(2, waitObjectPtrs, 0, INFINITE)) != 0) {
      if (waitResult != 1) {
        break;
      }
      IPerfRemoteSendQueued();
      s_ProfRequestEnqueuedEvent->Reset();
    }
    s_ProfLock.Enter(1);
    HNETCONN__ *old = (HNETCONN__ *)SInterlockedExchange((long *)&s_ProfConn, 0);
    s_ProfLock.Leave(1);
    if (old) {
      OsNetConnFreeHandle(old);
    }
    OsOutputDebugString("Exiting perf-server thread");
  }
  return 0;
}

BOOL IPerfRemoteSpaceAvail(DWORD size) {
  DWORD space;

  if (s_ProfRingBufTail > s_ProfRingBufHead) {
    space = s_ProfRingBufTail - s_ProfRingBufHead;
  } else {
    space = s_ProfRingBufTail - s_ProfRingBufHead + 0x10000;
  }

  if (size + 3 >= space) {
    if (!loggedFull) {
      loggedFull = 1;
    }
    return 0;
  }

  return 1;
}

void IPerfRemoteInitSimpleHdr(SimplePerfHdr *h, DWORD packetSize, WORD id) {
  float asyncTimeClocks;

  ASSERT(packetSize < 65532);

  h->signature = 0xAB;
  h->message = 1;
  h->len = packetSize;
  asyncTimeClocks = OsGetAsyncTimeClocks();
  h->timestamp = OsGetAsyncClocksDivisor() * asyncTimeClocks * 1000000.0f;
  h->frame = s_ProfRingFrame++;
  h->updateCount = 1;
  h->id = id;
}

void IPerfRemoteQueueOnRingBuffer(LPVOID dataToQueue, DWORD size) {
  char *dest;
  DWORD bytes;

  s_ringLock.Enter();

  if (0x10000 - s_ProfRingBufHead < 4) {
    OsGetAsyncTimeMs();
    s_ProfRingBufHead = 0;
  }

  dest = s_ProfRingBuf + s_ProfRingBufHead;
  bytes = 0x10000 - s_ProfRingBufHead;
  if (bytes >= size) {
    bytes = size;
  }

  memcpy(dest, dataToQueue, bytes);

  if (bytes < size) {
    memcpy(s_ProfRingBuf, (BYTE *)dataToQueue + bytes, size - bytes);
    OsGetAsyncTimeMs();
  }

  s_ProfRingBufHead = (s_ProfRingBufHead + size) & 0xFFFF;
  s_ProfRequestEnqueuedEvent->Set();
  s_ringLock.Leave();
}

static void IPerfRemoteQueuePerfData(float *perfTime) {
  struct {
    DWORD         msgSize;
    SimplePerfHdr h;
    float         perfTime[PERF_MAX];
  } queuePerfBuf;

  if (s_HasProfServer && s_HasProfConn && IPerfRemoteSpaceAvail(sizeof(queuePerfBuf))) {
    queuePerfBuf.msgSize = sizeof(queuePerfBuf);
    IPerfRemoteInitSimpleHdr(&queuePerfBuf.h, sizeof(queuePerfBuf) - 4, 1);
    queuePerfBuf.h.size = sizeof(queuePerfBuf.perfTime);
    memcpy(queuePerfBuf.perfTime, perfTime, sizeof(queuePerfBuf.perfTime));
    IPerfRemoteQueueOnRingBuffer(&queuePerfBuf, sizeof(queuePerfBuf));
  }
}

void IPerfRemoteQueuePerfLabels() {
  struct {
    DWORD         msgSize;
    SimplePerfHdr h;
    char          labels[0x400];
  } queueLabelsBuf;
  UINT  labelBytes;
  UINT  packetBytes;
  int   i;
  char *label;

  if (!s_HasProfServer || !s_HasProfConn) {
    return;
  }

  labelBytes = 0;
  for (i = 0; i < PERF_MAX; ++i) {
    labelBytes += SStrLen(s_perfPrompts[i]) + 1;
  }

  packetBytes = labelBytes + 32;
  if (!IPerfRemoteSpaceAvail(packetBytes)) {
    return;
  }

  queueLabelsBuf.msgSize = (WORD)packetBytes;
  IPerfRemoteInitSimpleHdr(&queueLabelsBuf.h, packetBytes - 4, 2);
  queueLabelsBuf.h.size = labelBytes;
  label = queueLabelsBuf.labels;

  for (i = 0; i < PERF_MAX; ++i) {
    label += SStrCopy(label, s_perfPrompts[i], 0x7FFFFFFF) + 1;
  }

  IPerfRemoteQueueOnRingBuffer(&queueLabelsBuf, packetBytes);
}

void IPerfRemoteQueueLogString(LPCSTR prompt) {
  struct {
    DWORD         msgSize;
    SimplePerfHdr h;
    char          prompt[0x100];
  } queuePromptBuf;
  UINT packetBytes;

  if (!s_HasProfServer || !s_HasProfConn) {
    return;
  }

  packetBytes = SStrLen(prompt) + 33;
  if (!IPerfRemoteSpaceAvail(packetBytes)) {
    return;
  }

  queuePromptBuf.msgSize = packetBytes;
  IPerfRemoteInitSimpleHdr(&queuePromptBuf.h, packetBytes - 4, 3);
  queuePromptBuf.h.size = SStrLen(prompt) + 1;
  SStrCopy(queuePromptBuf.prompt, prompt, sizeof(queuePromptBuf.prompt));
  IPerfRemoteQueueOnRingBuffer(&queuePromptBuf, packetBytes);
}

BOOL IPerfRemoteLoggingStartup() {
  if (SInterlockedIncrement(&s_ProfStartup) <= 1) {
    if (!SRegLoadString("Internal", "Engine PerfServer", 0, s_hostNameList, sizeof(s_hostNameList))) {
      s_hostNameList[0] = 0;
      SRegSaveString("Internal", "Engine PerfServer", 0, s_hostNameList);
    }
    if (!s_hostNameList[0]) {
      SInterlockedDecrement(&s_ProfStartup);
      return 0;
    }
    OsNetInitialize(0, 3);
    s_ProfRingBuf = (char *)SMemAlloc(0x10000, __FILE__, __LINE__, 0);
    s_ProfRingBufTail = s_ProfRingBufHead = 0;
    s_ProfStartupEvent = NEW(SEvent)(1, 0);
    s_ProfShutdownEvent = NEW(SEvent)(1, 0);
    s_ProfRequestEnqueuedEvent = NEW(SEvent)(1, 0);
    s_ProfThreadPtr = NEW(SThread)();
    SThread::Create(IPerfRemoteLoggingThread, 0, *s_ProfThreadPtr, "PerfLog");
  }
  return 1;
}

int IPerfRemoteLoggingShutdown() {
  if (s_ProfStartup && !SInterlockedDecrement(&s_ProfStartup)) {
    s_HasProfServer = 0;
    s_HasProfConn = 0;
    OsOutputDebugString("Closing perf-server connection");
    OsNetDestroy(3);
    ASSERT(s_ProfShutdownEvent);
    s_ProfShutdownEvent->Set();
    WaitMultiplePtr(1, (SSyncObject **)&s_ProfThreadPtr, 1, INFINITE);
    DEL(s_ProfThreadPtr);
    s_ProfThreadPtr = 0;
    SMemFree(s_ProfRingBuf, __FILE__, __LINE__, 0);
    s_ProfRingBuf = 0;
    DEL(s_ProfStartupEvent);
    DEL(s_ProfShutdownEvent);
    DEL(s_ProfRequestEnqueuedEvent);
    s_ProfStartupEvent = 0;
    s_ProfShutdownEvent = 0;
    s_ProfRequestEnqueuedEvent = 0;
  }
  return 1;
}

void ScrnPerfEnable(int enable) {
  s_enabled = enable;
}

int ScrnPerfIsEnabled() {
  return s_enabled;
}

int ScrnPerfRemoteShutdown() {
  return IPerfRemoteLoggingShutdown();
}

int ScrnPerfRemoteStartup() {
  return IPerfRemoteLoggingStartup();
}

void ScrnPerfRemoteSetMode(PERF_REMOTE_MODE mode) {
  if (mode >= PERF_REMOTE_MODE_MAX) {
    mode = PERF_REMOTE_MODE__ONPAINT;
  }
  s_PerfRemoteMode = mode;
}

PERF_REMOTE_MODE ScrnPerfRemoteGetMode() {
  return s_PerfRemoteMode;
}

void ScrnPerfRemoteLogString(LPCSTR prompt) {
  IPerfRemoteQueueLogString(prompt);
}

void ScrnPerfRemoteLogPerfCounters() {
  float perfTime[PERF_MAX];

  IPerfGetPerfTimes(perfTime);
  IPerfRemoteQueuePerfData(perfTime);
}

void ScrnPerfSetTextFunction(void (*inFunc)(char *, int, LPVOID), LPVOID inParam) {
  sTextFunc = inFunc;
  sTextFuncParam = inParam;
}

void ScrnPerfToggleDisplayedValues() {
  ++s_activityMode;
  if ((UINT)s_activityMode >= 3) {
    s_activityMode = 0;
  }

  IPerfResetTimePeaks();
}

void ScrnPerfResetTimePeaks() {
  IPerfResetTimePeaks();
}
