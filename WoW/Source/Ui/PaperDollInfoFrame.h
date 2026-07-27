#ifndef WOW_SOURCE_UI_PAPERDOLLINFOFRAME_H
#define WOW_SOURCE_UI_PAPERDOLLINFOFRAME_H

struct SkillInfo {
  int  isProf;
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
  static void UpdateAllSkillLines();
  static void UpdateItem(unsigned __int64 item);
  static void PickupItem(int slot);
  static void UseItem(int slot);
  static void PickupBag(int slot);
  static int PutItemInBag(int slot);
  static int PutItemInBackpack();
  static int GetSkillOffsetFromString(const char *string, int &offset);
  static const SkillInfo *GetSkillInfoByIndex(int index);
  static int GetNumClassSkills() {
    return m_profOffset - 1;
  }
  static int GetNumSpecSkills() {
    int count = static_cast<int>(m_racialOffset - m_specialOffset - 1);
    return count < 0 ? 0 : count;
  }
  static int GetNumRacialSkills() {
    int count = static_cast<int>(m_secondaryOffset - m_racialOffset - 1);
    return count < 0 ? 0 : count;
  }
  static int GetNumSecondarySkills() {
    int count = static_cast<int>(m_numSkills - m_secondaryOffset - 1);
    return count < 0 ? 0 : count;
  }
  static int GetNumProficiencies() {
    int count = static_cast<int>(m_specialOffset - m_profOffset - 1);
    return count < 0 ? 0 : count;
  }

 protected:
  static void InstallMirrorHandlers(unsigned __int64 player);
  static void RemoveMirrorHandlers(unsigned __int64 player);
  static void OrderSkillLines();
  static unsigned int OrderProficiencies(unsigned int offset);
  static SkillInfo               m_skillInfoList[93];
  static unsigned int            m_profOffset;
  static unsigned int            m_specialOffset;
  static unsigned int            m_racialOffset;
  static unsigned int            m_secondaryOffset;
  static unsigned int            m_numSkills;
  static CGCharacterModelBase   *m_paperDoll;
};

#endif
