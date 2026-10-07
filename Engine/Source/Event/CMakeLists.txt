add_library(Event STATIC
    CInputObserver.cpp
    CObserver.cpp
    EvtApi.cpp
    EvtInput.cpp
    EvtQueue.cpp
    EvtSched.cpp
    EvtTimer.cpp
)

target_include_directories(Event PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Event PRIVATE /Gr /Z7)
endif()
