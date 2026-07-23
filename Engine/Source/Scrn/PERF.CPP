#include "Scrn.h"

#include <Base/Activity.h>
#include <Base/Handle.h>
#include <Gx/Gx.h>
#include <Gxu/IGxuFont.h>
#include <Os/OsNet.h>
#include <Os/OsTime.h>
#include <Services/TextBlock.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>

#include <string.h>
static unsigned int memBytesComm;
static unsigned int memBytesAlloc;
static int loggedFull;
const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

#pragma pack(push, 4)
struct SimplePerfHdr {
  unsigned char signature;
  unsigned char message;
  unsigned short len;
  __int64 timestamp;
  __int64 frame;
  unsigned int updateCount;
  unsigned short id;
  unsigned short size;
};
#pragma pack(pop)

unsigned int __fastcall SndGetCPUPerformance();

static void __fastcall PaintFunc(void *,
                                 const RECTF *,
                                 const RECTF *visible,
                                 float elapsed);
static void __fastcall IPerfGetPerfTimes(float *perfTime);
static void __fastcall IPerfRemoteQueuePerfData(float *perfTime);
void __fastcall IPerfRemoteQueueOnRingBuffer(void *dataToQueue,
                                             unsigned long size);
int __fastcall IPerfRemoteSpaceAvail(unsigned long size);
void __fastcall IPerfRemoteInitSimpleHdr(SimplePerfHdr *h,
                                         unsigned long packetSize,
                                         unsigned short id);
void __fastcall IPerfRemoteQueuePerfLabels();
void __fastcall IPerfRemoteQueueLogString(const char *prompt);
void __fastcall IPerfResetTimePeaks();

static int s_enabled;
static const struct {
  float (__fastcall *func)(ACTIVITY);
  const char *title;
} s_activityModes[3] = {
  {ActivityGetTime, "ms per frame"},
  {ActivityGetTimePercent, "percentage CPU"},
  {ActivityGetCalls, "calls per frame"}
};
static int s_activityMode;
static HLAYER__ *s_layer;
static void (__fastcall *sTextFunc)(char *, int, void *);
static void *sTextFuncParam;
static float s_avgPerfTime[20];
static float s_peakPerfTime[20];
static int s_peakSustain[20];
static float s_avgPerfAccum[20][8];
static int s_avgPerfIndex;
static float s_perfFontHeight;
static CGxStringBatch *s_stringBatch;

static char *s_perfPrompts[20] = {
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
static CSRWLock s_ProfLock;
static SCritSect s_ringLock;
static int s_HasProfServer;
static int s_HasProfConn;
static SEvent *s_ProfRequestEnqueuedEvent;
static char *s_ProfRingBuf;
static int s_ProfRingBufHead;
static int s_ProfRingBufTail;
static __int64 s_ProfRingFrame;

void __stdcall HookRenderBegin(void *__formal) {
  ActivityBegin(ACTIVITY_RENDER);
}

void __stdcall HookRenderEnd(void *__formal) {
  ActivityEnd(ACTIVITY_RENDER);
}

static void __fastcall PaintFunc(void *,
                                 const RECTF *,
                                 const RECTF *visible,
                                 float elapsed) {
  float perfTime[20];
  float perfTimeTotal;
  float peakTimeTotal;
  unsigned int fps;
  unsigned int vertices;
  unsigned int primitives;
  unsigned int batches;
  unsigned int textures;
  unsigned int textureBytes;
  unsigned int uploads;
  unsigned int uploadBytes;
  unsigned int binds;
  unsigned int bindBytes;
  unsigned int vertexBytes;
  unsigned int indexBytes;
  EGxApi gxApi;
  const char *api;
  SMEMHEAPDETAILS details;
  HSHEAP heap;
  char line[11][0x200];
  CGxFont *theFont;
  CGxString *s_stringArray[11];
  NTempest::C3Vector pos;
  unsigned int i;
  unsigned int j;

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

  gxApi = GxDevApi();
  if (gxApi == GxApi_OpenGl) {
    api = "OpenGL";
  } else if (gxApi == GxApi_Direct3d) {
    api = "Direct3D";
  } else {
    api = "<unknown 3D API>";
  }

  memBytesAlloc = 0;
  memBytesComm = 0;
  details.size = sizeof(details);
  heap = 0;

  while (SMemFindNextHeap(heap, &heap, &details)) {
    memBytesComm += details.committedbytes;
    memBytesAlloc += details.allocatedbytes;
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

  for (i = 0; i < 20; ++i) {
    float accumulated;

    if (perfTime[i] > s_peakPerfTime[i]) {
      s_peakPerfTime[i] = perfTime[i];
      s_peakSustain[i] = 64;
    } else if (s_peakSustain[i]) {
      --s_peakSustain[i];
    } else {
      s_peakPerfTime[i] =
          perfTime[i] * 0.1f + s_peakPerfTime[i] * 0.9f;
    }

    s_avgPerfAccum[i][s_avgPerfIndex] = perfTime[i];
    accumulated = s_avgPerfTime[i];
    for (j = 0; j < 8; ++j) {
      accumulated += s_avgPerfAccum[i][j];
    }

    s_avgPerfTime[i] = accumulated;
    s_avgPerfTime[i] *= 0.125f;
    perfTimeTotal += s_avgPerfTime[i];
    peakTimeTotal += s_peakPerfTime[i];
  }

  SStrPrintf(line[0],
             sizeof(line[0]),
             "%3u FPS, %5.1f/%5.1fMB mem,  Displaying: %s",
             fps,
             static_cast<double>(memBytesAlloc) * 0.00000095367432,
             static_cast<double>(memBytesComm) * 0.00000095367432,
             s_activityModes[s_activityMode].title);
  SStrPrintf(line[1],
             sizeof(line[1]),
             "%2u batches, %5u verts, %5u prims, %s",
             batches,
             vertices,
             primitives,
             api);
  SStrPrintf(line[2],
             sizeof(line[2]),
             "%6u VB, %6u IB",
             vertexBytes,
             indexBytes);
  SStrPrintf(line[3],
             sizeof(line[3]),
             "tex:%3u, %4.2fMB   ren:%3u, %3.3fMB   upd:%3u, %2.4fMB",
             textures,
             static_cast<double>(textureBytes) * 0.00000095367432,
             binds,
             static_cast<double>(bindBytes) * 0.00000095367432,
             uploads,
             static_cast<double>(uploadBytes) * 0.00000095367432);
  SStrPrintf(line[4],
             sizeof(line[4]),
             "%6.1f idle @%4.0f, %6.1f evnt @%4.0f, %6.1f snd  @%4.0f, %6.1f pnt  @%4.0f",
             s_avgPerfTime[1], s_peakPerfTime[1],
             s_avgPerfTime[0], s_peakPerfTime[0],
             s_avgPerfTime[13], s_peakPerfTime[13],
             s_avgPerfTime[2], s_peakPerfTime[2]);
  SStrPrintf(line[5],
             sizeof(line[5]),
             "%6.1f fmgr @%4.0f, %6.1f wrld @%4.0f, %6.1f lght @%4.0f, %6.1f modl @%4.0f",
             s_avgPerfTime[11], s_peakPerfTime[11],
             s_avgPerfTime[12], s_peakPerfTime[12],
             s_avgPerfTime[8], s_peakPerfTime[8],
             s_avgPerfTime[6], s_peakPerfTime[6]);
  SStrPrintf(line[6],
             sizeof(line[6]),
             "%6.1f part @%4.0f, %6.1f rndr @%4.0f, %6.1f omgr @%4.0f",
             s_avgPerfTime[7], s_peakPerfTime[7],
             s_avgPerfTime[9], s_peakPerfTime[9],
             s_avgPerfTime[10], s_peakPerfTime[10]);
  SStrPrintf(line[7],
             sizeof(line[7]),
             "%6.1f anim @%4.0f, %6.1f aevt @%4.0f, %6.1f aseq @%4.0f",
             s_avgPerfTime[3], s_peakPerfTime[3],
             s_avgPerfTime[4], s_peakPerfTime[4],
             s_avgPerfTime[5], s_peakPerfTime[5]);
  SStrPrintf(line[8],
             sizeof(line[8]),
             "%6.1f p0   @%4.0f, %6.1f p1   @%4.0f, %6.1f p2   @%4.0f, %6.1f p3   @%4.0f",
             s_avgPerfTime[15], s_peakPerfTime[15],
             s_avgPerfTime[16], s_peakPerfTime[16],
             s_avgPerfTime[17], s_peakPerfTime[17],
             s_avgPerfTime[18], s_peakPerfTime[18]);
  SStrPrintf(line[9],
             sizeof(line[9]),
             "%6.1f perf @%4.0f, %6.1f othr @%4.0f, %6.1f totl @%4.0f,   miles %d%%",
             s_avgPerfTime[14], s_peakPerfTime[14],
             s_avgPerfTime[19], s_peakPerfTime[19],
             perfTimeTotal, peakTimeTotal,
             SndGetCPUPerformance());

  line[10][0] = 0;
  if (sTextFunc) {
    sTextFunc(line[10], sizeof(line[10]), sTextFuncParam);
  }

  theFont = TextBlockGetFontPtr(ScrnGetStockFont(STOCK_PERFFONT));
  ASSERT(theFont);

  for (i = 0; i < 11; ++i) {
    pos.x = visible->left;
    pos.z = 0.0f;
    NTempest::CImVector color(0xFFFFFF00);
    pos.y = visible->top - (i + 2) * 0.025f;
    GxuFontCreateString(theFont,
                        line[i],
                        0.0f,
                        pos,
                        1.0f,
                        s_perfFontHeight,
                        0.0f,
                        s_stringArray[i],
                        GxVJ_Middle,
                        GxHJ_Left,
                        0x54,
                        color,
                        -0.45f);
    GxuFontAddToBatch(s_stringBatch, s_stringArray[i]);
  }

  GxuFontRenderBatch(s_stringBatch);
  for (i = 0; i < 11; ++i) {
    GxuFontDestroyString(s_stringArray[i]);
    s_stringArray[i] = 0;
  }

  ActivityEnd(ACTIVITY_PERFDISPLAY);
}

void __fastcall IPerfDestroy() {
  if (s_stringBatch) {
    GxuFontDestroyBatch(s_stringBatch);
  }

  s_stringBatch = 0;
  HandleClose(reinterpret_cast<HOBJECT>(s_layer));
}

static void __fastcall IPerfGetPerfTimes(float *perfTime) {
  float (__fastcall *activityGet)(ACTIVITY) =
      s_activityModes[s_activityMode].func;

  perfTime[3] = activityGet(ACTIVITY_ANIMATE);
  perfTime[4] = activityGet(ACTIVITY_ANIMEVENTS);
  perfTime[5] = activityGet(ACTIVITY_ANIMSEQEND);
  perfTime[6] = activityGet(ACTIVITY_MODEL);
  perfTime[7] = activityGet(ACTIVITY_PARTICLE);
  perfTime[8] = activityGet(ACTIVITY_LIGHTING);
  perfTime[9] = activityGet(ACTIVITY_RENDER);
  perfTime[0] = activityGet(ACTIVITY_EVENTPOLL);
  perfTime[0] += activityGet(ACTIVITY_EVENTNET);
  perfTime[0] += activityGet(ACTIVITY_EVENTMOUSE);
  perfTime[0] += activityGet(ACTIVITY_EVENTKEYANDCHAR);
  perfTime[0] += activityGet(ACTIVITY_EVENTHANDLERS);
  perfTime[1] = activityGet(ACTIVITY_EVENTIDLE);
  perfTime[2] = activityGet(ACTIVITY_EVENTPAINT);
  perfTime[10] = activityGet(ACTIVITY_OBJMGR);
  perfTime[11] = activityGet(ACTIVITY_FRAMEMANAGER);
  perfTime[13] = activityGet(ACTIVITY_SOUND);
  perfTime[12] = activityGet(ACTIVITY_WORLD);
  perfTime[14] = activityGet(ACTIVITY_PERFDISPLAY);
  perfTime[15] = activityGet(ACTIVITY_PROG0);
  perfTime[16] = activityGet(ACTIVITY_PROG1);
  perfTime[17] = activityGet(ACTIVITY_PROG2);
  perfTime[18] = activityGet(ACTIVITY_PROG3);
  perfTime[19] = activityGet(ACTIVITY_OTHER);
  ActivityResetTimes();
}

void __fastcall IPerfInitialize() {
  if (s_stringBatch) {
    GxuFontDestroyBatch(s_stringBatch);
  }

  s_stringBatch = GxuFontCreateBatch();
  ScrnLayerCreate(0, 2.0f, 3, 0, PaintFunc, &s_layer);
  s_perfFontHeight = ScrnGetStockFontHeight(STOCK_PERFFONT);
}

void __fastcall IPerfResetTimePeaks() {
  memset(s_avgPerfAccum, 0, sizeof(s_avgPerfAccum));
  memset(s_avgPerfTime, 0, sizeof(s_avgPerfTime));
  s_avgPerfIndex = 0;
}

void __stdcall IPerfHostAddrProc(const NETADDR* addrs, unsigned long addrCount, void* user) {
    // TODO: implement
}

int __stdcall ChatEventProc(HNETCONN__* conn, const NETCONNADDR* conAddr, NETNOTE note, void* user, const void* data, unsigned long bytes, unsigned long* bytesProcessed) {
    // TODO: implement
    return 0;
}

int __fastcall IPerfRemoteLoggingIni() {
    // TODO: implement
    return 0;
}

void __fastcall IPerfGetFromBuffer(char*& buf1, long& len1, char*& buf2, long& len2) {
    // TODO: implement
}

void __fastcall IPerfRemoteSendQueued() {
    // TODO: implement
}

unsigned int __stdcall IPerfRemoteLoggingThread(void*) {
    // TODO: implement
    return 0;
}

int __fastcall IPerfRemoteSpaceAvail(unsigned long size) {
  unsigned long space;

  if (s_ProfRingBufTail > s_ProfRingBufHead) {
    space = s_ProfRingBufTail - s_ProfRingBufHead;
  } else {
    space = s_ProfRingBufTail - s_ProfRingBufHead + 0x10000;
  }

  if (size + 3 < space) {
    return 1;
  }

  if (!loggedFull) {
    loggedFull = 1;
  }

  return 0;
}

void __fastcall IPerfRemoteInitSimpleHdr(SimplePerfHdr *h,
                                         unsigned long packetSize,
                                         unsigned short id) {
  float asyncTimeClocks;

  ASSERT(packetSize < 65532);

  h->signature = 0xAB;
  h->message = 1;
  h->len = static_cast<unsigned short>(packetSize);
  asyncTimeClocks = static_cast<float>(OsGetAsyncTimeClocks());
  h->timestamp = static_cast<__int64>(
      OsGetAsyncClocksDivisor() * asyncTimeClocks * 1000000.0f);
  h->frame = s_ProfRingFrame++;
  h->updateCount = 1;
  h->id = id;
}

void __fastcall IPerfRemoteQueueOnRingBuffer(void *dataToQueue,
                                             unsigned long size) {
  unsigned long head;
  unsigned long bytes;

  s_ringLock.Enter();
  head = s_ProfRingBufHead;

  if (0x10000 - head < 4) {
    OsGetAsyncTimeMs();
    head = 0;
    s_ProfRingBufHead = 0;
  }

  bytes = 0x10000 - head;
  if (bytes >= size) {
    bytes = size;
  }

  memcpy(s_ProfRingBuf + head, dataToQueue, bytes);

  if (bytes < size) {
    memcpy(s_ProfRingBuf,
           static_cast<unsigned char *>(dataToQueue) + bytes,
           size - bytes);
    OsGetAsyncTimeMs();
  }

  s_ProfRingBufHead = (s_ProfRingBufHead + size) & 0xFFFF;
  s_ProfRequestEnqueuedEvent->Set();
  s_ringLock.Leave();
}

static void __fastcall IPerfRemoteQueuePerfData(float *perfTime) {
  struct {
    unsigned long msgSize;
    SimplePerfHdr h;
    float perfTime[20];
  } queuePerfBuf;

  if (s_HasProfServer && s_HasProfConn &&
      IPerfRemoteSpaceAvail(sizeof(queuePerfBuf))) {
    queuePerfBuf.msgSize = sizeof(queuePerfBuf);
    IPerfRemoteInitSimpleHdr(&queuePerfBuf.h,
                             sizeof(queuePerfBuf) - 4,
                             1);
    queuePerfBuf.h.size = sizeof(queuePerfBuf.perfTime);
    memcpy(queuePerfBuf.perfTime,
           perfTime,
           sizeof(queuePerfBuf.perfTime));
    IPerfRemoteQueueOnRingBuffer(&queuePerfBuf, sizeof(queuePerfBuf));
  }
}

void __fastcall IPerfRemoteQueuePerfLabels() {
  struct {
    unsigned long msgSize;
    SimplePerfHdr h;
    char labels[0x400];
  } queueLabelsBuf;
  unsigned int labelBytes;
  unsigned int packetBytes;
  unsigned int i;
  char *label;

  if (!s_HasProfServer || !s_HasProfConn) {
    return;
  }

  labelBytes = 0;
  for (i = 0; i < 20; ++i) {
    labelBytes += SStrLen(s_perfPrompts[i]) + 1;
  }

  packetBytes = labelBytes + 32;
  if (!IPerfRemoteSpaceAvail(packetBytes)) {
    return;
  }

  queueLabelsBuf.msgSize =
      static_cast<unsigned short>(packetBytes);
  IPerfRemoteInitSimpleHdr(&queueLabelsBuf.h, packetBytes - 4, 2);
  queueLabelsBuf.h.size = static_cast<unsigned short>(labelBytes);
  label = queueLabelsBuf.labels;

  for (i = 0; i < 20; ++i) {
    label += SStrCopy(label, s_perfPrompts[i], 0x7FFFFFFF) + 1;
  }

  IPerfRemoteQueueOnRingBuffer(&queueLabelsBuf, packetBytes);
}

void __fastcall IPerfRemoteQueueLogString(const char *prompt) {
  struct {
    unsigned long msgSize;
    SimplePerfHdr h;
    char prompt[0x100];
  } queuePromptBuf;
  unsigned int packetBytes;

  if (!s_HasProfServer || !s_HasProfConn) {
    return;
  }

  packetBytes = SStrLen(prompt) + 33;
  if (!IPerfRemoteSpaceAvail(packetBytes)) {
    return;
  }

  queuePromptBuf.msgSize = packetBytes;
  IPerfRemoteInitSimpleHdr(&queuePromptBuf.h, packetBytes - 4, 3);
  queuePromptBuf.h.size =
      static_cast<unsigned short>(SStrLen(prompt) + 1);
  SStrCopy(queuePromptBuf.prompt, prompt, sizeof(queuePromptBuf.prompt));
  IPerfRemoteQueueOnRingBuffer(&queuePromptBuf, packetBytes);
}

int __fastcall IPerfRemoteLoggingStartup() {
    // TODO: implement
    return 0;
}

int __fastcall IPerfRemoteLoggingShutdown() {
    // TODO: implement
    return 0;
}

void __fastcall ScrnPerfEnable(int enable) {
  s_enabled = enable;
}

int __fastcall ScrnPerfIsEnabled() {
  return s_enabled;
}

int __fastcall ScrnPerfRemoteShutdown() {
    // TODO: implement
    return 0;
}

int __fastcall ScrnPerfRemoteStartup() {
    // TODO: implement
    return 0;
}

void __fastcall ScrnPerfRemoteSetMode(PERF_REMOTE_MODE mode) {
  if (mode < PERF_REMOTE_MODE_MAX) {
    s_PerfRemoteMode = mode;
  } else {
    s_PerfRemoteMode = PERF_REMOTE_MODE__ONPAINT;
  }
}

PERF_REMOTE_MODE __fastcall ScrnPerfRemoteGetMode() {
  return s_PerfRemoteMode;
}

void __fastcall ScrnPerfRemoteLogString(const char *prompt) {
  IPerfRemoteQueueLogString(prompt);
}

void __fastcall ScrnPerfRemoteLogPerfCounters() {
  float perfTime[20];

  IPerfGetPerfTimes(perfTime);
  IPerfRemoteQueuePerfData(perfTime);
}

void __fastcall ScrnPerfSetTextFunction(
    void (__fastcall *inFunc)(char *, int, void *),
    void *inParam) {
  sTextFunc = inFunc;
  sTextFuncParam = inParam;
}

void __fastcall ScrnPerfToggleDisplayedValues() {
  ++s_activityMode;
  if (static_cast<unsigned int>(s_activityMode) >= 3) {
    s_activityMode = 0;
  }

  IPerfResetTimePeaks();
}

void __fastcall ScrnPerfResetTimePeaks() {
  IPerfResetTimePeaks();
}
