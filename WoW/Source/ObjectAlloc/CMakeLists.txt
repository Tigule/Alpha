add_library(ObjectAlloc STATIC
    ObjectAlloc.cpp
)

target_include_directories(ObjectAlloc PRIVATE
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(ObjectAlloc PRIVATE /Gr /Z7)
endif()
