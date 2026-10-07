add_library(WorldCommon STATIC
    WorldMath.cpp
)

target_include_directories(WorldCommon PRIVATE
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WorldCommon PRIVATE /Gr /Z7)
endif()
