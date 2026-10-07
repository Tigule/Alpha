add_library(regex STATIC
    regcomp.c
    regexec.c
    regfree.c
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(regex PRIVATE /Gr /Oy /Z7)
endif()
