add_library(Component STATIC
    CharacterCustomization.cpp
    Component.cpp
    ComponentUtil.cpp
)

target_include_directories(Component PRIVATE
    ..
    ../../../Engine/Source
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    get_filename_component(COMPONENT_COMMON_LINK ${CMAKE_CURRENT_BINARY_DIR}/../../common ABSOLUTE)
    if(NOT EXISTS ${COMPONENT_COMMON_LINK})
        file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/../../Common ${COMPONENT_COMMON_LINK} RESULT COMPONENT_COMMON_LINK_RESULT SYMBOLIC)
        if(NOT COMPONENT_COMMON_LINK_RESULT STREQUAL "0")
            get_filename_component(COMPONENT_COMMON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../../Common ABSOLUTE)
            file(TO_NATIVE_PATH ${COMPONENT_COMMON_SOURCE} COMPONENT_COMMON_TARGET)
            file(TO_NATIVE_PATH ${COMPONENT_COMMON_LINK} COMPONENT_COMMON_JUNCTION)
            execute_process(COMMAND cmd /c mklink /J "${COMPONENT_COMMON_JUNCTION}" "${COMPONENT_COMMON_TARGET}" OUTPUT_QUIET)
        endif()
    endif()
    target_compile_options(Component PRIVATE /Gr /Zi /I..\\..\\common)
else()
    target_include_directories(Component PRIVATE ../../Common)
endif()
