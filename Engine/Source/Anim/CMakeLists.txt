add_library(Anim STATIC
    Anim.cpp
    Init.cpp
    Interface.cpp
    Interp.cpp
    MdlAnim.cpp
    Transform.cpp
    WorldMatrix.cpp
)

target_include_directories(Anim PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Anim PRIVATE /Gr /Z7)
endif()
