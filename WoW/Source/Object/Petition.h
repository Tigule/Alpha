#ifndef WOW_SOURCE_OBJECT_PETITION_H
#define WOW_SOURCE_OBJECT_PETITION_H

class CDataStore;

class CGPetition {
 public:
  CGPetition() {
    unsigned int index;

    m_petitionID = 0;
    m_petitioner = 0;
    m_title[0] = 0;
    m_bodyText[0] = 0;
    m_flags = 0;
    m_minSignatures = 0;
    m_maxSignatures = 0;
    m_deadLine = 0;
    m_issueDate = 0;
    m_allowedGuildID = 0;
    m_allowedClasses = 0;
    m_allowedRaces = 0;
    m_allowedGender = 0;
    m_allowedMaxLevel = 0;
    m_allowedMinLevel = 0;

    for (index = 0; index < 10; ++index) {
      m_choicetext[index][0] = 0;
    }

    m_numChoices = 0;
    m_muid = 0;
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);
  void Unpack(CDataStore *msg);

  int              m_petitionID;
  unsigned __int64 m_petitioner;
  char             m_title[0x100];
  char             m_bodyText[0x1000];
  int              m_flags;
  int              m_minSignatures;
  int              m_maxSignatures;
  int              m_deadLine;
  int              m_issueDate;
  int              m_allowedGuildID;
  int              m_allowedClasses;
  int              m_allowedRaces;
  short            m_allowedGender;
  int              m_allowedMinLevel;
  int              m_allowedMaxLevel;
  char             m_choicetext[10][0x40];
  int              m_numChoices;
  unsigned int     m_muid;
};

#endif
