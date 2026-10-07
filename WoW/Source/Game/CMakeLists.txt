add_library(Game STATIC
    GameTime.cpp
    Loot.cpp
    ../../Common/WowTime.cpp
)

target_include_directories(Game PRIVATE
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Game PRIVATE /Gr /Z7)
endif()
