add_library(WowServices STATIC
    misc.cpp
    WDataStore.cpp
    WowConnection.cpp
    WowConnectionNet.cpp
    Linux/WowConnectionNetSelect.cpp
)

target_include_directories(WowServices PRIVATE
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowServices PRIVATE /Gr /Z7)
endif()
