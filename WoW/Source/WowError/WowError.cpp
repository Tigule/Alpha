#include <storm.h>
#include <sys/stat.h>

#include "../../Common/WowConst.h"

#define MAX_LOADSTRING 100

const char *WOW_ERROR_SERVER_ADDRESS = "12.41.72.213";

HINSTANCE ghInstance;
HINSTANCE hInst;
HWND      hWnd;
TCHAR     szTitle[MAX_LOADSTRING];
TCHAR     szWindowClass[MAX_LOADSTRING];

int            s_sendstate;
int            s_sock = -1;
char          *s_sendptr;
bool           s_winsockInitialized;
static WSADATA s_wsaData;
static char   *s_errortext;
int            s_errortextsize;
HWND           s_edit;
HWND           s_send;
HWND           s_cancel;
HWND           s_status;
HWND           s_description;
HBRUSH         s_statusBgBrush;
char          *s_systemInfo;

ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void             AddSystemMetrics();
void             SetState(int state);
void             PollNetwork();
void             SendError();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  MSG          msg;
  HACCEL       hAccelTable;

  struct _stat filestats;
  if (!_stat(lpCmdLine, &filestats)) {
    FILE *file = fopen(lpCmdLine, "rb");
    if (file) {
      s_errortext = (char *)SMemAlloc(filestats.st_size + 1);
      fread(s_errortext, 1, filestats.st_size, file);
      s_errortext[filestats.st_size] = 0;
      s_errortextsize = filestats.st_size;
      fclose(file);
    }
  }

  if (!s_errortext) {
    return 0;
  }

  AddSystemMetrics();
  s_errortextsize = SStrLen(s_errortext);
  msg.wParam = 0;
  LoadString(hInstance, 103, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, 109, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);
  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)109);
  bool done = false;
  while (!done) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
      if (!GetMessage(&msg, NULL, 0, 0)) {
        done = true;
        break;
      }

      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    PollNetwork();
    Sleep(25);
  }

  SMemFree(s_errortext);
  return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEX wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)107);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_SCROLLBAR + 1);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)107);

  return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  HWND  ctrl;
  int   height;
  RECT  wndClientRect;
  char  text[1204];
  HFONT font;
  RECT  wndRect;
  hInst = hInstance;
  ghInstance = hInstance;
  hWnd = CreateWindowEx(WS_EX_CONTROLPARENT, szWindowClass, szTitle, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, CW_USEDEFAULT, 0, 640, 400, NULL, NULL, hInstance, NULL);

  if (!hWnd) {
    return FALSE;
  }

  GetWindowRect(hWnd, &wndRect);
  GetClientRect(hWnd, &wndClientRect);
  int width = wndClientRect.right - wndClientRect.left;
  height = wndClientRect.bottom - wndClientRect.top;
  LoadString(ghInstance, 131, text, sizeof(text));
  font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  ctrl = CreateWindowEx(0, "STATIC", text, WS_CHILD | WS_VISIBLE, wndClientRect.left + 5, wndClientRect.top, width - 10, 30, hWnd, NULL, ghInstance, NULL);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  ctrl = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "edit", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, wndClientRect.left, wndClientRect.top + 20, width, height - 40, hWnd, NULL, ghInstance, NULL);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  SendMessage(ctrl, WM_SETTEXT, 0, (LPARAM)s_errortext);
  s_edit = ctrl;

  LoadString(ghInstance, 141, text, sizeof(text));
  CreateWindowEx(0, "STATIC", text, WS_CHILD | WS_VISIBLE, wndClientRect.left + 5, wndClientRect.top + 30, width - 10, 18, hWnd, NULL, ghInstance, NULL);
  ctrl = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE, wndClientRect.left, wndClientRect.top + 48, width - 10, 50, hWnd, (HMENU)1002, ghInstance, NULL);

  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  SendMessage(s_description, EM_SETSEL, 0, -1);
  s_description = ctrl;
  LoadString(ghInstance, 132, text, sizeof(text));
  ctrl = CreateWindowEx(0, "BUTTON", text, WS_CHILD | WS_VISIBLE, width - 50, height - 25, 100, 20, hWnd, (HMENU)1000, ghInstance, NULL);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  s_send = ctrl;
  LoadString(ghInstance, 133, text, sizeof(text));
  ctrl = CreateWindowEx(0, "BUTTON", text, WS_CHILD | WS_VISIBLE, 0, 0, 100, 20, hWnd, (HMENU)1001, ghInstance, NULL);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  s_cancel = ctrl;
  ctrl = CreateWindowEx(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_CENTER, 0, height - 45, width, 20, hWnd, NULL, ghInstance, NULL);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)font, 0);
  s_status = ctrl;
  SetState(0);
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  PAINTSTRUCT ps;
  HDC         hdc;
  switch (message) {
    case WM_CREATE:
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case 104:
          DialogBox(hInst, (LPCTSTR)103, hWnd, (DLGPROC)About);
          break;
        case 105:
        case 1001:
          DestroyWindow(hWnd);
          break;
        case 1000:
          SendError();
          break;
        default:
          return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);

      RECT rt;
      GetClientRect(hWnd, &rt);
      EndPaint(hWnd, &ps);
      break;
    case WM_SIZE: {
      RECT wndClientRect;
      GetClientRect(hWnd, &wndClientRect);

      int width = wndClientRect.right - wndClientRect.left;
      int height = wndClientRect.bottom - wndClientRect.top;
      MoveWindow(s_edit, wndClientRect.left + 5, wndClientRect.top + 103, width - 10, height - 153, FALSE);
      MoveWindow(s_description, wndClientRect.left + 5, wndClientRect.top + 48, width - 10, 50, FALSE);

      MoveWindow(s_send, width / 2 - 50, height - 25, 100, 20, FALSE);
      MoveWindow(s_cancel, width - 105, height - 25, 100, 20, FALSE);
      MoveWindow(s_status, wndClientRect.left + 5, height - 45, width - 7, 15, FALSE);
      break;
    }
    case WM_CTLCOLORSTATIC:
      if ((HWND)lParam == s_status) {

        if (!s_statusBgBrush) {
          s_statusBgBrush = CreateSolidBrush(RGB(0, 0, 255));
        }
        SetTextColor((HDC)wParam, RGB(255, 255, 255));
        SetBkColor((HDC)wParam, RGB(0, 0, 255));
        return (LRESULT)s_statusBgBrush;
      }
      return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG:
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
      }
      break;
  }
  return FALSE;
}

void AddSystemMetrics() {
  MEMORYSTATUS meminfo;
  SYSTEM_INFO  info;
  s_systemInfo = SStrDupA("\r\n======================================================================\r\nHardware/Driver Information:\r\n", __FILE__, __LINE__);
  char         systemstr[1024];
  GetSystemInfo(&info);
  GlobalMemoryStatus(&meminfo);
  SStrPrintf(systemstr,
    sizeof(systemstr),
    "Processor:              0x%x\r\n"
    "Page Size:              %d\r\n"
    "Min App Address:        0x%x\r\n"
    "Max App Address:        0x%x\r\n"
    "Processor Mask:         0x%x\r\n"
    "Number of Processors:   %d\r\n"
    "Processor Type:         %d\r\n"
    "Allocation Granularity: %d\r\n"
    "Processor Level:        %d\r\n"
    "Processor Revision:     %d\r\n"
    "\r\n"
    "Percent memory used:    %d\r\n"
    "Total physical memory:  %d\r\n"
    "Free Memory:            %d\r\n"
    "Page file:              %d\r\n"
    "Total virtual memory:   %d\r\n",
    info.dwOemId,
    info.dwPageSize,
    info.lpMinimumApplicationAddress,
    info.lpMaximumApplicationAddress,
    info.dwActiveProcessorMask,
    info.dwNumberOfProcessors,
    info.dwProcessorType,
    info.dwAllocationGranularity,
    info.wProcessorLevel,
    info.wProcessorRevision,
    meminfo.dwMemoryLoad,
    meminfo.dwTotalPhys,
    meminfo.dwAvailPhys,
    meminfo.dwTotalPageFile,
    meminfo.dwTotalVirtual
  );

  DWORD size = SStrLen(s_systemInfo) + SStrLen(systemstr) + 1;
  s_systemInfo = (char *)SMemReAlloc(s_systemInfo, size, __FILE__, __LINE__, 0);
  SStrPack(s_systemInfo, systemstr, size);

  s_errortext = (char *)SMemReAlloc(s_errortext, SStrLen(s_errortext) + SStrLen(s_systemInfo) + 1, __FILE__, __LINE__, 0);
  SStrPack(s_errortext, s_systemInfo, 0x7FFFFFFF);
}

void SetState(int state) {
  char text[256];
  char done[256];
  s_sendstate = state;
  UINT id = 134;
  switch (state) {
    case 0:
      id = 135;
      break;
    case 1:
      id = 136;
      EnableWindow(s_send, FALSE);

      break;
    case 2:
      id = 137;
      break;
    case 3:
    case 4:
      id = 138;
      LoadString(ghInstance, 140, done, sizeof(done));
      SendMessage(s_cancel, WM_SETTEXT, 0, (LPARAM)done);

      break;
    case 5:
      id = 139;
      EnableWindow(s_send, TRUE);
      break;
    case 6:
      id = 134;
      EnableWindow(s_send, TRUE);
      break;
  }
  LoadString(ghInstance, id, text, sizeof(text));
  SendMessage(s_status, WM_SETTEXT, 0, (LPARAM)text);
}

void Disconnect() {
  closesocket(s_sock);
  s_sock = -1;
  SetState(4);
}

void DoSend() {
  if (s_sock < 0) {
    return;
  }
  while (s_sendptr < s_errortext + s_errortextsize) {
    int size = s_errortext + s_errortextsize - s_sendptr;
    if (size > 1024) {
      size = 1024;
    }
    int sent = send(s_sock, s_sendptr, size, 0);
    if (sent < 0) {
      if (WSAGetLastError() != WSAEWOULDBLOCK) {
        Disconnect();
        SetState(6);
      }
      return;
    } else if (sent > 0) {
      s_sendptr += sent;
      if (s_sendptr >= s_errortext + s_errortextsize) {
        SetState(3);
        Disconnect();
        return;
      }
    }
  }
}

void CheckConnect() {
  struct timeval tv;
  fd_set         wfds;
  fd_set         efds;
  int            sockerrlen;
  int            sockerr;

  FD_ZERO(&wfds);
  FD_ZERO(&efds);
  FD_SET(s_sock, &efds);
  FD_SET(s_sock, &wfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  select(s_sock + 1, NULL, &wfds, &efds, &tv);

  if (FD_ISSET(s_sock, &wfds) || FD_ISSET(s_sock, &efds)) {
    sockerrlen = sizeof(sockerr);
    int result = getsockopt(s_sock, SOL_SOCKET, SO_ERROR, (char *)&sockerr, &sockerrlen);

    if (result) {
      result = WSAGetLastError();
      if (result == WSAEWOULDBLOCK || result == WSAEINPROGRESS) {
        return;
      }
    }
    if (sockerr) {
      if (sockerr == WSAEWOULDBLOCK || sockerr == WSAEINPROGRESS) {
        return;
      }
      Disconnect();
      SetState(5);
    } else {
      SetState(2);
      s_sendptr = s_errortext;
      DoSend();
    }
  }
}

void PollNetwork() {
  if (!s_sendstate) {
    return;
  }

  if (s_sock < 0) {
    return;
  }

  switch (s_sendstate) {
    case 1:
      CheckConnect();
      break;
    case 2:
      DoSend();
      break;
  }
}

DWORD GetServerAddress() {
  DWORD addr = 0;
  struct hostent *host = gethostbyname(WOW_ERROR_SERVER_ADDRESS);

  if (host) {
    addr = (((((BYTE)host->h_addr[3] << 8) | (BYTE)host->h_addr[2]) << 8 | (BYTE)host->h_addr[1]) << 8) | (BYTE)host->h_addr[0];
  }
  return addr;
}

void SendError() {
  struct sockaddr_in addr;
  char               description[2048];
  u_long             on;
  GetDlgItemText(hWnd, 1002, description, sizeof(description) - 1);
  description[sizeof(description) - 1] = 0;

  DWORD size = SStrLen(s_errortext) + SStrLen("Description:\r\n") + SStrLen(description) + 10;
  char *text = (char *)ALLOC(size);
  SStrCopy(text, "Description:\r\n", size);
  SStrPack(text, description, size);
  SStrPack(text, "\r\n", size);
  SStrPack(text, s_errortext, size);
  FREE(s_errortext);
  s_errortext = text;
  s_errortextsize = SStrLen(s_errortext);

  if (s_sendstate != 0 && s_sendstate != 5) {
    return;
  }

  if (!s_winsockInitialized) {
    if (WSAStartup(MAKEWORD(2, 2), &s_wsaData)) {
      return;
    }
    if (LOBYTE(s_wsaData.wVersion) < 1 || (LOBYTE(s_wsaData.wVersion) == 1 && HIBYTE(s_wsaData.wVersion) < 1)) {
      return;
    }
  }

  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_sock < 0) {
    return;
  }

  on = 1;
  ioctlsocket(s_sock, FIONBIO, &on);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8086);

  DWORD ip = GetServerAddress();
  if (!ip) {
    SetState(5);
    return;
  }

  addr.sin_addr.s_addr = ip;
  SetState(1);

  if (connect(s_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) {
      return;
    }
    SetState(5);

    closesocket(s_sock);
    s_sock = -1;
  } else {
    CheckConnect();
  }
}
