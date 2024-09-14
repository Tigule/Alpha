add_library(Expat STATIC
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c
)

target_include_directories(Expat
    PRIVATE
    .

    PUBLIC
    lib
)

add_compile_definitions(_STATIC)
