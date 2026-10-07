add_library(FrameScript STATIC
    FrameScript.cpp
    FrameScriptMethods.cpp
)

target_include_directories(FrameScript PRIVATE
    ..
    ../../../Storm/H
    ../lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(FrameScript PRIVATE /Gr /GX /Ob0 /Zd)
    set_source_files_properties(FrameScriptMethods.cpp PROPERTIES COMPILE_OPTIONS "/GX-;/Ob1")
endif()
