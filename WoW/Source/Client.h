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

typedef int(*CLIENTTIMERHANDLER)(const void *data, void *param);
typedef int(*CLIENTGUIDTIMERHANDLER)(const void *data, unsigned __int64 guid, void *param);

unsigned int ClientSetTimer(unsigned int timeout, CLIENTTIMERHANDLER handler, void *param);
unsigned int ClientSetTimer(unsigned int timeout, CLIENTGUIDTIMERHANDLER handler, unsigned __int64 guid, void *param);
void ClientKillTimer(unsigned int timerId, CLIENTTIMERHANDLER handler, const char *handlerName);

bool DrawingLoadingScreen();
void DisableLoadingScreen();
void EnableLoadingScreen();
void LoadingScreenRegisterWorldLoaded();
void ClientInitializeGame(unsigned int continentID, NTempest::C3Vector position);
void ClientPostClose();
void ClientDestroyGame(int connected, int resumeUI, int loginError);
void UninstallGameConsoleCommands();
void UninstallGMCommands();
void InstallGMCommands();

#endif
