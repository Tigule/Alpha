add_library(WowSvcsClient STATIC
    ClientServices.cpp
    FriendList.cpp
)

target_include_directories(WowSvcsClient PRIVATE
    ../..
    ../../../../Engine/Source
    ../../../../Storm/H

    ../../../Common
    ../../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowSvcsClient PRIVATE /Gr /Z7)
endif()
