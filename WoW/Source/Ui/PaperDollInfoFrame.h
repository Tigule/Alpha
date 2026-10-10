#ifndef WOW_SOURCE_UI_PAPERDOLLINFOFRAME_H
#define WOW_SOURCE_UI_PAPERDOLLINFOFRAME_H

struct SkillInfo {
  BOOL isProf;
  int  skillID;
  int  profLevel;
  char profName[64];
};

class CGCharacterModelBase;

class CGCharacterInfo {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void PickupItem(int slot);
  static void UseItem(int slot);
  static void PickupBag(int slot);
  static BOOL PutItemInBag(int slot);
  static int  PutItemInBackpack();
  static void UpdateAllSkillLines();
  static void UpdateItem(DWORDLONG item);
  static int              GetNumClassSkills() {
    return m_profOffset - 1;
  }
  static int GetNumSpecSkills() {
    return max(0, (int)(m_racialOffset - m_specialOffset - 1));
  }
  static int GetNumRacialSkills() {
    return max(0, (int)(m_secondaryOffset - m_racialOffset - 1));
  }
  static int GetNumSecondarySkills() {
    return max(0, (int)(m_numSkills - m_secondaryOffset - 1));
  }
  static int GetNumProficiencies() {
    return max(0, (int)(m_specialOffset - m_profOffset - 1));
  }
  static int              GetSkillOffsetFromString(LPCSTR string, int &offset);
  static const SkillInfo *GetSkillInfoByIndex(int index);

 protected:
  static void InstallMirrorHandlers(DWORDLONG player);
  static void RemoveMirrorHandlers(DWORDLONG player);
  static void OrderSkillLines();
  static UINT OrderProficiencies(UINT offset);

  static SkillInfo             m_skillInfoList[93];
  static UINT                  m_profOffset;
  static UINT                  m_specialOffset;
  static UINT                  m_racialOffset;
  static UINT                  m_secondaryOffset;
  static UINT                  m_numSkills;
  static CGCharacterModelBase *m_paperDoll;
};

#endif
