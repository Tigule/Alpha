#include "mirror.h"
#include "Object/Object.h"

#include <storm.h>
#include <string.h>

static ObjDataDescriptor s_objBaseDescriptors[5] = {
    {   "OBJECT_FIELD_GUID", 0, 2, 4, 1},
    {   "OBJECT_FIELD_TYPE", 2, 1, 1, 1},
    {  "OBJECT_FIELD_ENTRY", 3, 1, 1, 1},
    {"OBJECT_FIELD_SCALE_X", 4, 1, 3, 1},
    {"OBJECT_FIELD_PADDING", 5, 1, 1, 1},
};

static ObjDataDescriptor s_itemBaseDescriptors[9] = {
    {        "ITEM_FIELD_OWNER",  0,  2, 4,  1},
    {    "ITEM_FIELD_CONTAINED",  2,  2, 4,  1},
    {      "ITEM_FIELD_CREATOR",  4,  2, 4,  1},
    {  "ITEM_FIELD_STACK_COUNT",  6,  1, 1, 20},
    {     "ITEM_FIELD_DURATION",  7,  1, 1, 20},
    {"ITEM_FIELD_SPELL_CHARGES",  8,  5, 1, 20},
    {        "ITEM_FIELD_FLAGS", 13,  1, 2,  1},
    {  "ITEM_FIELD_ENCHANTMENT", 14, 15, 1,  1},
    {          "ITEM_FIELD_PAD", 29,  1, 1,  0},
};

static ObjDataDescriptor s_containerBaseDescriptors[3] = {
    {"CONTAINER_FIELD_NUM_SLOTS", 0,  1, 1, 1},
    {      "CONTAINER_ALIGN_PAD", 1,  1, 5, 0},
    {   "CONTAINER_FIELD_SLOT_1", 2, 40, 4, 1},
};

static ObjDataDescriptor s_unitBaseDescriptors[64] = {
    {                     "UNIT_FIELD_CHARM",   0,  2, 4,   1},
    {                    "UNIT_FIELD_SUMMON",   2,  2, 4,   1},
    {                 "UNIT_FIELD_CHARMEDBY",   4,  2, 4,   1},
    {                "UNIT_FIELD_SUMMONEDBY",   6,  2, 4,   1},
    {                 "UNIT_FIELD_CREATEDBY",   8,  2, 4,   1},
    {                    "UNIT_FIELD_TARGET",  10,  2, 4,   1},
    {              "UNIT_FIELD_COMBO_TARGET",  12,  2, 4,   1},
    {            "UNIT_FIELD_CHANNEL_OBJECT",  14,  2, 4,   1},
    {                    "UNIT_FIELD_HEALTH",  16,  1, 1,   1},
    {                    "UNIT_FIELD_POWER1",  17,  1, 1,   1},
    {                    "UNIT_FIELD_POWER2",  18,  1, 1,   1},
    {                    "UNIT_FIELD_POWER3",  19,  1, 1,   1},
    {                    "UNIT_FIELD_POWER4",  20,  1, 1,   1},
    {                 "UNIT_FIELD_MAXHEALTH",  21,  1, 1,   1},
    {                 "UNIT_FIELD_MAXPOWER1",  22,  1, 1,   1},
    {                 "UNIT_FIELD_MAXPOWER2",  23,  1, 1,   1},
    {                 "UNIT_FIELD_MAXPOWER3",  24,  1, 1,   1},
    {                 "UNIT_FIELD_MAXPOWER4",  25,  1, 1,   1},
    {                     "UNIT_FIELD_LEVEL",  26,  1, 1,   1},
    {           "UNIT_FIELD_FACTIONTEMPLATE",  27,  1, 1,   1},
    {                   "UNIT_FIELD_BYTES_0",  28,  1, 5,   1},
    {                     "UNIT_FIELD_STAT0",  29,  1, 1,   2},
    {                     "UNIT_FIELD_STAT1",  30,  1, 1,   2},
    {                     "UNIT_FIELD_STAT2",  31,  1, 1,   2},
    {                     "UNIT_FIELD_STAT3",  32,  1, 1,   2},
    {                     "UNIT_FIELD_STAT4",  33,  1, 1,   2},
    {                 "UINT_FIELD_BASESTAT0",  34,  1, 1,   2},
    {                 "UINT_FIELD_BASESTAT1",  35,  1, 1,   2},
    {                 "UINT_FIELD_BASESTAT2",  36,  1, 1,   2},
    {                 "UINT_FIELD_BASESTAT3",  37,  1, 1,   2},
    {                 "UINT_FIELD_BASESTAT4",  38,  1, 1,   2},
    {       "UNIT_VIRTUAL_ITEM_SLOT_DISPLAY",  39,  3, 1,   1},
    {               "UNIT_VIRTUAL_ITEM_INFO",  42,  6, 5,   1},
    {                     "UNIT_FIELD_FLAGS",  48,  1, 1,   1},
    {                   "UNIT_FIELD_COINAGE",  49,  1, 1,   2},
    {                      "UNIT_FIELD_AURA",  50, 56, 1,   1},
    {                 "UNIT_FIELD_AURAFLAGS", 106,  7, 5,   1},
    {                 "UNIT_FIELD_AURASTATE", 113,  1, 1,   1},
    {           "UNIT_FIELD_MOD_DAMAGE_DONE", 114,  6, 1,   2},
    {          "UNIT_FIELD_MOD_DAMAGE_TAKEN", 120,  6, 1,   2},
    {  "UNIT_FIELD_MOD_CREATURE_DAMAGE_DONE", 126,  8, 1,   2},
    {            "UNIT_FIELD_BASEATTACKTIME", 134,  2, 1,   1},
    {               "UNIT_FIELD_RESISTANCES", 136,  6, 1,   2},
    {            "UNIT_FIELD_BOUNDINGRADIUS", 142,  1, 3,   1},
    {               "UNIT_FIELD_COMBATREACH", 143,  1, 3,   1},
    {               "UNIT_FIELD_WEAPONREACH", 144,  1, 3,   1},
    {                 "UNIT_FIELD_DISPLAYID", 145,  1, 1,   1},
    {            "UNIT_FIELD_MOUNTDISPLAYID", 146,  1, 1,   1},
    {                    "UNIT_FIELD_DAMAGE", 147,  1, 2,   1},
    {"UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE", 148,  6, 1,   2},
    {"UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE", 154,  6, 1,   2},
    {        "UNIT_FIELD_RESISTANCEITEMMODS", 160,  6, 1,   2},
    {                   "UINT_FIELD_BYTES_1", 166,  1, 5,   1},
    {                 "UNIT_FIELD_PETNUMBER", 167,  1, 1,   1},
    {        "UNIT_FIELD_PET_NAME_TIMESTAMP", 168,  1, 1,   1},
    {             "UNIT_FIELD_PETEXPERIENCE", 169,  1, 1,   4},
    {           "UNIT_FIELD_PETNEXTLEVELEXP", 170,  1, 1,   4},
    {                   "UNIT_DYNAMIC_FLAGS", 171,  1, 1, 256},
    {                     "UNIT_EMOTE_STATE", 172,  1, 1,   1},
    {                   "UNIT_CHANNEL_SPELL", 173,  1, 1,   1},
    {                  "UNIT_MOD_CAST_SPEED", 174,  1, 1,   1},
    {                "UNIT_CREATED_BY_SPELL", 175,  1, 1,   1},
    {                   "UNIT_FIELD_BYTES_2", 176,  1, 5,   2},
    {                   "UNIT_FIELD_PADDING", 177,  1, 1,   0},
};

static ObjDataDescriptor s_playerBaseDescriptors[27] = {
    {    "PLAYER_FIELD_INV_SLOT_1",   0,  46, 4,  1},
    {   "PLAYER_FIELD_PACK_SLOT_1",  46,  32, 4, 18},
    {   "PLAYER_FIELD_BANK_SLOT_1",  78,  48, 4,  2},
    {"PLAYER_FIELD_BANKBAG_SLOT_1", 126,  12, 4,  2},
    {           "PLAYER_SELECTION", 138,   2, 4,  1},
    {            "PLAYER_FARSIGHT", 140,   2, 4,  2},
    {        "PLAYER_DUEL_ARBITER", 142,   2, 4,  1},
    { "PLAYER_FIELD_NUM_INV_SLOTS", 144,   1, 1,  1},
    {             "PLAYER_GUILDID", 145,   1, 1,  1},
    {           "PLAYER_GUILDRANK", 146,   1, 1,  1},
    {               "PLAYER_BYTES", 147,   1, 5,  1},
    {                  "PLAYER_XP", 148,   1, 1,  2},
    {       "PLAYER_NEXT_LEVEL_XP", 149,   1, 1,  2},
    {      "PLAYER_SKILL_INFO_1_1", 150, 192, 2,  2},
    {             "PLAYER_BYTES_2", 342,   1, 5,  1},
    {       "PLAYER_QUEST_LOG_1_1", 343,  96, 1,  2},
    {   "PLAYER_CHARACTER_POINTS1", 439,   1, 1,  2},
    {   "PLAYER_CHARACTER_POINTS2", 440,   1, 1,  2},
    {     "PLAYER_TRACK_CREATURES", 441,   1, 1,  2},
    {     "PLAYER_TRACK_RESOURCES", 442,   1, 1,  2},
    {        "PLAYER_CHAT_FILTERS", 443,   1, 1,  2},
    {           "PLAYER_DUEL_TEAM", 444,   1, 1,  1},
    {    "PLAYER_BLOCK_PERCENTAGE", 445,   1, 3,  2},
    {    "PLAYER_DODGE_PERCENTAGE", 446,   1, 3,  2},
    {    "PLAYER_PARRY_PERCENTAGE", 447,   1, 3,  2},
    {           "PLAYER_BASE_MANA", 448,   1, 1,  2},
    {     "PLAYER_GUILD_TIMESTAMP", 449,   1, 1,  1},
};

static ObjDataDescriptor s_gameObjectBaseDescriptors[11] = {
    {"GAMEOBJECT_DISPLAYID",  0, 1, 1,   1},
    {    "GAMEOBJECT_FLAGS",  1, 1, 1,   1},
    { "GAMEOBJECT_ROTATION",  2, 4, 3,   1},
    {    "GAMEOBJECT_STATE",  6, 1, 1,   1},
    {"GAMEOBJECT_TIMESTAMP",  7, 1, 1,   1},
    {    "GAMEOBJECT_POS_X",  8, 1, 3,   1},
    {    "GAMEOBJECT_POS_Y",  9, 1, 3,   1},
    {    "GAMEOBJECT_POS_Z", 10, 1, 3,   1},
    {   "GAMEOBJECT_FACING", 11, 1, 3,   1},
    {"GAMEOBJECT_DYN_FLAGS", 12, 1, 1, 256},
    {  "GAMEOBJECT_FACTION", 13, 1, 1,   1},
};

static ObjDataDescriptor s_dynamicObjectBaseDescriptors[9] = {
    { "DYNAMICOBJECT_CASTER", 0, 2, 4, 1},
    {  "DYNAMICOBJECT_BYTES", 2, 1, 5, 1},
    {"DYNAMICOBJECT_SPELLID", 3, 1, 1, 1},
    { "DYNAMICOBJECT_RADIUS", 4, 1, 3, 1},
    {  "DYNAMICOBJECT_POS_X", 5, 1, 3, 1},
    {  "DYNAMICOBJECT_POS_Y", 6, 1, 3, 1},
    {  "DYNAMICOBJECT_POS_Z", 7, 1, 3, 1},
    { "DYNAMICOBJECT_FACING", 8, 1, 3, 1},
    {    "DYNAMICOBJECT_PAD", 9, 1, 5, 1},
};

static ObjDataDescriptor s_corpseBaseDescriptors[11] = {
    {     "CORPSE_FIELD_OWNER",  0,  2, 4, 1},
    {    "CORPSE_FIELD_FACING",  2,  1, 3, 1},
    {     "CORPSE_FIELD_POS_X",  3,  1, 3, 1},
    {     "CORPSE_FIELD_POS_Y",  4,  1, 3, 1},
    {     "CORPSE_FIELD_POS_Z",  5,  1, 3, 1},
    {"CORPSE_FIELD_DISPLAY_ID",  6,  1, 1, 1},
    {      "CORPSE_FIELD_ITEM",  7, 19, 1, 1},
    {   "CORPSE_FIELD_BYTES_1", 26,  1, 5, 1},
    {   "CORPSE_FIELD_BYTES_2", 27,  1, 5, 1},
    {     "CORPSE_FIELD_GUILD", 28,  1, 1, 1},
    {     "CORPSE_FIELD_LEVEL", 29,  1, 1, 1},
};

static ObjDataDescriptor s_objDescriptors[6];
static ObjDataDescriptor s_itemDescriptors[36];
static ObjDataDescriptor s_containerDescriptors[78];
static ObjDataDescriptor s_unitDescriptors[184];
static ObjDataDescriptor s_playerDescriptors[634];
static ObjDataDescriptor s_gameObjectDescriptors[20];
static ObjDataDescriptor s_dynamicObjectDescriptors[16];
static ObjDataDescriptor s_corpseDescriptors[36];

static ObjDataDescriptor *const s_descriptors[8] = {
    s_objDescriptors,    s_itemDescriptors,       s_containerDescriptors,     s_unitDescriptors,
    s_playerDescriptors, s_gameObjectDescriptors, s_dynamicObjectDescriptors, s_corpseDescriptors,
};

static void __fastcall CopyAndExpandDescriptors(ObjDataDescriptor *dest, ObjDataDescriptor *source, unsigned int num, unsigned int destArraySize) {
  unsigned int d = 0;

  while (num--) {
    unsigned int fieldSize = source->fieldSize;

    while (fieldSize--) {
      dest[d++] = *source;
    }
    ++source;
  }

  ASSERT(d == destArraySize);
}

void __fastcall MirrorInitialize() {
  CopyAndExpandDescriptors(s_objDescriptors, s_objBaseDescriptors, 5, 6);

  memcpy(s_itemDescriptors, s_objDescriptors, sizeof(s_objDescriptors));
  CopyAndExpandDescriptors(s_itemDescriptors + 6, s_itemBaseDescriptors, 9, 30);

  memcpy(s_containerDescriptors, s_itemDescriptors, sizeof(s_itemDescriptors));
  CopyAndExpandDescriptors(s_containerDescriptors + 36, s_containerBaseDescriptors, 3, 42);

  memcpy(s_unitDescriptors, s_objDescriptors, sizeof(s_objDescriptors));
  CopyAndExpandDescriptors(s_unitDescriptors + 6, s_unitBaseDescriptors, 64, 178);

  memcpy(s_playerDescriptors, s_unitDescriptors, sizeof(s_unitDescriptors));
  CopyAndExpandDescriptors(s_playerDescriptors + 184, s_playerBaseDescriptors, 27, 450);

  memcpy(s_gameObjectDescriptors, s_objDescriptors, sizeof(s_objDescriptors));
  CopyAndExpandDescriptors(s_gameObjectDescriptors + 6, s_gameObjectBaseDescriptors, 11, 14);

  memcpy(s_dynamicObjectDescriptors, s_objDescriptors, sizeof(s_objDescriptors));
  CopyAndExpandDescriptors(s_dynamicObjectDescriptors + 6, s_dynamicObjectBaseDescriptors, 9, 10);

  memcpy(s_corpseDescriptors, s_objDescriptors, sizeof(s_objDescriptors));
  CopyAndExpandDescriptors(s_corpseDescriptors + 6, s_corpseBaseDescriptors, 11, 30);
}

const ObjDataDescriptor* __fastcall MirrorGetObjDataDescriptor(OBJECT_TYPE type, unsigned int blockID) {
    // TODO: implement
    return 0;
}
