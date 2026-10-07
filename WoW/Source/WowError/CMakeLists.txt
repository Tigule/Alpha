add_executable(WowError
    WowError.cpp
)

target_include_directories(WowError PRIVATE
    ../../../Storm/H
)

set_target_properties(WowError PROPERTIES OUTPUT_NAME WowErrorAE)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowError PRIVATE /Gr /Z7)
    target_link_libraries(WowError PRIVATE unicows msvcrt)
endif()

target_link_libraries(WowError PRIVATE
    kernel32
    user32
    advapi32
    wsock32
    gdi32
    StormStAE
)
