add_library(ObjectMgrClient STATIC
    ObjectMgrClient.cpp
)

target_include_directories(ObjectMgrClient PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(ObjectMgrClient PRIVATE /Gr /Z7)
endif()
