add_library(Detours
    STATIC
    src/creatwth.cpp
    src/detours.cpp
    src/disasm.cpp
    src/disolx86.cpp
    src/image.cpp
    src/modules.cpp
)

target_include_directories(Detours
    PUBLIC
    src
)
