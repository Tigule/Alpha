#ifndef WOW_SOURCE_CLIENT_H
#define WOW_SOURCE_CLIENT_H

#include "Tempest/crandom.h"
#include "Tempest/c3vector.h"

#include <Base/Status.h>
#include <Services/SysMessage.h>
#include <storm.h>

class CWOWClientStatus : public CStatus {
 public:
  CWOWClientStatus(const char *logFile) : m_logFile(0) {
    if (logFile[0] && !SLogCreate(logFile, 0, &m_logFile)) {
      SysMsgPrintf(SYSMSG_WARNING, "Error, cannot create WOWClient log file \"%s\"!", logFile);
    }
  }

  virtual ~CWOWClientStatus() {
    if (m_logFile) {
      STATUSENTRY *entry = statusList.Head();
      while (entry) {
        SLogWrite(m_logFile, entry->text);
        entry = statusList.Next(entry);
      }
      SLogClose(m_logFile);
    }
  }

 private:
  HSLOG m_logFile;
};

extern NTempest::CRndSeed g_rndSeed;

typedef int(__fastcall *CLIENTTIMERHANDLER)(const void *data, void *param);
typedef int(__fastcall *CLIENTGUIDTIMERHANDLER)(const void *data, unsigned __int64 guid, void *param);

unsigned int __fastcall ClientSetTimer(unsigned int timeout, CLIENTTIMERHANDLER handler, void *param);
unsigned int __fastcall ClientSetTimer(unsigned int timeout, CLIENTGUIDTIMERHANDLER handler, unsigned __int64 guid, void *param);
void __fastcall         ClientKillTimer(unsigned int timerId, CLIENTTIMERHANDLER handler, const char *handlerName);

bool __fastcall DrawingLoadingScreen();
void __fastcall DisableLoadingScreen();
void __fastcall EnableLoadingScreen();
void __fastcall LoadingScreenRegisterWorldLoaded();
void __fastcall ClientInitializeGame(unsigned int continentID, NTempest::C3Vector position);
void __fastcall ClientPostClose();
void __fastcall ClientDestroyGame(int connected, int resumeUI, int loginError);
void __fastcall UninstallGameConsoleCommands();
void __fastcall UninstallGMCommands();
void __fastcall InstallGMCommands();

#endif
