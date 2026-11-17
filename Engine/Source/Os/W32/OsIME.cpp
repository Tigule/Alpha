#include <winsock2.h>
#include <windows.h>
#include <Base/Base.h>
#include <time.h>

#include "OsGui.h"

#include "OsIME.h"
#include "Input.h"

#include "Base/ConvertUTF.h"

#include <imm.h>
#include <stdio.h>

typedef struct HIMC__ *HIMC;
static HIMC s_IMC;
static int  s_IMEActive;

OS_IME_LANGUAGEMODE OsIMEGetLanguageMode() {
  HWND wnd = (HWND)OsGuiGetWindow(0);
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return OS_IME_MODE_ROMAN;
  }

  OS_IME_LANGUAGEMODE mode = OS_IME_MODE_ROMAN;
  if (ImmGetOpenStatus(context)) {
    switch (OsInputGetCodePage()) {
      case 932:
        mode = OS_IME_MODE_JAPANESE;
        break;
      case 949:
        mode = OS_IME_MODE_KOREAN;
        break;
      case 936:
      case 950:
        mode = OS_IME_MODE_CHINESE;
        break;
    }
  }
  ImmReleaseContext(wnd, context);
  return mode;
}

static BOOL GetCompositionString(int which, char *string, int maxlen) {
  memset(string, 0, maxlen);
  HWND wnd = (HWND)OsGuiGetWindow(0);
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return FALSE;
  }

  WORD wtemp[512];
  ImmGetCompositionStringA(context, which, string, maxlen);
  MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, string, -1, wtemp, 512);
  UINT written;
  ConvertUTF16toUTF8(string, maxlen - 1, wtemp, 512, &written, 0);
  string[written] = 0;
  ImmReleaseContext(wnd, context);
  return TRUE;
}

int OsIMEGetCompositionString(char *string, UINT maxlen) {
  return GetCompositionString(GCS_COMPSTR, string, maxlen);
}

int OsIMEGetCompositionResult(char *string, UINT maxlen) {
  return GetCompositionString(GCS_RESULTSTR, string, maxlen);
}

BOOL OsIMEGetClauseInfo(UINT &clauseLeft, UINT &clauseRight, UINT &cursorPos) {
  UINT codepage = OsInputGetCodePage();
  HWND wnd = (HWND)OsGuiGetWindow(0);
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return FALSE;
  }

  UINT cursor = ImmGetCompositionStringA(context, GCS_CURSORPOS, 0, 0) & 0xFFFF;
  UINT length = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, 0, 0);
  if (!length) {
    ImmReleaseContext(wnd, context);
    return FALSE;
  }

  UINT *clauses = (UINT *)SMemAlloc(length, __FILE__, __LINE__, 0);
  memset(clauses, 0, length);
  length = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, clauses, length);
  UINT currentClause = 0;
  if (length == IMM_ERROR_NODATA || length == IMM_ERROR_GENERAL) {
    SMemFree(clauses, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, context);
    return FALSE;
  }

  length /= sizeof(UINT);
  UINT i;
  if (length > 2) {
    for (i = 0; i < length - 1; ++i) {
      if (cursor >= clauses[i] && cursor < clauses[i + 1]) {
        currentClause = i;
      }
    }
  }

  char attrib[512];
  ImmGetCompositionStringA(context, GCS_COMPATTR, attrib, sizeof(attrib));
  for (i = 0; i < length - 1; ++i) {
    if (!attrib[clauses[i]]) {
      currentClause = i;
    }
  }
  char string[512];
  memset(string, 0, sizeof(string));
  ImmGetCompositionStringA(context, GCS_COMPSTR, string, sizeof(string));
  ImmReleaseContext(wnd, context);

  UINT cursorLen = MultiByteToWideChar(codepage, MB_PRECOMPOSED, string, cursor, 0, 0);
  length = MultiByteToWideChar(codepage, MB_PRECOMPOSED, string, clauses[currentClause], 0, 0);
  UINT clauseLen =
      MultiByteToWideChar(codepage, MB_PRECOMPOSED, string, clauses[currentClause + 1] - clauses[currentClause], 0, 0);
  SMemFree(clauses, __FILE__, __LINE__, 0);
  clauseLeft = length;
  clauseRight = length + clauseLen;
  cursorPos = cursorLen;
  return TRUE;
}

BOOL OsIMEGetCandidates(DWORD which, UINT &pagesize, UINT &count, UINT &selection, TSGrowableArray<OsIMECandidate> &candidates) {
  candidates.Clear();

  HWND wnd = (HWND)OsGuiGetWindow(0);
  HIMC hIMC = ImmGetContext(wnd);
  if (!hIMC) {
    return FALSE;
  }

  if (!which) {
    ImmReleaseContext(wnd, hIMC);
    return FALSE;
  }

  DWORD listIndex = -1;
  while (which & 1) {
    which >>= 1;
    ++listIndex;
  }

  DWORD size = ImmGetCandidateListA(hIMC, listIndex, 0, 0);
  if (!size) {
    ImmReleaseContext(wnd, hIMC);
    return FALSE;
  }

  int            pageStartChanged = 0;
  CANDIDATELIST *pcl = (CANDIDATELIST *)SMemAlloc(size, __FILE__, __LINE__, 0);
  ImmGetCandidateListA(hIMC, listIndex, pcl, size);

  if (!pcl->dwPageSize) {
    ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, listIndex, 9);
    SMemFree(pcl, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, hIMC);
    return FALSE;
  }

  while (pcl->dwSelection < pcl->dwPageStart) {
    if (pcl->dwPageStart > pcl->dwPageSize) {
      pcl->dwPageStart -= pcl->dwPageSize;
    } else {
      pcl->dwPageStart = 0;
    }
    pageStartChanged = 1;
  }

  while (pcl->dwPageStart + pcl->dwPageSize <= pcl->dwSelection) {
    pcl->dwPageStart += pcl->dwPageSize;
    pageStartChanged = 1;
  }

  if (pageStartChanged) {
    ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, listIndex, pcl->dwPageStart);
    SMemFree(pcl, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, hIMC);
    return FALSE;
  }

  pagesize = pcl->dwPageSize;
  count = pcl->dwCount;
  selection = pcl->dwSelection;

  for (UINT i = 0; i < pcl->dwPageSize; ++i) {
    OsIMECandidate *candidate = candidates.New();
    UINT            written = 0;

    if (pcl->dwPageStart + i < pcl->dwCount) {
      WORD wtemp[512];
      MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, (LPCSTR)pcl + pcl->dwOffset[pcl->dwPageStart + i], -1, wtemp, 512);
      ConvertUTF16toUTF8(candidate->candidate, 1023, wtemp, 512, &written, 0);
    }

    candidate->candidate[written] = 0;
  }

  SMemFree(pcl, __FILE__, __LINE__, 0);
  ImmReleaseContext(wnd, hIMC);
  return TRUE;
}

void OsIMEEnable(int enabled) {
  HWND wnd = (HWND)OsGuiGetWindow(0);
  if (enabled) {
    if (++s_IMEActive == 1) {
      ImmAssociateContext(wnd, s_IMC);
    }
  } else if (s_IMEActive && !--s_IMEActive) {
    ImmAssociateContext(wnd, 0);
  }
}

void OsIMEInitialize() {
  s_IMC = ImmAssociateContext((HWND)OsGuiGetWindow(0), 0);
}

void OsIMEDestroy() {
  ImmAssociateContextEx((HWND)OsGuiGetWindow(0), 0, IACE_DEFAULT);
  s_IMC = NULL;
}
