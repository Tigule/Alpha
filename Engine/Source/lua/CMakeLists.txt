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
    src/lmemPool.cpp
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
    ../../../Storm/H
)

if(MSVC)
    target_compile_options(lua PRIVATE /UASSERTIONS_ENABLED)
endif()

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(lua PRIVATE /Gr)
    set_source_files_properties(src/lmemPool.cpp PROPERTIES COMPILE_OPTIONS "/GX;/Ob0")
elseif(MSVC)
    # MSVC 19.51 /O2 drops the n == -1 check in computesizes() and builds a 0x80000000 array size
    set_source_files_properties(src/ltable.c PROPERTIES COMPILE_OPTIONS $<$<NOT:$<CONFIG:Debug>>:/O1>)
endif()
