add_library(LogClient STATIC
    Log.cpp
)

target_include_directories(LogClient PRIVATE
    ../../../Storm/H

    ../../../Engine/Source
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(LogClient PRIVATE /Gr /Z7)
endif()
