add_library(FreeType STATIC
    src/autohint/autohint.c

    src/base/ftbase.c
    src/base/ftinit.c
    src/base/ftsystem.c

    src/cff/cff.c

    src/cid/type1cid.c

	src/psaux/psaux.c

	src/psnames/psmodule.c

	src/raster/raster.c

	src/sfnt/sfnt.c

    src/smooth/smooth.c

	src/truetype/truetype.c

	src/type1/type1.c

    src/winfonts/winfnt.c
)

target_include_directories(FreeType
    PRIVATE
    src

    PUBLIC
    include
)
