#include <storm.h>

static DWORD s_alloccount;
static DWORD s_freecount;

void __fastcall StormOptCdThread(DWORD *threadId, void **hThread);

STORMOPTIONS g_opt = {
    0, 0, 0x10000, 0, 0, 0, 0, 0,
};

void __fastcall IncrementAllocCount() {
  ++s_alloccount;
}

void __fastcall IncrementFreeCount() {
  ++s_freecount;
}

extern "C" BOOL APIENTRY StormGetOption(int optname, void *optval, LPDWORD optlen) {
  FATALASSERT(optval);
  FATALASSERT(optlen);
  SErrSetLastError(ERROR_INVALID_PARAMETER);
  switch (optname) {
    case 1:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.serrleaksilentwarning;
      break;
    case 2:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = g_opt.wavechunksize;
      break;
    case 3:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.smemleaksilentwarning;
      break;
    case 4:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.alignstreamingwavedata;
      break;
    case 5:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.echotooutputdebugstring;
      break;
    case 6:
      if (*optlen < 2 * sizeof(DWORD)) {
        return FALSE;
      }
      ((DWORD *)optval)[0] = s_alloccount;
      ((DWORD *)optval)[1] = s_freecount;
      *optlen = 2 * sizeof(DWORD);
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 7:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.serrsuppresslogs;
      break;
    case 8:
      if (*optlen < 2 * sizeof(DWORD)) {
        return FALSE;
      }
      {
        DWORD threadId;
        void *hThread;

        StormOptCdThread(&threadId, &hThread);
        ((DWORD *)optval)[0] = threadId;
        ((DWORD *)optval)[1] = (DWORD)hThread;
      }
      *optlen = 2 * sizeof(DWORD);
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 9:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.crcenabled;
      break;
    case 10:
      if (*optlen < sizeof(DWORD)) {
        return FALSE;
      }
      *(DWORD *)optval = (DWORD)g_opt.orderedprintfenabled;
      break;
    default:
      return FALSE;
  }

  *optlen = sizeof(DWORD);
  SErrSetLastError(ERROR_SUCCESS);
  return TRUE;
}

extern "C" BOOL APIENTRY StormSetOption(int optname, void *optval, DWORD optlen) {
  FATALASSERT(optval);
  SErrSetLastError(ERROR_INVALID_PARAMETER);
  switch (optname) {
    case 1:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.serrleaksilentwarning = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 2:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      if (!*(DWORD *)optval || ((*(DWORD *)optval - 1) ^ *(DWORD *)optval) != (*(DWORD *)optval + *(DWORD *)optval - 1)) {
        return FALSE;
      }
      g_opt.wavechunksize = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 3:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.smemleaksilentwarning = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 4:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.alignstreamingwavedata = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 5:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.echotooutputdebugstring = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 6:
      if (optlen != 2 * sizeof(DWORD)) {
        return FALSE;
      }
      s_alloccount = ((DWORD *)optval)[0];
      s_freecount = ((DWORD *)optval)[1];
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 7:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.serrsuppresslogs = *(DWORD *)optval;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 8:
      return FALSE;
    case 9:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.crcenabled = *(DWORD *)optval != 0;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    case 10:
      if (optlen != sizeof(DWORD)) {
        return FALSE;
      }
      g_opt.orderedprintfenabled = *(DWORD *)optval != 0;
      SErrSetLastError(ERROR_SUCCESS);
      return TRUE;
    default:
      return FALSE;
  }
}
