#include <Component/CharacterCustomization.h>
#include <InventoryConst.h>
#include <ComponentCore/ComponentUtilCore.h>
#include <DB/WowClientDB.h>

WOW_LOCALE CURRENT_LANGUAGE;

struct VARIATIONSECTIONS {
  CHARTEXTURESECTIONID section[4];
};

enum AI_GROUP_ACTION {
  AI_GROUP_SPAWN,
  AI_GROUP_IDLE,
  AI_GROUP_MOVETO,
  AI_GROUP_TELEPORT,
  AI_GROUP_WANDER,
  AI_GROUP_WANDER_SPAWNRELATIVE_OBSOLETE,
  AI_GROUP_WANDER_AREA,
  AI_GROUP_FOLLOW_GUID,
  AI_GROUP_FOLLOW_PATH,
  AI_GROUP_PATROL_LINE,
  AI_GROUP_PATROL_CIRCLE,
  AI_GROUP_GUARD_GUID,
  AI_GROUP_GUARD_AREA,
  AI_GROUP_SET_FORMATION,
  AI_GROUP_UNIT_CHANGE_MODE_OBSOLETE,
  AI_GROUP_UNIT_SAY,
  AI_GROUP_UNIT_CAST,
  AI_GROUP_UNIT_ACTIVATE_OBJECT,
  AI_GROUP_GENERATE_EVENT,
  AI_GROUP_DESPAWN,
  AI_GROUP_SET_RADIUS_OBSOLETE,
  AI_GROUP_SET_FACTION_OBSOLETE,
  AI_GROUP_UNIT_SET_FACING,
  AI_GROUP_UNIT_FACE_GUID,
  AI_GROUP_UNIT_EMOTE,
  AI_GROUP_MOVETO_GUID,
  AI_GROUP_ATTACK_GUID,
  AI_GROUP_UNIT_MOUNT,
  AI_GROUP_UNIT_DISMOUNT,
  AI_GROUP_BEASTMASTER_ON,
  AI_GROUP_BEASTMASTER_OFF,
  AI_GROUP_UNIT_MODE,
  AI_GROUP_UNIT_MODE_RESET,
  AI_GROUP_UNIT_FACTION,
  AI_GROUP_UNIT_FACTION_RESET,
  AI_GROUP_UNIT_RADIUS,
  AI_GROUP_UNIT_RADIUS_RESET,
  AI_GROUP_QUEST_COMPLETE,
  AI_GROUP_UNIT_QUESTGIVER,
  AI_GROUP_UNIT_TRAINER,
  AI_GROUP_SPLINE_PATH,
  AI_GROUP_PLAYER_ACTION,
  AI_GROUP_RETURN_HOME,
  AI_GROUP_UNIT_SAY_RANDOM,
  AI_GROUP_UNIT_YELL,
  AI_GROUP_UNIT_YELL_RANDOM,
  AI_GROUP_UNIT_SET_ITEM_MAINHAND,
  AI_GROUP_UNIT_RESET_ITEM_MAINHAND,
  AI_GROUP_UNIT_CHAT_EMOTE,
  AI_GROUP_UNIT_CHAT_EMOTE_RANDOM,
  AI_GROUP_UNIT_GENERATE_EVENT,
  AI_GROUP_VENDOR_IDLE_OBSOLETE,
  AI_GROUP_QUEST_FAILED,
  AI_GROUP_UNIT_TRIGGERS,
  AI_GROUP_UNIT_TRIGGERS_RESET,
  AI_GROUP_UNIT_LEAVE_COMBAT,
  AI_GROUP_IDLE_COMBAT_START,
  AI_GROUP_IDLE_COMBAT_STOP,
  AI_GROUP_UNIT_IMMUNEPC,
  AI_GROUP_UNIT_IMMUNEPC_RESET,
  AI_GROUP_UNIT_IMMUNENPC,
  AI_GROUP_UNIT_IMMUNENPC_RESET,
  AI_GROUP_UNIT_UNKILLABLE,
  AI_GROUP_UNIT_UNKILLABLE_RESET,
  AI_GROUP_UNIT_SPELLS,
  AI_GROUP_UNIT_SPELLS_RESET,
  AI_GROUP_REUSEME,
  AI_GROUP_UNIT_SEND_LOCAL_EVENT,
  AI_GROUP_UNIT_BROADCAST_LOCAL_EVENT,
  AI_GROUP_UNIT_FLEE,
  AI_GROUP_UNIT_RETREAT,
  AI_GROUP_OBJECT_CHAT_EMOTE,
  AI_GROUP_OBJECT_CHAT_EMOTE_RANDOM,
  AI_GROUP_AVOID,
  AI_GROUP_AVOID_GUID,
  AI_GROUP_OBJECT_ACTIVATE,
  AI_GROUP_UNIT_ACTIVATE_OBJECTS,
  AI_GROUP_UNIT_STRINGID,
  AI_GROUP_UNIT_STRINGID_RESET,
  AI_GROUP_PERIODIC_EVENT,
  AI_GROUP_UNIT_SET_ITEM_OFFHAND,
  AI_GROUP_UNIT_RESET_ITEM_OFFHAND,
  AI_GROUP_UNIT_SET_ITEM_RANGED,
  AI_GROUP_UNIT_RESET_ITEM_RANGED,
  AI_GROUP_UNIT_SHEATHE,
  AI_GROUP_UNIT_UNSHEATHE,
  AI_GROUP_UNIT_CANCEL_CAST,
  AI_GROUP_UNIT_CANCEL_AURA,
  AI_GROUP_UNIT_FINISH_CAST,
  AI_GROUP_EMOTE_STATE,
  AI_GROUP_UNIT_CALL_FOR_HELP,
  AI_GROUP_FLIGHT_PATH,
  AI_GROUP_UNIT_COMBAT_TRIGGER,
  NUM_AI_GROUP_ACTIONS
};

extern LPCSTR const s_inventoryTypeNames[] = {
    "NonEquip", "Head", "Neck", "Shoulder", "Body", "Chest", "Waist", "Legs", "Feet", "Wrist", "Hand", "Finger", "Trinket", "Weapon", "Shield",
    "Ranged", "Cloak", "2 Hand", "Bag", "Tabard", "Robe", "Main Hand Weapon", "Off-Hand Weapon", "Holdable", "Ammo", "Thrown", "RangedRight"
};

extern LPCSTR const itemSpellTriggers[] = {"OnUse", "OnEquip", "OnProc", "SummonedBySpell", "OnDeath"};

extern LPCSTR const bonusStatFields[] = {
    "Mana", "Health", "Endurance", "Agility", "Strength", "Intellect", "Spirit", "Stamina", "Energy", "Rage", "Focus"
};

extern LPCSTR const damageTypeFields[] = {"Blunt", "Slash", "Pierce"};

extern LPCSTR const damageClasses[] = {"Weapon", "Fire", "Nature", "Frost", "Shadow", "Holy"};

extern LPCSTR const geoSetLayerFields[] = {"No Layer", "Layer 1", "Layer 2", "Layer 3"};

extern LPCSTR const s_questEventStrings[] = {
    "questStarted", "questComplete", "killMonster", "visitArea", "visitNPC", "giveItem", "seeMonster", "receiveItem", "worldObject", "timeElapsed",
    "NPCDeath", "NPCVisitArea", "timeOfDay"
};

extern LPCSTR const s_questActionStrings[] = {"completePart", "giveItem", "rewardQuest"};

extern LPCSTR const g_skillTypeStrings[] = {"Class", "Specialization", "Racial", "Primary", "Secondary"};

extern LPCSTR const g_groupActions[] = {
    "Spawn", "Idle", "Move within a certain radius of a point", "Teleport to a point in the world", "Wander a certain radius of a point",
    "Wander a certain radius of the spawn point (obsolete)", "Wander a convex poly area (not yet implemented)", "Follow a unit", "Follow a path",
    "Patrol back and forth along a path", "Patrol in a circular path", "Guard a unit", "Guard an area (not yet implemented)",
    "Set a specific formation and spread", "Change mode on unit(s) in the group (obsolete)", "Unit(s) say something", "Unit(s) cast a spell",
    "Unit(s) activate a game object", "Generate an event", "Despawn", "Set the group radius record (obsolete)",
    "Set the group faction template (obsolete)", "Unit(s) face a particular angle", "Unit(s) face a unit or game object", "Unit(s) perform an emote",
    "Move within a certain radius of a unit or game object", "Attack unit", "Unit(s) mount a creature", "Unit(s) dismount",
    "Turn on beastmaster flag for the group", "Turn off beastmaster flag for the group", "Change mode for unit(s)", "Reset mode for unit(s)",
    "Change faction template for unit(s)", "Reset faction template for unit(s)", "Change radius record for unit(s)",
    "Reset radius record for unit(s)", "Quest actions completed", "Unit(s) become a quest giver", "Unit(s) become a trainer",
    "Follow a short path exactly", "Player performs an action", "Move everybody back to spawn positions", "Unit(s) say something random",
    "Unit(s) yell something", "Unit(s) yell something random", "Set the item in the mainhand slot for unit(s)",
    "Reset the item in the mainhand slot for unit(s)", "Unit(s) chat emote something", "Unit(s) chat emote something random",
    "Unit in the group generates an event", "Vendor wander + face + idle action sequence (obsolete)", "Quest actions failed",
    "Change trigger action record for unit(s)", "Reset trigger action record for unit(s)", "Leave combat (combat action only)",
    "Idle until group goes into combat", "Idle until group returns from combat", "Set \"ImmunePC\" flag for unit(s)",
    "Reset \"ImmunePC\" flag for unit(s)", "Set \"ImmuneNPC\" flag for unit(s)", "Reset \"ImmuneNPC\" flag for unit(s)",
    "Set \"Unkillable\" flag for unit(s)", "Reset \"Unkillable\" flag for unit(s)", "Set the spells record for unit(s)",
    "Reset the spells record for unit(s)", "REUSEME", "Unit(s) send a local event", "Unit(s) broadcast a local event",
    "Flee combat (combat action only)", "Retreat (combat action only)", "Object(s) chat emote something", "Object(s) chat emote something random",
    "Move outside a certain radius of a point", "Move outside a certain radius of a unit or game object", "Activate object(s) in the group",
    "Unit(s) activate nearby game objects", "Set the string ID for unit(s)", "Reset the string ID for unit(s)", "Set a periodic event for the group",
    "Set the item in the offhand slot for unit(s)", "Reset the item in the offhand slot for unit(s)", "Set the item in the ranged slot for unit(s)",
    "Reset the item in the ranged slot for unit(s)", "Unit(s) sheathe their weapons", "Unit(s) unsheathe their weapons",
    "Unit(s) cancel casting a spell", "Unit(s) cancel an aura on themselves", "Unit(s) wait until they finish casting",
    "All units continually emote", "Call for help (combat action only)", "Follow a flight spline path exactly", "Trigger actions on units in combat"
};

extern LPCSTR const g_groupActionsAbbr[] = {
    "Spawn", "Idle", "Move", "Teleport", "WRpoint", "WRadius", "WPoly", "FGUID", "FPath", "PLinear", "PCirc", "GGUID", "GArea", "Formation",
    "ChangeMode", "USay", "UCast", "UActivateObject", "CEvent", "Despawn", "Radius", "Faction", "UFacing", "UFaceGUID", "UEmote", "MGUID", "AGUID",
    "UMount", "UDismount", "BeastmasterON", "BeastmasterOFF", "UMode", "UModeReset", "UFaction", "UFactionReset", "URadius", "URadiusReset",
    "QuestComplete", "UQuestGiver", "UTrainer", "SPath", "PAction", "GoHome", "USayRandom", "UYell", "UYellRandom", "UMItem", "UMItemReset",
    "UChatEmote", "UChatEmoteRandom", "UEvent", "VendorIdle", "QuestFailed", "UTriggers", "UTriggersReset", "ULeaveCombat", "IdleCombatStart",
    "IdleCombatStop", "UImmunePC", "UImmunePCReset", "UImmuneNPC", "UImmuneNPCReset", "UUnkillable", "UUnkillableReset", "USpells", "USpellsReset",
    "REUSEME", "ULocalEvent", "UBLocalEvent", "UFlee", "URetreat", "OChatEmote", "OChatEmoteRandom", "Avoid", "AvoidGUID", "OActivate",
    "UActivateOOO", "UStringID", "UStringIDReset", "PeriodicEvent", "UOItem", "UOItemReset", "URItem", "URItemReset", "USheathe", "UUnsheathe",
    "UCancelCast", "UCancelAura", "UFinishCast", "EmoteState", "UCallForHelp", "FlightPath", "CombatTrigger"
};

extern const DWORD g_groupActionsFields[] = {
    0x0, 0x10, 0x26, 0x2, 0x2000096, 0x7fffffff, 0x7fffffff, 0x314, 0x10000028, 0x10000038, 0x20000038, 0x314, 0x7fffffff, 0x44, 0x7fffffff, 0x10081,
    0x40381, 0x4000381, 0x200080, 0x0, 0x7fffffff, 0x7fffffff, 0x81, 0x301, 0x400081, 0x324, 0x300, 0x80081, 0x1, 0x0, 0x0, 0x80081, 0x1, 0x1081,
    0x1, 0x2081, 0x1, 0x0, 0x4081, 0x8081, 0x10000028, 0x0, 0x0, 0x20081, 0x10081, 0x20081, 0x100081, 0x1, 0x10081, 0x20081, 0x200081, 0x7fffffff,
    0x0, 0x1000081, 0x1, 0x0, 0x10, 0x10, 0x81, 0x1, 0x81, 0x1, 0x81, 0x1, 0x800081, 0x1, 0x0, 0x200381, 0x200381, 0x10, 0x10, 0x10081, 0x20081,
    0x26, 0x334, 0x4000081, 0x4000381, 0x181, 0x81, 0x200090, 0x100081, 0x1, 0x100081, 0x1, 0x1, 0x1, 0x40081, 0x40081, 0x11, 0x400090, 0x300,
    0x50000028, 0x381
};

AI_GROUP_ACTION g_groupActionsList[] = {
    AI_GROUP_SPAWN,
    AI_GROUP_IDLE,
    AI_GROUP_IDLE_COMBAT_START,
    AI_GROUP_IDLE_COMBAT_STOP,
    AI_GROUP_MOVETO,
    AI_GROUP_AVOID,
    AI_GROUP_TELEPORT,
    AI_GROUP_WANDER,
    AI_GROUP_SPLINE_PATH,
    AI_GROUP_FLIGHT_PATH,
    AI_GROUP_FOLLOW_PATH,
    AI_GROUP_PATROL_LINE,
    AI_GROUP_PATROL_CIRCLE,
    AI_GROUP_SET_FORMATION,
    AI_GROUP_EMOTE_STATE,
    AI_GROUP_UNIT_MODE,
    AI_GROUP_UNIT_MODE_RESET,
    AI_GROUP_UNIT_FACTION,
    AI_GROUP_UNIT_FACTION_RESET,
    AI_GROUP_UNIT_RADIUS,
    AI_GROUP_UNIT_RADIUS_RESET,
    AI_GROUP_UNIT_SPELLS,
    AI_GROUP_UNIT_SPELLS_RESET,
    AI_GROUP_UNIT_IMMUNEPC,
    AI_GROUP_UNIT_IMMUNEPC_RESET,
    AI_GROUP_UNIT_IMMUNENPC,
    AI_GROUP_UNIT_IMMUNENPC_RESET,
    AI_GROUP_UNIT_UNKILLABLE,
    AI_GROUP_UNIT_UNKILLABLE_RESET,
    AI_GROUP_UNIT_TRIGGERS,
    AI_GROUP_UNIT_TRIGGERS_RESET,
    AI_GROUP_UNIT_STRINGID,
    AI_GROUP_UNIT_STRINGID_RESET,
    AI_GROUP_UNIT_QUESTGIVER,
    AI_GROUP_UNIT_TRAINER,
    AI_GROUP_UNIT_SET_ITEM_MAINHAND,
    AI_GROUP_UNIT_RESET_ITEM_MAINHAND,
    AI_GROUP_UNIT_SET_ITEM_OFFHAND,
    AI_GROUP_UNIT_RESET_ITEM_OFFHAND,
    AI_GROUP_UNIT_SET_ITEM_RANGED,
    AI_GROUP_UNIT_RESET_ITEM_RANGED,
    AI_GROUP_UNIT_SHEATHE,
    AI_GROUP_UNIT_UNSHEATHE,
    AI_GROUP_UNIT_SET_FACING,
    AI_GROUP_UNIT_FACE_GUID,
    AI_GROUP_UNIT_SAY,
    AI_GROUP_UNIT_SAY_RANDOM,
    AI_GROUP_UNIT_YELL,
    AI_GROUP_UNIT_YELL_RANDOM,
    AI_GROUP_UNIT_CHAT_EMOTE,
    AI_GROUP_UNIT_CHAT_EMOTE_RANDOM,
    AI_GROUP_UNIT_EMOTE,
    AI_GROUP_UNIT_CAST,
    AI_GROUP_UNIT_FINISH_CAST,
    AI_GROUP_UNIT_CANCEL_CAST,
    AI_GROUP_UNIT_CANCEL_AURA,
    AI_GROUP_UNIT_ACTIVATE_OBJECT,
    AI_GROUP_UNIT_ACTIVATE_OBJECTS,
    AI_GROUP_UNIT_MOUNT,
    AI_GROUP_UNIT_DISMOUNT,
    AI_GROUP_UNIT_CALL_FOR_HELP,
    AI_GROUP_UNIT_RETREAT,
    AI_GROUP_UNIT_FLEE,
    AI_GROUP_UNIT_LEAVE_COMBAT,
    AI_GROUP_OBJECT_ACTIVATE,
    AI_GROUP_OBJECT_CHAT_EMOTE,
    AI_GROUP_OBJECT_CHAT_EMOTE_RANDOM,
    AI_GROUP_MOVETO_GUID,
    AI_GROUP_FOLLOW_GUID,
    AI_GROUP_GUARD_GUID,
    AI_GROUP_AVOID_GUID,
    AI_GROUP_ATTACK_GUID,
    AI_GROUP_BEASTMASTER_ON,
    AI_GROUP_BEASTMASTER_OFF,
    AI_GROUP_GENERATE_EVENT,
    AI_GROUP_PERIODIC_EVENT,
    AI_GROUP_UNIT_GENERATE_EVENT,
    AI_GROUP_UNIT_SEND_LOCAL_EVENT,
    AI_GROUP_UNIT_BROADCAST_LOCAL_EVENT,
    AI_GROUP_UNIT_COMBAT_TRIGGER,
    AI_GROUP_QUEST_COMPLETE,
    AI_GROUP_QUEST_FAILED,
    AI_GROUP_RETURN_HOME,
    AI_GROUP_DESPAWN
};

extern LPCSTR const g_actionTriggers[] = {
    "-NONE-", "OnReaction", "OnEnterCombat", "OnLeaveCombat", "OnCombatTick", "OnHealthRange", "OnEnergyRange", "OnDeath", "OnSpell", "OnKill",
    "OnSpawn", "OnEmote", "OnMelee", "OnInteract", "OnCombatTrigger"
};

extern LPCSTR const g_aiMoveSpeed[] = {"Normal", "Hurry", "Meander"};

extern LPCSTR const g_aiFormations[] = {
    "Just random", "Single file, leader at the front", "Side by side, leader in the center", "Like geese", "Fanned out behind the leader",
    "Fanned out in front of the leader", "Circle the leader"
};

extern LPCSTR const g_aiWanderFacing[] = {"Random", "Current", "Spawn", "Angle"};

extern LPCSTR const g_actionPriorities[] = {"-NONE-", "Low", "Medium", "High"};

extern LPCSTR const g_eventTypes[] = {"Spawn", "General event (designer defined)", "Time event", "Quest event", "Area trigger event"};

extern LPCSTR const g_aiGroupUnits[] = {
    "Random Unit", "All Units", "Unit 1", "Unit 2", "Unit 3", "Unit 4", "Unit 5", "Unit 6", "Unit 7", "Unit 8", "Unit 9", "Unit 10"
};

extern LPCSTR const g_unitReactions[] = {"Hated", "Hostile", "Unfriendly", "Neutral", "Amiable", "Friendly", "Revered"};

extern const int g_killReactionModifierPos[] = {3, 3, 3, 3, 3, 3, 3};

extern const int g_killReactionModifierNeg[] = {-15, -15, -15, -15, -60, -200, -400};

extern LPCSTR const g_factionFlags[] = {
    "Respond to call for help", "Broadcast to enemies (low priority)", "Broadcast to enemies (med priority)", "Broadcast to enemies (high priority)",
    "Search for enemies (low priority)", "Search for enemies (med priority)", "Search for enemies (high priority)",
    "Search for friends (low priority)", "Search for friends (med priority)", "Search for friends (high priority)", "Flee from call for help"
};

extern const ITEMGEOSETGROUPS g_geosetGroupsPerItem[INDEX_NUMSLOTS] = {
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_SLEEVES, CHARITEMGEOSETS_DOUBLET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_SLEEVES, CHARITEMGEOSETS_DOUBLET, CHARITEMGEOSETS_ROBE, CHARITEMGEOSETS_TABARD}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_PANTDOUBLET, CHARITEMGEOSETS_PANTS, CHARITEMGEOSETS_ROBE, CHARITEMGEOSETS_LOINCLOTH}},
    {{CHARITEMGEOSETS_BOOTS, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_GLOVES, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_TABARD, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{CHARITEMGEOSETS_SLEEVES, CHARITEMGEOSETS_DOUBLET, CHARITEMGEOSETS_ROBE, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}},
    {{INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET, INVALID_CHARITEMGEOSET}}
};

GEOCOMPONENTINFO g_geometryComponentLookups[INDEX_NUMSLOTS] = {
    {0x000, {-1, -1}, {-1, -1}},
    {0x800, {11, -1}, {11, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x060,   {6, 5},   {6, 5}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x002,  {1, -1},  {2, -1}},
    {0x001,  {0, -1},  {0, -1}},
    {0x004,  {2, -1},  {2, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x002,  {1, -1},  {1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x002,  {1, -1},  {1, -1}},
    {0x004,  {2, -1},  {2, -1}},
    {0x004,  {2, -1},  {2, -1}},
    {0x000, {-1, -1}, {-1, -1}},
    {0x002,  {1, -1},  {1, -1}},
    {0x002,  {1, -1},  {1, -1}}
};

extern const CHARACTER_GEOSET_SECTIONS g_clothingGeosetRanges[] = {
    CHARGEOSET_GLOVE, CHARGEOSET_BOOT, CHARGEOSET_SLEEVES, CHARGEOSET_PANTS, CHARGEOSET_DOUBLET, CHARGEOSET_PANTDOUBLET, CHARGEOSET_TABARD,
    CHARGEOSET_ROBE, CHARGEOSET_LOINCLOTH
};

extern const int g_defaultGeosets[] = {401, 501, -1, -1, -1, 1101, -1, 1301, 1401};

int g_charGeosetIsOverlay[NUM_CHARGEOSETS] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0};

INVENTORY_TYPES g_NPCItemDisplayInvTypes[10] = {INDEX_HEAD_TYPE, INDEX_SHOULDER_TYPE, INDEX_BODY_TYPE,  INDEX_CHEST_TYPE, INDEX_WAIST_TYPE,
                                                INDEX_LEGS_TYPE, INDEX_FEET_TYPE,     INDEX_WRIST_TYPE, INDEX_HAND_TYPE,  INDEX_TABARD_TYPE};

INVENTORY_SLOTS g_NPCItemDisplayInvSlots[10] = {INVSLOT_HEAD, INVSLOT_SHOULDER, INVSLOT_BODY,  INVSLOT_CHEST, INVSLOT_WAIST,
                                                INVSLOT_LEGS, INVSLOT_FEET,     INVSLOT_WRIST, INVSLOT_HAND,  INVSLOT_TABARD};

extern LPCSTR const g_triggerTypes[] = {
    "Message", "Teleport", "Target", "Event (Player)", "Event (Monster)", "Quest", "Cartography", "Activate Objects (Player)"
};

extern LPCSTR const g_spellVisualAttach[] = {"Head", "Chest", "Base", "Left Hand", "Right Hand", "Breath", "Special 1", "Special 2", "Special 3"};

extern LPCSTR const g_spellProcEvents[] = {
    "Proc - Deal Cmbt Dmg", "Proc - Take Cmbt Dmg", "Proc - Kill", "Proc - Heartbeat", "Proc - Dodge", "Proc - Parry", "Proc - Block",
    "Proc - Swing", "Proc - Spell Cast", "Proc - Spell Cast Sch.", "Proc - Spell Hit", "Proc - Spell Hit Sch."
};

extern LPCSTR const g_skillAcquireStrings[] = {"Learned", "Automatic"};

extern LPCSTR const g_spellAcquireStrings[] = {"Learned", "Automatic - Skill Rank", "Automatic - Char Level"};

extern LPCSTR const g_spellDefenseTypes[] = {"None", "Magical", "Melee", "Ranged"};

extern LPCSTR const g_spellCastUITypes[] = {"-NONE-", "Pet Training", "Disguises", "Inscribing"};

extern LPCSTR const g_auraCancelsFlags[] = {
    "Hostile Action Cancels", "Damage Cancels", "Action Cancels", "Moving Cancels", "Turning Cancels", "Anim Cancels", "Dismount Cancels",
    "Under Water Cancels", "Above Water Cancels", "Sheathing Cancels"
};

extern const int g_ITEMTYPEARRAY[27] = {0,     1,      2,     4,     8,       16,     32, 64,    128,   256,   512, 3072,   12288, 98304,
                                        65536, 131072, 16384, 32768, 7864320, 262144, 16, 32768, 65536, 65536, 0,   131072, 131072};

extern LPCSTR const unitAttributes[] = {"Strength", "Agility", "Stamina", "Intellect", "Spirit"};

extern LPCSTR const s_areaTableFlags[] = {"Emit Breath Particles", "Breath Particles Overrideparent"};

extern LPCSTR const s_lockTypeStrings[] = {"NONE", "ITEM", "SPELL"};

extern LPCSTR const s_lockOpenActionStrings[] = {"Open", "Open + Unlock", "Close", "Destroy", "Rebuild"};

extern LPCSTR const s_itemBindStrings[] = {
    "Not Bound", "Bind On Acquire", "Bind On Equip", "Bind On Use", "Quest Item", "Quest Item - Multi (OBS - use flag)",
    "Multi Drop (OBS - use flag)"
};

extern LPCSTR const s_monsterSizeClassStrings[] = {"small", "medium", "large", "giant", "colossal"};

extern LPCSTR const s_sheathing[] = {"None", "Shoulder", "Inverted", "Hip", "Shield"};

extern LPCSTR const s_vocalSounds[] = {
    "Inventory Full", "Out Of Ammo", "No Equip Level", "No Equip Ever", "Soul Bound No Drop", "Can't Use, Item Cooling", "Cant Drink More",
    "Cant Eat More", "Cant Invite", "Invite Busy", "Target Too Far", "Invalid Target", "Spell Cooling", "Cant Learn Spell", "Locked",
    "Can't Cast, No Mana", "Not While Dead", "Can't Loot", "Can't Create", "Can't Invite, Invitation Declined", "Can't Invite, Already in Group",
    "Can't Invite, Already in Guild", "Can't Buy, Can't Afford Bank Slot", "Can't Buy, Too Many Bank Slots", "Can't Eat, Moving",
    "Can't Put, Not A Bag", "Can't Put In Bag", "Can't Put, Wrong Slot", "Can't Put, Only Ammo in Bag", "Can't Put, Bag Full",
    "Can't Put, Item Max Count", "Can't Loot, Didn't Kill", "Can't Loot, Wrong Facing", "Can't Loot, Locked", "Can't Loot, Not Standing",
    "Can't Loot, Too Far", "Can't Attack, Wrong Direction", "Can't Attack, NotStanding", "Can't Attack, No Target", "Can't Afford, Not Enough Gold",
    "Can't Afford, Not Enough Money", "Can't Equip, No Skill", "Can't Equip, 2nd hand full", "Can't Equip, 2h skill missing",
    "Can't Equip, Not Equippable", "Can't Cast, Generic No Target", "Can't Cast, Out of Range", "Can't Use, Potion Cooling",
    "Can't Use, No Proficiency", "Can't Use, Must Equip Item", "Can't Use, Ability Cooling", "Can't Use, Generic", "Can't Use, Chest In Use",
    "Can't Use, Food Cooling", "Can't Use Taxi, No Money", "Can't Use, Locked", "Can't Use, No Equipment Slot", "Can't Use, Too Far", "Can't Swap",
    "Can't Trade, Soulbound", "Can't Equip, Not Owner", "Can't Open, Item Locked", "Guild Permissions", "Can't Cast, No Rage",
    "Can't Cast, No Energy", "Can't Cast, No Focus"
};

extern LPCSTR const s_instanceType[] = {"Not Instanced", "Party Instance", "Raid Instance"};

extern LPCSTR const s_hairTextureSet[] = {"Hair", "Facial Hair"};

extern const VARIATIONSECTIONS g_variationSectionIDs[] = {
    {CHARTEXTURESECTION_SKIN, CHARTEXTURESECTION_NAKEDSKINTORSO, CHARTEXTURESECTION_NAKEDSKINPELVIS, CHARTEXTURESECTION_SKINEXTRA},
    {CHARTEXTURESECTION_FACEUPPER, CHARTEXTURESECTION_FACELOWER, CHARTEXTURESECTION_NUM},
    {CHARTEXTURESECTION_HAIR, CHARTEXTURESECTION_SCALPUPPERHAIR, CHARTEXTURESECTION_SCALPLOWERHAIR},
    {CHARTEXTURESECTION_FACIALUPPERHAIR, CHARTEXTURESECTION_FACIALLOWERHAIR, CHARTEXTURESECTION_NUM}
};

const CHARTEXTUREVARIATIONS g_charTextureSectionMapping[CHARTEXTURESECTION_NUM] = {
    CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN, CHARTEXTUREVAR_SKIN,       CHARTEXTUREVAR_FACE,      CHARTEXTUREVAR_FACE,
    CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_HAIR, CHARTEXTUREVAR_FACIALHAIR, CHARTEXTUREVAR_FACIALHAIR
};

extern LPCSTR const g_itemStaticFlags[] = {
    "No Pickup", "Conjured", "Has Loot Table", "Exotic", "Deprecated", "OBSOLETE", "Player Cast", "No Equip Cooldown", "Intellect Bonus Instead",
    "Gift Wrap", "Uses Resources", "Multi Drop", "Brief Spelleffects", "Petition"
};

extern LPCSTR const g_creatureStaticFlags[] = {
    "Mountable", "No XP", "No Loot", "Unkillable", "Tameable", "ImmunePC", "ImmuneNPC", "CanWieldLoot", "Sessile", "Unselectable", "NoAutoRegen",
    "CorpseNone", "CorpseRaid", "Creator Loot", "No Defense", "No Spell Defense", "Tabard Vendor", "Combat Ping", "Aquatic", "Amphibious", "No Melee"
};

extern LPCSTR const g_trainerTypes[] = {"General", "Talents", "TradeSkills", "Pet"};

extern LPCSTR const g_cameraShakeTypes[] = {"Sine", "Decayed Sine"};

extern LPCSTR const g_cameraDirections[] = {"Camera Forward", "Camera Right", "Camera Up"};

LPCSTR g_localeID[NUM_LOCALES] = {"enUS", "koKR", "jaJP", "zhCN", "zhTW", "esES", "frFR", "deDE"};

LPCSTR const g_sexString[UNITSEX_LAST] = {"Male", "Female", "NOSEX"};

extern LPCSTR const g_itemDisplayInfoFlags[] = {"Emblazoned Tabard", "Hide Underwear"};

extern LPCSTR const g_soundEmitterType[] = {"One Shot", "Looping", "Randomize"};

extern LPCSTR const g_regionFlags[] = {"Start Region Disabled", "Dynamic Spawning"};

extern LPCSTR const g_charActionBar[] = {"-NONE-", "Spell", "Item"};

extern LPCSTR const g_questDifficultyRatings[] = {
    "No Exp", "Trivial", "Very Small", "Small", "Substandard", "Standard", "High", "Epic", "Placeholder1", "Placeholder2"
};

extern LPCSTR const g_siflagStrings[] = {
    "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "No Duplicates", "UNUSED", "UNUSED", "UNUSED", "Looping", "Vary Pitch", "Vary Volume", "UNUSED"
};

extern LPCSTR const g_powerTypes[] = {"Mana", "Rage", "Focus", "Energy"};

extern LPCSTR const g_creatureAges[] = {"Infant", "Young Child", "Young Adult", "Mature Adult", "Old", "Ancient", "Venerable/Supernatural"};

extern LPCSTR const g_creatureRanking[] = {"Slave/Untouchable", "Serf", "Peasant", "Merchant/PC", "Noble/King", "Emperor", "God"};

extern LPCSTR const g_characterSections[] = {
    "Skin", "Naked Skin Pelvis", "Naked Skin Torso", "Skin Extra", "Face Lower", "Face Upper", "Hair", "Scalp Lower Hair", "Scalp Upper Hair",
    "Facial Lower Hair", "Facial Upper Hair"
};

extern LPCSTR const g_itemSize[] = {"Miniscule", "Tiny", "Small", "Medium", "Large", "Huge", "Ludicrous"};

extern LPCSTR const g_pointOfInterestFlags[] = {
    "Show On Minimap", "Show Minimap Icon", "Show On Worldmap", "Worldmap Continent Zoom", "Worldmap World Zoom"
};

extern LPCSTR const g_spellScriptHooks[] = {
    "Validate", "Precast", "Targeted", "Cast", "Aura Applied", "Aura Removed", "Check Interrupt", "Get Duration", "Get Cast Time", "Heartbeat",
    "Take Combat Hit", "Deal Combat Hit", "Script Effect", "Take Spell Hit"
};

extern LPCSTR const g_spellArgs[] = {
    "(caster, spellrec)", "(caster, cast, ...)", "(caster, castinfo)", "(caster, cast, value)", "(unit, aura)", "(unit, aura, slot)", "*",
    "(victim, attacker, spell, value, value2)", "(unit, caster, spell)"
};

UINT g_spellArgsNeeded[] = {0, 1, 2, 2, 4, 4, 3, 0, 0, 5, 7, 7, 8, 7};

extern LPCSTR const g_creatureImpactSound[] = {"FLESH", "STONE", "WOOD", "ETHEREAL"};

LPCSTR g_emoteSpecProcNames[] = {"One Shot Emote", "Changes Stand State", "Sets Emote State"};

LPCSTR g_emoteFlagNames[] = {
    "Only while standing", "Never animate", "Do not sheath", "Talk anim talk", "Talk anim question", "Talk anim exclamation", "Talk anim shout",
    "Not while swimming", "Talk anim laugh", "Ok while sleeping or dead", "Disallow from client", "Client side sheathe", "Movement ends",
    "End emote state", "Only while still"
};

extern LPCSTR const g_creatureClassification[] = {"Normal", "Elite", "Rare Elite", "World Boss", "Rare"};
