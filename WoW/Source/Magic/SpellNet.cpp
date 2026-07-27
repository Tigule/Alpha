#include <Base/CDataStore.h>
#include <Tempest/c3vector.h>

struct SpellCast {
  unsigned __int64   caster;
  unsigned __int64   casterUnit;
  int                spellID;
  unsigned short     targets;
  unsigned __int64   unitTarget;
  unsigned __int64   itemTarget;
  unsigned __int64   selectedTarget;
  NTempest::C3Vector sourceLocation;
  NTempest::C3Vector destLocation;
  float              destFacing;
  unsigned int       destZoneID;
  unsigned int       castTime;
  unsigned int       castEndTime;
  int                spellIndex;
  unsigned int       spellLevel;
  unsigned __int64   ammoItem;
  unsigned __int64   reflector;
  char               targetString[128];
  int                overrideRank;
  unsigned short     flags;
};

void SpellPutCastTargets(SpellCast *cast, CDataStore *msg) {
  msg->Put(cast->targets);
  if (cast->targets & 0x802) {
    msg->Put(cast->unitTarget);
  }
  if (cast->targets & 0x1010) {
    msg->Put(cast->itemTarget);
  }
  if (cast->targets & 0x20) {
    msg->Put(cast->sourceLocation.x);
    msg->Put(cast->sourceLocation.y);
    msg->Put(cast->sourceLocation.z);
  }
  if (cast->targets & 0x40) {
    msg->Put(cast->destLocation.x);
    msg->Put(cast->destLocation.y);
    msg->Put(cast->destLocation.z);
  }
  if (cast->targets & 0x2000) {
    msg->PutArray(cast->targetString, sizeof(cast->targetString));
  }
}

void SpellGetCastTargets(SpellCast* cast, CDataStore* msg) {
  msg->Get(cast->targets);
  if (cast->targets & 0x802) {
    msg->Get(cast->unitTarget);
  }
  if (cast->targets & 0x1010) {
    msg->Get(cast->itemTarget);
  }
  if (cast->targets & 0x20) {
    msg->Get(cast->sourceLocation.x);
    msg->Get(cast->sourceLocation.y);
    msg->Get(cast->sourceLocation.z);
  }
  if (cast->targets & 0x40) {
    msg->Get(cast->destLocation.x);
    msg->Get(cast->destLocation.y);
    msg->Get(cast->destLocation.z);
  }
  if (cast->targets & 0x2000) {
    msg->GetArray(reinterpret_cast<unsigned char *>(cast->targetString), sizeof(cast->targetString));
  }
}
