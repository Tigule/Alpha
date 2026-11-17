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
elseif(APPLE)
    target_sources(Gx PRIVATE
        CGxDevice/CGxDevice.cpp
        CGxDeviceOpenGL/Mac/CGxOglDeviceMac.cpp
        CGxDeviceOpenGL/Mac/CGxOglPrim_OSX.cpp
        CGxDeviceOpenGL/Mac/GlWndSupportMac.mm
    )
    set_source_files_properties(CGxDeviceOpenGL/CGxOglPrim.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
elseif(UNIX)
    target_sources(Gx PRIVATE
        CGxDevice/CGxDevice.cpp
        CGxDeviceOpenGL/SDL/CGxOglDeviceSDL.cpp
    )
    target_link_libraries(Gx PUBLIC PkgConfig::SDL2 GL GLU X11)
endif()

target_include_directories(Gx PRIVATE
    ..
    ../../../Storm/H
)

if(APPLE OR UNIX)
elseif(NOT HAVE_WIN8_SDK)
    target_include_directories(Gx PRIVATE "$ENV{DXSDK_DIR}/Include")
else()
    # D3DAssemble is not exposed in Windows SDK headers or import libs
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def "LIBRARY d3dcompiler_47.dll\nEXPORTS\n    D3DAssemble\n")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.cpp "extern \"C\" long __stdcall D3DAssemble(const void *, unsigned long, const char *, const void *, void *, unsigned int, void **, void **) {\n  return 0;\n}\n")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib
        COMMAND ${CMAKE_CXX_COMPILER} /nologo /Zl $<$<STREQUAL:${CMAKE_CXX_COMPILER_ARCHITECTURE_ID},ARM64EC>:/arm64EC> /c ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.cpp /Fo${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.obj
        COMMAND ${CMAKE_AR} /nologo /def:${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.obj /out:${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.lib /machine:${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.def ${CMAKE_CURRENT_BINARY_DIR}/d3dcompiler_47.cpp)
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
