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

    ../../../Storm/Source
)

if(NOT HAVE_WIN8_SDK)
    target_include_directories(Os PRIVATE "$ENV{DXSDK_DIR}/Include")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_property(SOURCE W32/OsSoundMIDI.cpp APPEND PROPERTY INCLUDE_DIRECTORIES "$ENV{DXSDK_DIR}/Include")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # dmusic was never ported to 64-bit
    set_property(SOURCE W32/OsSoundMIDI.cpp APPEND PROPERTY COMPILE_DEFINITIONS TIGULE_NO_AMBIENCE)
elseif(NOT EXISTS "$ENV{DXSDK_DIR}/Include/dmusici.h")
    message(STATUS "DXSDK_DIR has no DirectMusic headers (dmusici.h): building without ambience music")
    set_property(SOURCE W32/OsSoundMIDI.cpp APPEND PROPERTY COMPILE_DEFINITIONS TIGULE_NO_AMBIENCE)
endif()

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Os PRIVATE /Gr /Zi)
endif()
