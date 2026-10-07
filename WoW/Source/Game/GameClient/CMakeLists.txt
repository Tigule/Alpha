add_library(GameClient STATIC
    ../../../Common/DayNight.cpp
    GameTime_C.cpp
    GuildClient.cpp
    ../../../Common/LightsAndFog.cpp
    Minimap.cpp
    NameCache.cpp
    PetNameCache.cpp
    PlayerName.cpp
    QuestCache.cpp
    shadow.cpp
    TaxiMap.cpp
    ValidateName_C.cpp
    ViolenceLevels.cpp
    WeaponTrails.cpp
    WorldText.cpp
)

target_include_directories(GameClient PRIVATE
    ../..
    ../../../../Engine/Source
    ../../../../Storm/H
    ../../../../Engine/Source/lua/include

    ../../../Common
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(GameClient PRIVATE /Gr /Z7)
endif()
