add_library(Expat STATIC
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c
)

if(MSVC)
    target_compile_options(Expat PRIVATE /Gd)
endif()

target_include_directories(Expat
    PRIVATE
    .

    PUBLIC
    lib
)

add_compile_definitions(_STATIC)
