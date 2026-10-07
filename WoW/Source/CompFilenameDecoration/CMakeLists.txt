add_library(CompFilenameDecoration STATIC
    CompDecoration.cpp
)

target_include_directories(CompFilenameDecoration PRIVATE
    ../../../Engine/Source
    ../../../Storm/H

    ..
    ../../Common
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(CompFilenameDecoration PRIVATE /Gr /Z7)
endif()
