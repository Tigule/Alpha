add_library(FreeType STATIC
    src/autohint/autohint.c
    src/cff/cff.c
    src/base/ftbase.c
    src/base/ftinit.c
    src/base/ftsystem.c
    src/psaux/psaux.c
    src/psnames/psnames.c
    src/sfnt/sfnt.c
    src/smooth/smooth.c
    src/raster/raster.c
    src/truetype/truetype.c
    src/type1/type1.c
    src/cid/type1cid.c
    src/winfonts/winfnt.c
)

target_include_directories(FreeType PRIVATE
    include
)

if(MSVC_VERSION EQUAL 1200)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/src)
        file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src RESULT FREETYPE_SRC_LINK SYMBOLIC)
        if(NOT FREETYPE_SRC_LINK STREQUAL "0")
            file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src FREETYPE_SRC_TARGET)
            file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/src FREETYPE_SRC_JUNCTION)
            execute_process(COMMAND cmd /c mklink /J "${FREETYPE_SRC_JUNCTION}" "${FREETYPE_SRC_TARGET}" OUTPUT_QUIET)
        endif()
    endif()
    target_compile_options(FreeType PRIVATE /Gr /Zi /Za /Isrc)
else()
    target_include_directories(FreeType PRIVATE src)
endif()
