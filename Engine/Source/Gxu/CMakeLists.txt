add_library(Gxu STATIC
    Gxu.cpp
    GxuFont.cpp
    GxuFontFace.cpp
    GxuFontUtil.cpp
    GxuLight.cpp
    IGxuFontGlyph.cpp
)

target_include_directories(Gxu PRIVATE
    ..
    ../../../Storm/H

    FreeType/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Gxu PRIVATE /Gr /Z7)
endif()
