cmake_minimum_required(VERSION 3.18)
project(BuildWoW)

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC AND MSVC_VERSION GREATER_EQUAL 1400)
    add_compile_options(/MP)
endif()

if(MSVC_VERSION EQUAL 1200)
    if(DEFINED ENV{PSDK_DIR})
        include_directories(BEFORE "$ENV{PSDK_DIR}/Include")
        link_directories("$ENV{PSDK_DIR}/Lib")
    else()
        message(FATAL_ERROR
            "PSDK_DIR is not set. Please install the November 2001 Platform SDK and configure your system.")
    endif()
    add_definitions(-DWINVER=0x0501)
    add_definitions(-DNOMETAFILE)
    add_compile_options(/G6 /Ob1 /Oy- /GX-)
    set(CMAKE_C_STANDARD_LIBRARIES "unicows.lib ${CMAKE_C_STANDARD_LIBRARIES}")
    set(CMAKE_CXX_STANDARD_LIBRARIES "unicows.lib ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /MAP")
endif()

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
