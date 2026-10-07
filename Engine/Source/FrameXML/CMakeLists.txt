add_library(FrameXML STATIC
    FrameXML.cpp
    LoadXML.cpp
    XMLTree.cpp
)

target_include_directories(FrameXML PRIVATE
    ..
    ../../../Storm/H
    ../lua/include
)

target_compile_definitions(FrameXML PRIVATE _STATIC)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(FrameXML PRIVATE /Gr /GX /Ob0 /Zd)
endif()
