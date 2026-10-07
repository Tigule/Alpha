add_library(NetClient STATIC
    NetClient.cpp
    ../NetInternal.cpp
)

target_include_directories(NetClient PRIVATE
    ../..
    ../../../../Engine/Source
    ../../../../Storm/H

    ../../../Common
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(NetClient PRIVATE /Gr /Z7)
endif()
