cmake_minimum_required(VERSION 3.18)
project(BuildWoW)

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC_VERSION GREATER 1200 OR MINGW)
    set(WOWHOOK 1)
    add_compile_definitions(WOWHOOK=1)
endif()

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
