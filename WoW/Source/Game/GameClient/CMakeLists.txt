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
    target_compile_options(GameClient PRIVATE /Gr /Zi)
    set(GAMECLIENT_PCH ${CMAKE_CURRENT_BINARY_DIR}/GameClient.pch)
    set(GAMECLIENT_PCH_FI /FIBase/Base.h /FIGx/Gx.h /FIMapDefs.h /FIWorldClient/World.h /FIWowConst.h /FIDayNight.h)

    get_target_property(GAMECLIENT_SOURCES GameClient SOURCES)
    set(GAMECLIENT_YX Minimap.cpp PlayerName.cpp shadow.cpp WeaponTrails.cpp WorldText.cpp)
    list(REMOVE_ITEM GAMECLIENT_SOURCES ${GAMECLIENT_YX})

    set_source_files_properties(${GAMECLIENT_YX} PROPERTIES COMPILE_OPTIONS /YX)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES vc60.pch)
    set_source_files_properties(${GAMECLIENT_SOURCES} PROPERTIES
        COMPILE_OPTIONS "${GAMECLIENT_PCH_FI};/YuDayNight.h;/Fp${GAMECLIENT_PCH}"
        OBJECT_DEPENDS ${GAMECLIENT_PCH})
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/GameClientPch.cpp "")

    target_sources(GameClient PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/GameClientPch.cpp)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/GameClientPch.cpp PROPERTIES
        COMPILE_OPTIONS "${GAMECLIENT_PCH_FI};/YcDayNight.h;/Fp${GAMECLIENT_PCH}"
        OBJECT_OUTPUTS ${GAMECLIENT_PCH})
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${GAMECLIENT_PCH})
endif()
