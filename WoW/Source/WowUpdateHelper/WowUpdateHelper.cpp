#include <storm.h>
#include <sys/stat.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  char                program[1024];
  char                oldprogram[1024];
  char                newprogram[1024];
  char                cmdline[1024];
  STARTUPINFO         si;
  struct _stat        statbuf;
  PROCESS_INFORMATION pi;

  char *space = SStrChr(lpCmdLine, ' ');
  if (!space || (DWORD)(space - lpCmdLine) > 1023) {
    MessageBox(NULL, "Program to update not specified", "Error", MB_OK);
    return 1;
  }

  DWORD length = space - lpCmdLine;
  memcpy(program, lpCmdLine, length);
  program[length] = 0;
  SStrCopy(cmdline, space + 1, 1023);
  SStrPrintf(newprogram, 1024, "%s.new", program);
  SStrPrintf(oldprogram, 1024, "%s.old", program);

  if (!_stat(oldprogram, &statbuf) && !DeleteFile(oldprogram)) {
    MessageBox(NULL, "Can't delete previous saved version of Wow.exe(.old), please remove manually", "Error", MB_OK);
    return 1;
  }

  BOOL moved = FALSE;
  for (int i = 0; i < 30; i++) {
    if (MoveFile(program, oldprogram) || _stat(program, &statbuf)) {
      moved = TRUE;
      break;
    }
    Sleep(1000);
  }
  if (!moved) {
    MessageBox(NULL, "Can't move old program out of the way, please rename WoW.exe.new to Wow.exe manually and restart", "Error", MB_OK);
    return 1;
  }

  if (MoveFile(newprogram, program)) {
    GetStartupInfo(&si);
    CreateProcess(program, cmdline, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi);
    return 0;
  }

  MessageBox(NULL, "Can't rename new file to old, please rerun the updater", "Error", MB_OK);
  return 1;
}
