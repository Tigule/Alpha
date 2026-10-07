add_library(UIUtil STATIC
    Camera.cpp
    CSimpleCamera.cpp
    Cursor.cpp
    HealthBar.cpp
    InputControl.cpp
    PortraitButton.cpp
    ScriptEvents.cpp
    SmartScreenRects.cpp
    TextureCache.cpp
    Tooltip.cpp
)

target_include_directories(UIUtil PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(UIUtil PRIVATE /Gr /Z7)
endif()
