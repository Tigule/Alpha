add_library(ConsoleClient STATIC
    ConsoleClient.cpp
    ConsoleCommand.cpp
    ConsoleDetect.cpp
    ConsoleVar.cpp
    IConsole.cpp
)

target_include_directories(ConsoleClient PRIVATE
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ..
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(ConsoleClient PRIVATE /Gr /Z7)
endif()
