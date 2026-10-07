add_library(ObjectClient STATIC
    Bag_C.cpp
    CollideDisplay.cpp
    Container_C.cpp
    Corpse_C.cpp
    DynamicObject_C.cpp
    GameObject_C.cpp
    ../../../Common/GameObjectDefs.cpp
    GuildStats_C.cpp
    Item_C.cpp
    IUnitEffects.cpp
    Movement_C.cpp
    NPC_C.cpp
    Object_C.cpp
    Player_C.cpp
    PlayerGM_C.cpp
    PlayerSound_C.cpp
    Trade_C.cpp
    Unit_C.cpp
    UnitCombat_C.cpp
    UnitCombatLog_C.cpp
    UnitEffects.cpp
    UnitFootprints.cpp
    UnitSound_C.cpp
    ZoneDebug.cpp
)

target_include_directories(ObjectClient PRIVATE
    ../..
    ../../../Common
    ../../../../Engine/Source
    ../../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(ObjectClient PRIVATE /Gr /Z7)
endif()
