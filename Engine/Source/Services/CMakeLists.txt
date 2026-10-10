add_library(Services STATIC
    AsyncFileRead.cpp
    Camera.cpp
    DataMgr.cpp
    GfxSingletonManager.cpp
    Lightning.cpp
    ParticleEmitters.cpp
    ParticleMisc.cpp
    ParticleSystem.cpp
    ParticleSystem2.cpp
    Profile.cpp
    RibbonEmitter.cpp
    SysMessage.cpp
    TextBlock.cpp
    Texture.cpp
)

target_include_directories(Services PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Services PRIVATE /Gr /Z7 /YX)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES vc60.pch)
endif()
