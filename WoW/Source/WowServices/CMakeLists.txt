add_library(WowServices STATIC
    misc.cpp
    WDataStore.cpp
    WowConnection.cpp
    WowConnectionNet.cpp
)

if(UNIX AND NOT APPLE)
    target_sources(WowServices PRIVATE Linux/WowConnectionNetRTSignal.cpp)
else()
    target_sources(WowServices PRIVATE Linux/WowConnectionNetSelect.cpp)
endif()

target_include_directories(WowServices PRIVATE
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowServices PRIVATE /Gr /Zi)
endif()
