#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Base/CDataStore.h>

#include "Object/Petition.h"

void CGPetition::Pack(CDataStore *msg) {
  int i;

  msg->Put(m_petitionID);
  msg->Put(m_petitioner);
  msg->PutString(m_title);
  msg->PutString(m_bodyText);
  msg->Put(m_flags);
  msg->Put(m_minSignatures);
  msg->Put(m_maxSignatures);
  msg->Put(m_deadLine);
  msg->Put(m_issueDate);
  msg->Put(m_allowedGuildID);
  msg->Put(m_allowedClasses);
  msg->Put(m_allowedRaces);
  msg->Put(m_allowedGender);
  msg->Put(m_allowedMinLevel);
  msg->Put(m_allowedMaxLevel);
  msg->Put(m_numChoices);

  for (i = 0; i < m_numChoices; ++i) {
    msg->PutString(m_choicetext[i]);
  }

  msg->Put(m_muid);
}

void CGPetition::Unpack(CDataStore *msg) {
  int i;

  msg->Get(m_petitionID);
  msg->Get(m_petitioner);
  msg->GetString(m_title, 0x7FFFFFFF);
  msg->GetString(m_bodyText, 0x7FFFFFFF);
  msg->Get(m_flags);
  msg->Get(m_minSignatures);
  msg->Get(m_maxSignatures);
  msg->Get(m_deadLine);
  msg->Get(m_issueDate);
  msg->Get(m_allowedGuildID);
  msg->Get(m_allowedClasses);
  msg->Get(m_allowedRaces);
  msg->Get(m_allowedGender);
  msg->Get(m_allowedMinLevel);
  msg->Get(m_allowedMaxLevel);
  msg->Get(m_numChoices);

  for (i = 0; i < m_numChoices; ++i) {
    msg->GetString(m_choicetext[i], 0x7FFFFFFF);
  }

  msg->Get(m_muid);
}
