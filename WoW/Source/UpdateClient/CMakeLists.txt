add_executable(UpdateClient
    UpdateClient.cpp
)

target_include_directories(UpdateClient PRIVATE
    ../../../Storm/H
)

set_target_properties(UpdateClient PROPERTIES
    OUTPUT_NAME UpdateClient
    OUTPUT_NAME_RELEASE_ASSERTIONS_ENABLED UpdateClientAE
    OUTPUT_NAME_RELEASE UpdateClient
    OUTPUT_NAME_DEBUG UpdateClientD
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(UpdateClient PRIVATE /Gr /Zi)
    target_link_libraries(UpdateClient PRIVATE unicows msvcrt)
endif()

target_link_libraries(UpdateClient PRIVATE
    kernel32
    user32
    advapi32
    Storm
)
