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
  static void __fastcall            ClearCharacterModel();
  static void __fastcall            ClearPetModel();
  static int __fastcall             GetNumCharacters();
  static CHARACTER_INFO *__fastcall GetSelectedCharacterInfo();
  static void __fastcall            GuildCallback(int guildID, const unsigned __int64 &guid, void *arg, bool granted);
  static void __fastcall            Initialize();
  static void __fastcall            SelectCharacter(int index);
  static void __fastcall            SetBackgroundModel(const char *filename);
  static void __fastcall            SetModelFrame(CSimpleModel *frame);
  static void __fastcall            Shutdown();
  static void __fastcall            UpdateCharacterList();

 protected:
  static void __fastcall ChangeSkinTexture();
  static void __fastcall EnumerateCharactersCallback(CHARACTER_INFO &info, void *__formal);
  static void __fastcall UpdateCharacterInfo();

 private:
  friend class CGlueMgr;

  static int           m_selectionIndex;
  static CSimpleModel *m_modelFrame;
};

void __fastcall CharSelectRegisterScriptFunctions();
void __fastcall CharSelectUnregisterScriptFunctions();
