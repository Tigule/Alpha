add_library(Object STATIC
    Collide.cpp
    CollideDebug.cpp
    Combat.cpp
    CreatureStats.cpp
    GameObject.cpp
    GameObjectStats.cpp
    GuildStats.cpp
    ItemStats.cpp
    mirror.cpp
    Movement.cpp
    NPCText.cpp
    Object.cpp
    PageTextCache.cpp
    Petition.cpp
    Unit.cpp
)

target_include_directories(Object PRIVATE
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Object PRIVATE /Gr /Z7 /YX)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES vc60.pch)
endif()
