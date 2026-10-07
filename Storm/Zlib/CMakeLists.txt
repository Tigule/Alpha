add_library(Zlib OBJECT
    adler32.c
    deflate.c
    infblock.c
    infcodes.c
    inffast.c
    inflate.c
    inftrees.c
    infutil.c
    trees.c
    zutil.c
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Zlib PRIVATE /Gr /Ob2 /Oy)
    target_compile_definitions(Zlib PRIVATE Z_PREFIX)
endif()
