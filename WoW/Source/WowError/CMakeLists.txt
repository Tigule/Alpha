if(APPLE)
    add_executable(WowError MACOSX_BUNDLE
        WowErrorMac.cpp
    )

    set_source_files_properties(WowErrorMac.cpp PROPERTIES LANGUAGE OBJCXX)

    set_target_properties(WowError PROPERTIES
        OUTPUT_NAME WoWError
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Mac/Info.plist
    )

    target_link_libraries(WowError PRIVATE
        "-framework AppKit"
        "-framework Cocoa"
    )

    return()
endif()

add_executable(WowError WIN32
    WowError.cpp
)

target_include_directories(WowError PRIVATE
    ../../../Storm/H
)

set_target_properties(WowError PROPERTIES
    OUTPUT_NAME WowError
    OUTPUT_NAME_RELEASE_ASSERTIONS_ENABLED WowErrorAE
    OUTPUT_NAME_RELEASE WowError
    OUTPUT_NAME_DEBUG WowErrorD
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WowError PRIVATE /Gr /Zi /YX)
    target_link_libraries(WowError PRIVATE unicows msvcrt)
endif()

target_link_libraries(WowError PRIVATE
    kernel32
    user32
    advapi32
    wsock32
    gdi32
    Storm
)
