cmake_minimum_required(VERSION 3.18)
project(BuildWoW)

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC AND MSVC_VERSION GREATER_EQUAL 1400)
    add_compile_options(/MP)
endif()

if(MSVC_VERSION EQUAL 1200)
    add_compile_options(/G6 /Ob1 /Oy- /Z7 /GX-)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /MAP")
endif()

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
