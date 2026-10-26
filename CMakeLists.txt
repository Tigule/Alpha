cmake_minimum_required(VERSION 3.18)

set(CMAKE_CONFIGURATION_TYPES "Release_Assertions_Enabled;Release;Debug" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release_Assertions_Enabled CACHE STRING "" FORCE)
endif()

project(BuildWoW)

foreach(lang C CXX RC)
    set(CMAKE_${lang}_FLAGS_RELEASE_ASSERTIONS_ENABLED "${CMAKE_${lang}_FLAGS_RELEASE}")
endforeach()

foreach(type EXE SHARED MODULE STATIC)
    set(CMAKE_${type}_LINKER_FLAGS_RELEASE_ASSERTIONS_ENABLED "${CMAKE_${type}_LINKER_FLAGS_RELEASE}")
endforeach()

set(CMAKE_CXX_STANDARD 98)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<CONFIG>)

add_compile_definitions($<$<NOT:$<CONFIG:Release>>:ASSERTIONS_ENABLED>)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/WoW/bin")
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE ON)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

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
    set(CMAKE_C_STANDARD_LIBRARIES "")
    set(CMAKE_CXX_STANDARD_LIBRARIES "")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /MAP")
elseif(MSVC)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_definitions(-D_USE_32BIT_TIME_T)
    add_compile_options(/Zc:wchar_t- /wd4595 /Gr)
endif()

add_subdirectory(Engine)
add_subdirectory(Storm)
add_subdirectory(WoW)
