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
  static void __fastcall       InitializeGame();
  static void __fastcall       ShutdownGame();
  static void __fastcall       EnterWorld();
  static void __fastcall       LeaveWorld();
  static void __fastcall       UpdateAllSkillLines();
  static void __fastcall       UpdateItem(unsigned __int64 item);
  static void __fastcall       PickupItem(int slot);
  static void __fastcall       UseItem(int slot);
  static void __fastcall       PickupBag(int slot);
  static int __fastcall        PutItemInBag(int slot);
  static int __fastcall        PutItemInBackpack();
  static int __fastcall        GetSkillOffsetFromString(const char *string, int &offset);
  static SkillInfo *__fastcall GetSkillInfoByIndex(int index);
  static int __fastcall        GetNumClassSkills() {
    return m_profOffset - 1;
  }
  static int __fastcall GetNumSpecSkills() {
    return m_racialOffset - m_specialOffset;
  }
  static int __fastcall GetNumRacialSkills() {
    return m_secondaryOffset - m_racialOffset;
  }
  static int __fastcall GetNumSecondarySkills() {
    return m_numSkills - m_secondaryOffset;
  }
  static int __fastcall GetNumProficiencies() {
    return m_specialOffset - m_profOffset;
  }

 protected:
  static void __fastcall         InstallMirrorHandlers(unsigned __int64 player);
  static void __fastcall         RemoveMirrorHandlers(unsigned __int64 player);
  static void __fastcall         OrderSkillLines();
  static unsigned int __fastcall OrderProficiencies(unsigned int offset);
  static SkillInfo               m_skillInfoList[93];
  static unsigned int            m_profOffset;
  static unsigned int            m_specialOffset;
  static unsigned int            m_racialOffset;
  static unsigned int            m_secondaryOffset;
  static unsigned int            m_numSkills;
  static CGCharacterModelBase   *m_paperDoll;
};

#endif
