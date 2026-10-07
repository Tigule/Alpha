add_library(Model STATIC
    CollisionData.cpp
    CollisionDisplay.cpp
    MdlCamera.cpp
    MdlParticleEmitter2.cpp
    MdlRibbonEmitter.cpp
    MdlSgLight.cpp
    MdlSgModel.cpp
    MdlValidate.cpp
    ModelAnim.cpp
    ModelCreate.cpp
    ModelMisc.cpp
    ModelRender.cpp
)

target_include_directories(Model PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Model PRIVATE /Gr /Z7)
endif()
