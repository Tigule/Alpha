add_library(FreeType STATIC
    src/autohint/autohint.c
    src/cff/cff.c
    src/base/ftbase.c
    src/base/ftinit.c
    src/base/ftsystem.c
    src/psaux/psaux.c
    src/psnames/psmodule.c
    src/raster/raster.c
    src/sfnt/sfnt.c
    src/smooth/smooth.c
    src/truetype/truetype.c
    src/type1/type1.c
    src/cid/type1cid.c
    src/winfonts/winfnt.c
)

target_include_directories(FreeType PRIVATE
    src

    include
)

set_source_files_properties(src/psnames/psmodule.c PROPERTIES OBJECT_NAME psnames)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(FreeType PRIVATE /Gr /Z7 /Za)
endif()
