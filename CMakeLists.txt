cmake_minimum_required(VERSION 3.18)
project(BuildWoW)

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC_VERSION EQUAL 1200)
    add_compile_options(/G6 /Ob1 /Oy-)
endif()

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
