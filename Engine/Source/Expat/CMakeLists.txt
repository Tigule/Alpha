add_library(expat STATIC
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c
)

target_include_directories(expat PRIVATE
    .
)

target_compile_definitions(expat PRIVATE _STATIC)

if(MSVC AND NOT MSVC_VERSION EQUAL 1200)
    target_compile_options(expat PRIVATE /Gd)
endif()

set_target_properties(expat PROPERTIES ARCHIVE_OUTPUT_DIRECTORY expat___Win32_Release_Assertions_Enabled)
