add_library(Glue STATIC
    CGlueMgr.cpp
    CharCreateInfo.cpp
    CharSelectInfo.cpp
    GlueScriptEvents.cpp
)

target_include_directories(Glue PRIVATE
    ..
    ../../../Engine/Source
    ../../../Storm/H

    ../../Common
    ../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Glue PRIVATE /Gr /Z7)
endif()
