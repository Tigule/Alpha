add_library(lua STATIC
    src/lib/lauxlib.c
    src/lib/lmathlib.c
    src/lib/lstrlib.c
    src/lib/ltablib.c

    src/lapi.c
    src/lcode.c
    src/ldebug.c
    src/ldo.c
    src/lfunc.c
    src/lgc.c
    src/llex.c
    src/lmem.c
    src/lobject.c
    src/lopcodes.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/ltable.c
    src/ltm.c
    src/lundump.c
    src/lvm.c
    src/lzio.c
)

target_include_directories(lua
    PUBLIC
    include
)
