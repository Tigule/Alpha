#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

#include <Base/CDataStore.h>

#include "Object/GuildStats.h"

void GuildStats_C::Unpack(CDataStore *msg) {
  ASSERT(msg);
  msg->Get(m_guildID);
  msg->GetString(m_guildName, 0x18);
  msg->Get(m_emblemStyle);
  msg->Get(m_emblemColor);
  msg->Get(m_borderStyle);
  msg->Get(m_borderColor);
  msg->Get(m_backgroundColor);
}
