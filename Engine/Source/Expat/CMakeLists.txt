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

target_compile_definitions(Expat PUBLIC _STATIC)
