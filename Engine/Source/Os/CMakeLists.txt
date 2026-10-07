add_library(Os STATIC
    W32/Debugging.cpp
    W32/Errors.cpp
    W32/Input.cpp
    W32/OsCall.cpp
    W32/OsClipboard.cpp
    W32/OsFile.cpp
    W32/OsGui.cpp
    W32/OsIME.cpp
    W32/OsISoundCache.cpp
    W32/OsJoystick.cpp
    W32/OsMemory.cpp
    W32/OsSound.cpp
    W32/OsSoundMIDI.cpp
    W32/OSSystem.cpp
    W32/OsTcp.cpp
    W32/Path.cpp
    W32/Time.cpp
    W32/Tls.cpp
)

target_include_directories(Os PRIVATE
    .
    ..
    ../../../Storm/H
    "$ENV{DXSDK_DIR}/Include"

    ../../../Storm/Source
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Os PRIVATE /Gr /Z7)
endif()
