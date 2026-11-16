add_library(Gx STATIC
    CGxDeviceOpenGL/CGxDeviceOpenGl.cpp
    CGxDeviceOpenGL/CGxOglCaps.cpp
    CGxDeviceOpenGL/CGxOglPrim.cpp
    CGxDeviceOpenGL/CGxOglScene.cpp
    CGxDeviceOpenGL/CGxOglShader.cpp
    CGxDeviceOpenGL/CGxOglState.cpp
    CGxDeviceOpenGL/CGxOglTexture.cpp
    CGxDeviceOpenGL/CGxOglXform.cpp
    CGxDeviceOpenGL/GlExtSupport.cpp
    GxApi/GxApi.cpp
)

if(WIN32)
    target_sources(Gx PRIVATE
        CGxDeviceD3d/CGxD3dCaps.cpp
        CGxDeviceD3d/CGxD3dDevice.cpp
        CGxDeviceD3d/CGxD3dPrim.cpp
        CGxDeviceD3d/CGxD3dScene.cpp
        CGxDeviceD3d/CGxD3dShader.cpp
        CGxDeviceD3d/CGxD3dState.cpp
        CGxDeviceD3d/CGxD3dTexture.cpp
        CGxDeviceD3d/CGxD3dXform.cpp
        CGxDevice/CGxDevice.cpp
        CGxDeviceD3d/CGxDeviceD3d.cpp
        CGxDeviceOpenGL/W32/CGxOglDeviceW32.cpp
        CGxDeviceOpenGL/W32/GlWndSupport.cpp
    )
endif()

target_include_directories(Gx PRIVATE
    ..
    ../../../Storm/H
)

if(NOT HAVE_WIN8_SDK)
    target_include_directories(Gx PRIVATE "$ENV{DXSDK_DIR}/Include")
else()
    # D3DAssemble is not exposed in Windows SDK headers
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(D3DASSEMBLE_EXPORT D3DAssemble@32)
    else()
        set(D3DASSEMBLE_EXPORT D3DAssemble)
    endif()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def "LIBRARY d3dcompiler_47.dll\nEXPORTS\n    ${D3DASSEMBLE_EXPORT}\n")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib
        COMMAND ${CMAKE_AR} /nologo /def:${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def /out:${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib /machine:${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def)
    add_custom_target(d3dcompiler_47 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib)
    add_dependencies(Gx d3dcompiler_47)
    target_link_libraries(Gx PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib)
endif()

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Gx PRIVATE /Gr /Zi)

    set(GX_PCH ${CMAKE_CURRENT_BINARY_DIR}/Gx.pch)
    get_target_property(GX_SOURCES Gx SOURCES)
    foreach(src IN LISTS GX_SOURCES)
        if(src STREQUAL "GxApi/GxApi.cpp")
            set_source_files_properties(${src} PROPERTIES
                COMPILE_OPTIONS "/FIwindows.h;/FIGx/CGxDevice.h;/YcGx/CGxDevice.h;/Fp${GX_PCH}"
                OBJECT_OUTPUTS ${GX_PCH})
        else()
            set_source_files_properties(${src} PROPERTIES
                COMPILE_OPTIONS "/FIwindows.h;/FIGx/CGxDevice.h;/YuGx/CGxDevice.h;/Fp${GX_PCH}"
                OBJECT_DEPENDS ${GX_PCH})
        endif()
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${GX_PCH})
elseif(MSVC)
    target_compile_options(Gx PRIVATE /FIwindows.h)
endif()
