add_library(SRP STATIC
    SHA.cpp
    SRP6.cpp
)

target_include_directories(SRP PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(SRP PRIVATE /Gr /Z7)
endif()
