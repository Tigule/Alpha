#include "OsGui.h"
#include "OsIME.h"
#include "Input.h"

#include "Base/ConvertUTF.h"

#include <windows.h>
#include <imm.h>

#ifndef IACE_DEFAULT
#define IACE_DEFAULT 0x0010
extern "C" BOOL WINAPI ImmAssociateContextEx(HWND, HIMC, DWORD);
#endif

static HIMC s_IMC;
static int  s_IMEActive;

OS_IME_LANGUAGEMODE OsIMEGetLanguageMode() {
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
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

static int GetCompositionString(int which, char* string, int maxlen) {
  memset(string, 0, maxlen);
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return 0;
  }

  unsigned short wtemp[512];
  ImmGetCompositionStringA(context, which, string, maxlen);
  MultiByteToWideChar(OsInputGetCodePage(), 0, string, -1, reinterpret_cast<wchar_t *>(wtemp), 512);
  ConvertUTF16toUTF8(string, maxlen - 1, wtemp, 512, reinterpret_cast<unsigned int *>(&maxlen), 0);
  string[maxlen] = 0;
  ImmReleaseContext(wnd, context);
  return 1;
}

int OsIMEGetCompositionString(char* string, unsigned int maxlen) {
  return GetCompositionString(GCS_COMPSTR, string, maxlen);
}

int OsIMEGetCompositionResult(char* string, unsigned int maxlen) {
  return GetCompositionString(GCS_RESULTSTR, string, maxlen);
}

int OsIMEGetClauseInfo(unsigned int& clauseLeft, unsigned int& clauseRight, unsigned int& cursorPos) {
  unsigned int codepage = OsInputGetCodePage();
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return 0;
  }

  unsigned int cursor = static_cast<unsigned short>(ImmGetCompositionStringA(context, GCS_CURSORPOS, 0, 0));
  unsigned int length = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, 0, 0);
  if (!length) {
    ImmReleaseContext(wnd, context);
    return 0;
  }

  unsigned int *clauses = static_cast<unsigned int *>(SMemAlloc(length, __FILE__, __LINE__, 0));
  memset(clauses, 0, length);
  length = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, clauses, length);
  if (length == IMM_ERROR_NODATA || length == IMM_ERROR_GENERAL) {
    SMemFree(clauses, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, context);
    return 0;
  }

  length /= sizeof(unsigned int);
  unsigned int currentClause = 0;
  unsigned int i;
  for (i = 0; i + 1 < length; ++i) {
    if (cursor >= clauses[i] && cursor < clauses[i + 1]) {
      currentClause = i;
    }
  }

  unsigned char attrib[512];
  ImmGetCompositionStringA(context, GCS_COMPATTR, attrib, sizeof(attrib));
  for (i = 0; i + 1 < length; ++i) {
    if (!attrib[clauses[i]]) {
      currentClause = i;
    }
  }
  char string[512];
  memset(string, 0, sizeof(string));
  ImmGetCompositionStringA(context, GCS_COMPSTR, string, sizeof(string));
  ImmReleaseContext(wnd, context);

  unsigned int cursorLen = MultiByteToWideChar(codepage, 0, string, cursor, 0, 0);
  length = MultiByteToWideChar(codepage, 0, string, clauses[currentClause], 0, 0);
  currentClause = length + MultiByteToWideChar(
      codepage, 0, string + clauses[currentClause],
      clauses[currentClause + 1] - clauses[currentClause], 0, 0
  );
  SMemFree(clauses, __FILE__, __LINE__, 0);
  clauseLeft = length;
  clauseRight = currentClause;
  cursorPos = cursorLen;
  return 1;
}

int OsIMEGetCandidates(
    unsigned long which,
    unsigned int &pagesize,
    unsigned int &count,
    unsigned int &selection,
    TSGrowableArray<OsIMECandidate> &candidates) {
  candidates.Clear();

  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  HIMC hIMC = ImmGetContext(wnd);
  if (!hIMC || !which) {
    if (hIMC) {
      ImmReleaseContext(wnd, hIMC);
    }
    return 0;
  }

  unsigned long listIndex = static_cast<unsigned long>(-1);
  while (which & 1) {
    which >>= 1;
    ++listIndex;
  }

  DWORD size = ImmGetCandidateListA(hIMC, listIndex, 0, 0);
  if (!size) {
    ImmReleaseContext(wnd, hIMC);
    return 0;
  }

  CANDIDATELIST *pcl = static_cast<CANDIDATELIST *>(SMemAlloc(size, __FILE__, __LINE__, 0));
  ImmGetCandidateListA(hIMC, listIndex, pcl, size);

  if (!pcl->dwPageSize) {
    ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, listIndex, 9);
    SMemFree(pcl, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, hIMC);
    return 0;
  }

  int pageStartChanged = 0;
  if (pcl->dwSelection < pcl->dwPageStart) {
    pageStartChanged = 1;
    while (pcl->dwPageStart > pcl->dwPageSize) {
      pcl->dwPageStart -= pcl->dwPageSize;
      if (pcl->dwSelection >= pcl->dwPageStart) {
        break;
      }
    }
    if (pcl->dwSelection < pcl->dwPageStart) {
      pcl->dwPageStart = 0;
    }
  }

  if (pcl->dwSelection >= pcl->dwPageStart + pcl->dwPageSize) {
    do {
      pcl->dwPageStart += pcl->dwPageSize;
    } while (pcl->dwSelection >= pcl->dwPageStart + pcl->dwPageSize);
    pageStartChanged = 1;
  }

  if (pageStartChanged) {
    ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, listIndex, pcl->dwPageStart);
    SMemFree(pcl, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, hIMC);
    return 0;
  }

  pagesize = pcl->dwPageSize;
  count = pcl->dwCount;
  selection = pcl->dwSelection;

  for (unsigned int i = 0; i < pagesize; ++i) {
    OsIMECandidate *candidate = candidates.New();
    unsigned int written = 0;

    if (pcl->dwPageStart + i < pcl->dwCount) {
      unsigned short wtemp[512];
      const char *source = reinterpret_cast<const char *>(pcl) + pcl->dwOffset[pcl->dwPageStart + i];
      MultiByteToWideChar(
          OsInputGetCodePage(),
          0,
          source,
          -1,
          reinterpret_cast<wchar_t *>(wtemp),
          512);
      ConvertUTF16toUTF8(candidate->candidate, 1023, wtemp, 512, &written, 0);
    }

    candidate->candidate[written] = 0;
  }

  SMemFree(pcl, __FILE__, __LINE__, 0);
  ImmReleaseContext(wnd, hIMC);
  return 1;
}

void OsIMEEnable(int enabled) {
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
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
  s_IMC = 0;
}
