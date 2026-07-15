cmake_minimum_required(VERSION 3.18)
project(BuildWoW)

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
