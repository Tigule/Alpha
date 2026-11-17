if(APPLE)
    add_library(Os STATIC
        Mac/Debugging.cpp
        Mac/Errors.cpp
        Mac/GlobalAUGraph.cpp
        Mac/Input.cpp
        Mac/InputMac.mm
        Mac/LoaderThread.cpp
        Mac/MIDIPlayer.cpp
        Mac/MP3Player.cpp
        Mac/OsCall.cpp
        Mac/OsClipboard.cpp
        Mac/OsFile.cpp
        Mac/OsGui.cpp
        Mac/OsIME.cpp
        Mac/OsJoystick.cpp
        Mac/OsMemory.cpp
        Mac/OsNet.cpp
        Mac/OsSound.cpp
        Mac/OSSystem.cpp
        Mac/Path.cpp
        Mac/Recycler.cpp
        Mac/StreamingCache.cpp
        Mac/Time.cpp
        Mac/Tls.cpp
        Mac/WAVPlayer.cpp
    )
else()
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
endif()

target_include_directories(Os PRIVATE
    .
    ..
    ../../../Storm/H

    ../../../Storm/Source
)

if(APPLE)
    target_link_libraries(Os PUBLIC "-framework AudioToolbox" "-framework AudioUnit" "-framework CoreAudio" "-framework CoreFoundation")
elseif(NOT HAVE_WIN8_SDK)
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
