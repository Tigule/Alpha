add_executable(WowUpdateHelper
    WowUpdateHelper.cpp
)

target_include_directories(WowUpdateHelper PRIVATE
    ../../../Storm/H
)

set_target_properties(WowUpdateHelper PROPERTIES OUTPUT_NAME WowUpdateHelperAE)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowUpdateHelper PRIVATE /Gr /Z7)
    target_link_libraries(WowUpdateHelper PRIVATE unicows msvcrt)
endif()

target_link_libraries(WowUpdateHelper PRIVATE
    kernel32
    user32
    advapi32
    StormStAE
)
