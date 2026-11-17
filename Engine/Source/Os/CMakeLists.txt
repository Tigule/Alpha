if(BUILD_PORTABLE_MIDI)
    set(OS_MIDI_SOURCES W32/OsSoundMIDISynth.cpp W32/OsSoundSynth.cpp)
else()
    set(OS_MIDI_SOURCES W32/OsSoundMIDI.cpp)
endif()

if(APPLE)
    add_library(Os STATIC
        Mac/Debugging.cpp
        Mac/Errors.cpp
        Mac/Input.cpp
        Mac/InputMac.mm
        Mac/OsCall.cpp
        Mac/OsClipboard.cpp
        Mac/OsFile.cpp
        Mac/OsGui.cpp
        Mac/OsIME.cpp
        Mac/OsJoystick.cpp
        Mac/OsMemory.cpp
        Mac/OsNet.cpp
        Mac/OSSystem.cpp
        Mac/Path.cpp
        Mac/Time.cpp
        Mac/Tls.cpp
        W32/OsISoundCache.cpp
        W32/OsSound.cpp
        ${OS_MIDI_SOURCES}
    )
elseif(UNIX)
    add_library(Os STATIC
        Linux/Debugging.cpp
        Linux/Errors.cpp
        Linux/InputSDL.cpp
        Linux/OsClipboard.cpp
        Linux/OsFile.cpp
        Linux/OsGui.cpp
        Linux/OsNet.cpp
        Linux/OSSystem.cpp
        Linux/Time.cpp
        Mac/OsCall.cpp
        Mac/OsIME.cpp
        Mac/OsJoystick.cpp
        Mac/OsMemory.cpp
        Mac/Path.cpp
        Mac/Tls.cpp
        W32/OsISoundCache.cpp
        W32/OsSound.cpp
        ${OS_MIDI_SOURCES}
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
        ${OS_MIDI_SOURCES}
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
    target_link_libraries(Os PUBLIC fmod "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation")
elseif(UNIX)
    target_link_libraries(Os PUBLIC fmod PkgConfig::SDL2 X11)
elseif(NOT HAVE_WIN8_SDK)
    target_include_directories(Os PRIVATE "$ENV{DXSDK_DIR}/Include")
elseif(NOT BUILD_PORTABLE_MIDI)
    set_property(SOURCE W32/OsSoundMIDI.cpp APPEND PROPERTY INCLUDE_DIRECTORIES "$ENV{DXSDK_DIR}/Include")
endif()

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Os PRIVATE /Gr /Zi)
endif()
