add_library(Magic STATIC
    SpellNet.cpp
)

target_include_directories(Magic PRIVATE
    ../../../Engine/Source
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Magic PRIVATE /Gr /Z7)
endif()
