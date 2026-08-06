#ifndef WOW_SOURCE_CLIENT_H
#define WOW_SOURCE_CLIENT_H

#include "Tempest/crandom.h"
#include "Tempest/c3vector.h"

#include <Base/Status.h>
#include <Services/SysMessage.h>
#include <storm.h>

class CWOWClientStatus : public CStatus {
 public:
  CWOWClientStatus(LPCSTR logFile) : m_logFile(0) {
    if (logFile[0] && !SLogCreate(logFile, 0, &m_logFile)) {
      SysMsgPrintf(SYSMSG_WARNING, "Error, cannot create WOWClient log file \"%s\"!", logFile);
    }
  }

  virtual ~CWOWClientStatus() {
    if (m_logFile) {
      ITERATELIST(STATUSENTRY, statusList, entry) {
        SLogWrite(m_logFile, entry->text);
      }
      SLogClose(m_logFile);
    }
  }

 private:
  HSLOG m_logFile;
};

extern NTempest::CRndSeed g_rndSeed;

typedef int (*CLIENTTIMERHANDLER)(LPCVOID data, LPVOID param);
typedef int (*CLIENTGUIDTIMERHANDLER)(LPCVOID data, DWORDLONG guid, LPVOID param);

UINT ClientSetTimer(UINT timeout, CLIENTTIMERHANDLER handler, LPVOID param);
UINT ClientSetTimer(UINT timeout, CLIENTGUIDTIMERHANDLER handler, DWORDLONG guid, LPVOID param);
void ClientKillTimer(UINT timerId, CLIENTTIMERHANDLER handler, LPCSTR handlerName);

bool DrawingLoadingScreen();
void DisableLoadingScreen();
void EnableLoadingScreen();
void LoadingScreenRegisterWorldLoaded();
void ClientInitializeGame(UINT continentID, NTempest::C3Vector position);
void ClientPostClose();
void ClientDestroyGame(int connected, int resumeUI, int loginError);
void UninstallGameConsoleCommands();
void UninstallGMCommands();
void InstallGMCommands();

#endif
