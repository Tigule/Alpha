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

OS_IME_LANGUAGEMODE __fastcall OsIMEGetLanguageMode() {
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

  char temp[512];
  unsigned short wide[512];
  memset(temp, 0, sizeof(temp));
  ImmGetCompositionStringA(context, which, temp, sizeof(temp) - 1);
  MultiByteToWideChar(OsInputGetCodePage(), 0, temp, -1, reinterpret_cast<wchar_t *>(wide), 512);
  unsigned int length = 0;
  ConvertUTF16toUTF8(string, maxlen - 1, wide, 512, &length, 0);
  string[length] = 0;
  ImmReleaseContext(wnd, context);
  return 1;
}

int __fastcall OsIMEGetCompositionString(char* string, unsigned int maxlen) {
  return GetCompositionString(GCS_COMPSTR, string, maxlen);
}

int __fastcall OsIMEGetCompositionResult(char* string, unsigned int maxlen) {
  return GetCompositionString(GCS_RESULTSTR, string, maxlen);
}

int __fastcall OsIMEGetClauseInfo(unsigned int& clauseLeft, unsigned int& clauseRight, unsigned int& cursorPos) {
  unsigned int codePage = OsInputGetCodePage();
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  HIMC context = ImmGetContext(wnd);
  if (!context) {
    return 0;
  }

  unsigned int cursor = static_cast<unsigned short>(ImmGetCompositionStringA(context, GCS_CURSORPOS, 0, 0));
  LONG clauseBytes = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, 0, 0);
  if (!clauseBytes) {
    ImmReleaseContext(wnd, context);
    return 0;
  }

  unsigned int *clauses = static_cast<unsigned int *>(SMemAlloc(clauseBytes, __FILE__, __LINE__, 0));
  memset(clauses, 0, clauseBytes);
  LONG received = ImmGetCompositionStringA(context, GCS_COMPCLAUSE, clauses, clauseBytes);
  if (received == IMM_ERROR_NODATA || received == IMM_ERROR_GENERAL) {
    SMemFree(clauses, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, context);
    return 0;
  }

  unsigned int clauseCount = received / sizeof(unsigned int);
  unsigned int currentClause = 0;
  for (unsigned int i = 0; i + 1 < clauseCount; ++i) {
    if (cursor >= clauses[i] && cursor < clauses[i + 1]) {
      currentClause = i;
    }
  }

  unsigned char attrib[512];
  char composition[512];
  memset(attrib, 0, sizeof(attrib));
  memset(composition, 0, sizeof(composition));
  ImmGetCompositionStringA(context, GCS_COMPATTR, attrib, sizeof(attrib));
  for (unsigned int j = 0; j + 1 < clauseCount; ++j) {
    if (!attrib[clauses[j]]) {
      currentClause = j;
    }
  }
  ImmGetCompositionStringA(context, GCS_COMPSTR, composition, sizeof(composition));
  ImmReleaseContext(wnd, context);

  clauseLeft = MultiByteToWideChar(codePage, 0, composition, clauses[currentClause], 0, 0);
  clauseRight = clauseLeft + MultiByteToWideChar(
      codePage, 0, composition + clauses[currentClause],
      clauses[currentClause + 1] - clauses[currentClause], 0, 0
  );
  cursorPos = MultiByteToWideChar(codePage, 0, composition, cursor, 0, 0);
  SMemFree(clauses, __FILE__, __LINE__, 0);
  return 1;
}

int __fastcall OsIMEGetCandidates(
    unsigned long which,
    unsigned int &pageSize,
    unsigned int &count,
    unsigned int &selection,
    TSGrowableArray<OsIMECandidate> &candidates) {
  candidates.Clear();

  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  HIMC context = ImmGetContext(wnd);
  if (!context || !which) {
    if (context) {
      ImmReleaseContext(wnd, context);
    }
    return 0;
  }

  unsigned long listIndex = static_cast<unsigned long>(-1);
  while (which & 1) {
    which >>= 1;
    ++listIndex;
  }

  DWORD size = ImmGetCandidateListA(context, listIndex, 0, 0);
  if (!size) {
    ImmReleaseContext(wnd, context);
    return 0;
  }

  CANDIDATELIST *list = static_cast<CANDIDATELIST *>(SMemAlloc(size, __FILE__, __LINE__, 0));
  ImmGetCandidateListA(context, listIndex, list, size);

  if (!list->dwPageSize) {
    ImmNotifyIME(context, NI_SETCANDIDATE_PAGESIZE, listIndex, 9);
    SMemFree(list, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, context);
    return 0;
  }

  int pageChanged = 0;
  if (list->dwSelection < list->dwPageStart) {
    pageChanged = 1;
    while (list->dwPageStart > list->dwPageSize) {
      list->dwPageStart -= list->dwPageSize;
      if (list->dwSelection >= list->dwPageStart) {
        break;
      }
    }
    if (list->dwSelection < list->dwPageStart) {
      list->dwPageStart = 0;
    }
  }

  if (list->dwSelection >= list->dwPageStart + list->dwPageSize) {
    do {
      list->dwPageStart += list->dwPageSize;
    } while (list->dwSelection >= list->dwPageStart + list->dwPageSize);
    pageChanged = 1;
  }

  if (pageChanged) {
    ImmNotifyIME(context, NI_SETCANDIDATE_PAGESTART, listIndex, list->dwPageStart);
    SMemFree(list, __FILE__, __LINE__, 0);
    ImmReleaseContext(wnd, context);
    return 0;
  }

  pageSize = list->dwPageSize;
  count = list->dwCount;
  selection = list->dwSelection;

  for (unsigned int i = 0; i < pageSize; ++i) {
    OsIMECandidate *candidate = candidates.New();
    unsigned int written = 0;

    if (list->dwPageStart + i < list->dwCount) {
      unsigned short wide[512];
      const char *source = reinterpret_cast<const char *>(list) + list->dwOffset[list->dwPageStart + i];
      MultiByteToWideChar(
          OsInputGetCodePage(),
          0,
          source,
          -1,
          reinterpret_cast<wchar_t *>(wide),
          512);
      ConvertUTF16toUTF8(candidate->candidate, 1023, wide, 512, &written, 0);
    }

    candidate->candidate[written] = 0;
  }

  SMemFree(list, __FILE__, __LINE__, 0);
  ImmReleaseContext(wnd, context);
  return 1;
}

void __fastcall OsIMEEnable(int enabled) {
  HWND wnd = static_cast<HWND>(OsGuiGetWindow(0));
  if (enabled) {
    if (++s_IMEActive == 1) {
      ImmAssociateContext(wnd, s_IMC);
    }
  } else if (s_IMEActive && !--s_IMEActive) {
    ImmAssociateContext(wnd, 0);
  }
}

void __fastcall OsIMEInitialize() {
  s_IMC = ImmAssociateContext((HWND)OsGuiGetWindow(0), 0);
}

void __fastcall OsIMEDestroy() {
  ImmAssociateContextEx((HWND)OsGuiGetWindow(0), 0, IACE_DEFAULT);
  s_IMC = 0;
}
