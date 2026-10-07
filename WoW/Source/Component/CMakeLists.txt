add_library(Component STATIC
    CharacterCustomization.cpp
    Component.cpp
    ComponentUtil.cpp
)

target_include_directories(Component PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Component PRIVATE /Gr /Z7)
endif()
