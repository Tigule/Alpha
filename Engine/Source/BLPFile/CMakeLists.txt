add_library(BLPFile STATIC
    blp.cpp
)

target_include_directories(BLPFile PRIVATE
    ../../Source
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(BLPFile PRIVATE /Gr /Z7)
endif()
