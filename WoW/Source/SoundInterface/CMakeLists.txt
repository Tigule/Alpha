add_library(SoundInterface STATIC
    ISoundInterface.cpp
    SndDebug.cpp
    SndDebugIndoors.cpp
    SndDebugOutdoors.cpp
    SoundInterface.cpp
    SoundInterfaceDoodad.cpp
    SoundInterfaceGlueMusic.cpp
    SoundInterfaceMIDI.cpp
    SoundInterfaceProviderPrefs.cpp
    SoundInterfaceVocal.cpp
    SoundInterfaceWater.cpp
    SoundInterfaceZoneIntro.cpp
    SoundInterfaceZoneMusic.cpp
)

target_include_directories(SoundInterface PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(SoundInterface PRIVATE /Gr /Z7)
endif()
