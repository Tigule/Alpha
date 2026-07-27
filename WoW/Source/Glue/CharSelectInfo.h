#pragma once

#include <stpl.h>

#include "Component/Component.h"
#include "Model/IModel.h"
#include "WowSvcs/WowSvcsClient/ClientConnection.h"

class CSimpleModel;
class CGlueMgr;

struct CHARINFO {
  CHARINFO() : m_eStyle(-1), m_eColor(-1), m_bStyle(-1), m_bColor(-1), m_background(-1), m_characterModel(0), m_characterComponent(0), m_petModel(0) {
  }

  ~CHARINFO();

  void ChangeSkinTexture();
  void CommitTexture(int force);
  void UpdateCharacterInfo(const char *modelName, HMODEL backgroundModel);
  void UpdateTabardTexture();

  CHARACTER_INFO m_characterInfo;
  int            m_eStyle;
  int            m_eColor;
  int            m_bStyle;
  int            m_bColor;
  int            m_background;
  HMODEL         m_characterModel;
  HTEXCOMPONENT  m_characterComponent;
  HMODEL         m_petModel;
};

class CCharSelectInfo {
 public:
  static void ClearCharacterModel();
  static void ClearCharacterList();
  static void ClearPetModel();
  static int GetNumCharacters();
  static int GetSelectionIndex();
  static CHARACTER_INFO *GetSelectedCharacterInfo();
  static void GuildCallback(int guildID, const unsigned __int64 &guid, void *arg, bool granted);
  static void Initialize();
  static void SelectCharacter(int index);
  static void SetBackgroundModel(const char *filename);
  static void SetModelFrame(CSimpleModel *frame);
  static void Shutdown();
  static void UpdateCharacterList();

 protected:
  static void ChangeSkinTexture();
  static void EnumerateCharactersCallback(CHARACTER_INFO &info, void *__formal);
  static void UpdateCharacterInfo();

 private:
  friend class CGlueMgr;

  static int           m_selectionIndex;
  static CSimpleModel *m_modelFrame;
};

void CharSelectRegisterScriptFunctions();
void CharSelectUnregisterScriptFunctions();
