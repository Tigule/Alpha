add_library(MagicClient STATIC
    Spell_C.cpp
    SpellTable.cpp
    SpellVisuals.cpp
)

target_include_directories(MagicClient PRIVATE
    ../..
    ../../../Common
    ../../../../Engine/Source
    ../../../../Storm/H

    ../../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(MagicClient PRIVATE /Gr /Z7)
endif()
