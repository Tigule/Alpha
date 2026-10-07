add_library(lua STATIC
    src/lapi.c
    src/lib/lauxlib.c
    src/lcode.c
    src/ldebug.c
    src/ldo.c
    src/ldump.c
    src/lfunc.c
    src/lgc.c
    src/llex.c
    src/lib/lmathlib.c
    src/lmem.c
    src/lobject.c
    src/lopcodes.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/lib/lstrlib.c
    src/ltable.c
    src/lib/ltablib.c
    src/ltm.c
    src/lundump.c
    src/lvm.c
    src/lzio.c
)

target_include_directories(lua PRIVATE
    include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(lua PRIVATE /Gr /Ob2 /Oy)
    set_source_files_properties(src/ldump.c PROPERTIES COMPILE_OPTIONS /Ob1)
endif()
