#ifndef WOW_SOURCE_OBJECT_PETITION_H
#define WOW_SOURCE_OBJECT_PETITION_H

#include <storm.h>

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

  CGPetition &operator=(const CGPetition &petition) {
    int index;

    m_petitionID = petition.m_petitionID;
    m_petitioner = petition.m_petitioner;
    SStrCopy(m_title, petition.m_title, sizeof(m_title));
    SStrCopy(m_bodyText, petition.m_bodyText, sizeof(m_bodyText));
    m_flags = petition.m_flags;
    m_minSignatures = petition.m_minSignatures;
    m_maxSignatures = petition.m_maxSignatures;
    m_deadLine = petition.m_deadLine;
    m_issueDate = petition.m_issueDate;
    m_allowedGuildID = petition.m_allowedGuildID;
    m_allowedClasses = petition.m_allowedClasses;
    m_allowedRaces = petition.m_allowedRaces;
    m_allowedGender = petition.m_allowedGender;
    m_allowedMaxLevel = petition.m_allowedMaxLevel;
    m_allowedMinLevel = petition.m_allowedMinLevel;
    m_muid = petition.m_muid;

    for (index = 0; index < 10; ++index) {
      SStrCopy(m_choicetext[index], petition.m_choicetext[index], sizeof(m_choicetext[index]));
    }

    m_numChoices = petition.m_numChoices;
    return *this;
  }

  static int Version() {
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
