add_library(Zlib STATIC
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

target_compile_definitions(Zlib PRIVATE Z_PREFIX)

if(MSVC)
    target_compile_options(Zlib PRIVATE /Gr)
endif()
