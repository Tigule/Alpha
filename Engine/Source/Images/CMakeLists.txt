add_library(Images STATIC
    blit.cpp
    dxt.cpp
    funcs.cpp
    tga.cpp
)

target_include_directories(Images PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Images PRIVATE /Gr /Z7)
endif()
