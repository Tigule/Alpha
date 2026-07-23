add_subdirectory(Source/Expat)
add_subdirectory(Source/Gxu/FreeType)
add_subdirectory(Source/lua)

add_library(Engine STATIC
    Source/Anim/Interp.cpp
    Source/Anim/Anim.cpp
    Source/Anim/MdlAnim.cpp
    Source/Anim/Interface.cpp
    Source/Anim/Transform.cpp
    Source/Anim/WorldMatrix.cpp
    Source/Anim/Init.cpp
    Source/Images/blit.cpp
    Source/Images/dxt.cpp
    Source/Images/funcs.cpp
    Source/Images/tga.cpp
    Source/MDLFile/TSet.cpp
    Source/MDLFile/Attachment.cpp
    Source/MDLFile/lexout.cpp
    Source/MDLFile/Bone.cpp
    Source/MDLFile/TokenText.cpp
    Source/MDLFile/BinHandlers.cpp
    Source/MDLFile/TextHandlers.cpp
    Source/MDLFile/Sequences.cpp
    Source/MDLFile/Camera.cpp
    Source/MDLFile/EventObject.cpp
    Source/MDLFile/Version.cpp
    Source/MDLFile/Header.cpp
    Source/MDLFile/MDLFile.cpp
    Source/MDLFile/Light.cpp
    Source/MDLFile/Texture.cpp
    Source/MDLFile/Helper.cpp
    Source/MDLFile/Geoset.cpp
    Source/MDLFile/PivotPoint.cpp
    Source/MDLFile/MDLStatus.cpp
    Source/MDLFile/Model.cpp
    Source/MDLFile/GenObject.cpp
    Source/MDLFile/HitTest.cpp
    Source/MDLFile/Material.cpp
    Source/MDLFile/Parser.cpp
    Source/MDLFile/Collision.cpp
    Source/MDLFile/TextureAnim.cpp
    Source/MDLFile/ParticleEmitter.cpp
    Source/MDLFile/ParticleEmitter2.cpp
    Source/MDLFile/RibbonEmitter.cpp
    Source/Tempest/caabox.cpp
    Source/Tempest/c22matrix.cpp
    Source/Tempest/crandom.cpp
    Source/Tempest/cfacet.cpp
    Source/Tempest/c3vector.cpp
    Source/Tempest/cmath_inline.cpp
    Source/Tempest/c4quaternion.cpp
    Source/Tempest/cmemblock.cpp
    Source/Tempest/c34matrix.cpp
    Source/Tempest/cmath.cpp
    Source/Tempest/c44matrix.cpp
    Source/Tempest/c33matrix.cpp
    Source/Tempest/c3spline.cpp
    Source/Tempest/cimvector.cpp
    Source/Tempest/tempest_intersect.cpp
    Source/Frame/CSimpleMessageScrollFrameScript.cpp
    Source/Frame/CSimpleFrameScript.cpp
    Source/Frame/CSimpleMessageFrameScript.cpp
    Source/Frame/CFramePoint.cpp
    Source/Frame/CSimpleMessageFrame.cpp
    Source/Frame/CSimpleSliderScript.cpp
    Source/Frame/CSimpleHTMLScript.cpp
    Source/Frame/CSimpleSlider.cpp
    Source/Frame/CSimpleRenderScript.cpp
    Source/Frame/CSimpleRegion.cpp
    Source/Frame/CSimpleScrollFrameScript.cpp
    Source/Frame/CSimpleModel.cpp
    Source/Frame/CBackdropGenerator.cpp
    Source/Frame/CSimpleEditBox.cpp
    Source/Frame/CSimpleButtonScript.cpp
    Source/Frame/CSimpleTop.cpp
    Source/Frame/CSimpleCheckbox.cpp
    Source/Frame/CSimpleRender.cpp
    Source/Frame/CSimpleButton.cpp
    Source/Frame/CSimpleStatusBarScript.cpp
    Source/Frame/CSimpleScrollFrame.cpp
    Source/Frame/CSimpleMessageScrollFrame.cpp
    Source/Frame/CSimpleFrame.cpp
    Source/Frame/CSimpleModelScript.cpp
    Source/Frame/CSimpleStatusBar.cpp
    Source/Frame/CSimpleHTML.cpp
    Source/Frame/CSimpleCheckboxScript.cpp
    Source/Frame/SimpleFrameRegistry.cpp
    Source/Frame/CSimpleHyperlinkedFrame.cpp
    Source/Frame/CLayoutFrame.cpp
    Source/Frame/CSimpleEditBoxScript.cpp
    Source/FrameScript/FrameScript.cpp
    Source/FrameScript/FrameScriptMethods.cpp
    Source/Model/ModelCreate.cpp
    Source/Model/MdlSgModel.cpp
    Source/Model/MdlValidate.cpp
    Source/Model/CollisionData.cpp
    Source/Model/MdlSgLight.cpp
    Source/Model/MdlRibbonEmitter.cpp
    Source/Model/CollisionDisplay.cpp
    Source/Model/ModelMisc.cpp
    Source/Model/ModelAnim.cpp
    Source/Model/MdlParticleEmitter2.cpp
    Source/Model/MdlCamera.cpp
    Source/Model/ModelRender.cpp
    Source/BLPFile/blp.cpp
    Source/Images/tga.cpp
    Source/Gx/GxApi/GxApi.cpp
    Source/Gx/CGxDeviceOpenGL/CGxDeviceOpenGl.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglTexture.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglPrim.cpp
    Source/Gx/CGxDeviceOpenGL/GlExtSupport.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglState.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglXform.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglShader.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglCaps.cpp
    Source/Gx/CGxDeviceOpenGL/CGxOglScene.cpp
    Source/FrameXML/XMLTree.cpp
    Source/FrameXML/FrameXML.cpp
    Source/FrameXML/LoadXML.cpp
    Source/Gxu/Gxu.cpp
    Source/Gxu/GxuFontUtil.cpp
    Source/Gxu/GxuFont.cpp
    Source/Gxu/IGxuFontGlyph.cpp
    Source/Gxu/GxuFontFace.cpp
    Source/Gxu/GxuLight.cpp
    Source/SRP/SHA.cpp
    Source/SRP/SRP6.cpp
    Source/Event/EvtInput.cpp
    Source/Event/EvtTimer.cpp
    Source/Event/EvtQueue.cpp
    Source/Event/EvtApi.cpp
    Source/Event/CInputObserver.cpp
    Source/Event/EvtSched.cpp
    Source/Event/CObserver.cpp
    Source/Base/CUnreal.cpp
    Source/Base/MsgBuffer.cpp
    Source/Base/UnrealConstants.cpp
    Source/Base/Status.cpp
    Source/Base/Prop.cpp
    Source/Base/ConvertUTF.cpp
    Source/Base/RCString.cpp
    Source/Base/Base.cpp
    Source/Base/Coordinate.cpp
    Source/Base/Activity.cpp
    Source/Base/CDataStore.cpp
    Source/Base/CmdLine.cpp
    Source/Base/Handle.cpp
    Source/Base/CDataAllocator.cpp
    Source/Base/FileCache.cpp
    Source/Base/CDataRecycler.cpp
    Source/regex/regfree.c
    Source/regex/regcomp.c
    Source/regex/regexec.c
    Source/Services/Camera.cpp
    Source/Services/ParticleSystem.cpp
    Source/Services/GfxSingletonManager.cpp
    Source/Services/Texture.cpp
    Source/Services/DataMgr.cpp
    Source/Services/Profile.cpp
    Source/Services/TextBlock.cpp
    Source/Services/Lightning.cpp
    Source/Services/SysMessage.cpp
    Source/Services/ParticleMisc.cpp
    Source/Services/AsyncFileRead.cpp
    Source/Services/ParticleEmitters.cpp
    Source/Services/RibbonEmitter.cpp
    Source/Services/ParticleSystem2.cpp
    Source/Scrn/SCRN.CPP
    Source/Scrn/LAYER.CPP
    Source/Scrn/STOCK.CPP
    Source/Scrn/PERF.CPP
)

set_source_files_properties(
    Source/MDLFile/lexout.cpp
    PROPERTIES OBJECT_NAME lex
)

if (WIN32)
    target_sources(Engine PRIVATE
        Source/Gx/CGxDeviceOpenGL/W32/GlWndSupport.cpp
        Source/Gx/CGxDeviceOpenGL/W32/CGxOglDeviceW32.cpp
        Source/Gx/CGxDevice/CGxDevice.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dState.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dPrim.cpp
        Source/Gx/CGxDeviceD3d/CGxDeviceD3d.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dXform.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dTexture.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dScene.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dShader.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dDevice.cpp
        Source/Gx/CGxDeviceD3d/CGxD3dCaps.cpp
        Source/Os/W32/OsMemory.cpp
        Source/Os/W32/OsSoundMIDI.cpp
        Source/Os/W32/Input.cpp
        Source/Os/W32/OsSound.cpp
        Source/Os/W32/OsCall.cpp
        Source/Os/W32/OsIME.cpp
        Source/Os/W32/OsGui.cpp
        Source/Os/W32/OsClipboard.cpp
        Source/Os/W32/OsJoystick.cpp
        Source/Os/W32/Debugging.cpp
        Source/Os/W32/OsISoundCache.cpp
        Source/Os/W32/OsFile.cpp
        Source/Os/W32/OSSystem.cpp
        Source/Os/W32/Tls.cpp
        Source/Os/W32/OsTcp.cpp
        Source/Os/W32/Path.cpp
        Source/Os/W32/Errors.cpp
        Source/Os/W32/Time.cpp
    )

endif()

if(MSVC_VERSION EQUAL 1200)
    set_source_files_properties(
        Source/regex/regfree.c
        Source/regex/regcomp.c
        Source/regex/regexec.c
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob1")

    set_source_files_properties(
        Source/FrameXML/FrameXML.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/FrameXML/XMLTree.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob0 /Oy-")
    set_source_files_properties(
        Source/FrameXML/LoadXML.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob0 /Oy-")

    set_source_files_properties(
        Source/Gxu/GxuFont.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /GX- /Ob2 /Oy-")
    set_source_files_properties(
        Source/Gxu/GxuFontUtil.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Gxu/IGxuFontGlyph.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr")
    set_source_files_properties(
        Source/Gxu/GxuFontFace.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /GX-")

    set_source_files_properties(
        Source/Event/EvtSched.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Event/EvtApi.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Event/CObserver.cpp
        PROPERTIES COMPILE_FLAGS "/GX-")

    set_source_files_properties(
        Source/Base/Coordinate.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Oy-")
    set_source_files_properties(
        Source/Base/ConvertUTF.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Oy-")
    set_source_files_properties(
        Source/Os/W32/Time.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/OsSound.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-")
    set_source_files_properties(
        Source/Os/W32/OSSystem.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/OsISoundCache.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob2 /Oy- /GX-")
    set_source_files_properties(
        Source/Os/W32/OsFile.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/OsGui.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/Path.cpp
        Source/Gxu/Gxu.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Gxu/GxuLight.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob2 /Oy-")
    set_source_files_properties(
        Source/Base/Activity.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Base/CDataStore.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Base/RCString.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Base/Handle.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1")
    set_source_files_properties(
        Source/Anim/Init.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob2 /Oy-")
    set_source_files_properties(
        Source/MDLFile/MDLFile.cpp
        Source/MDLFile/TokenText.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1")
    set_source_files_properties(
        Source/Anim/MdlAnim.cpp
        Source/Anim/Transform.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1")
    set_source_files_properties(
        Source/Anim/Interp.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Anim/WorldMatrix.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Services/Profile.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-")
    set_source_files_properties(
        Source/FrameScript/FrameScript.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /Ob0")
    set_source_files_properties(
        Source/Gx/GxApi/GxApi.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob2 /Oy-")
    set_source_files_properties(
        Source/Gx/CGxDevice/CGxDevice.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Gx/CGxDeviceD3d/CGxD3dDevice.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Gx/CGxDeviceD3d/CGxD3dPrim.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Gx/CGxDeviceD3d/CGxD3dXform.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Services/Camera.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Services/Lightning.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Tempest/c34matrix.cpp
        Source/Tempest/c44matrix.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Tempest/caabox.cpp
        Source/Tempest/c33matrix.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Tempest/cmemblock.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Tempest/c3vector.cpp
        Source/Tempest/cimvector.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-")
    set_source_files_properties(
        Source/Model/ModelAnim.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Anim/Anim.cpp
        Source/Anim/Interface.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Model/ModelRender.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy- /GX-")
    set_source_files_properties(
        Source/Services/GfxSingletonManager.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Services/RibbonEmitter.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Services/ParticleSystem.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Services/ParticleSystem2.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob2 /Oy-")
    set_source_files_properties(
        Source/Frame/CSimpleTop.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/CLayoutFrame.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Frame/CSimpleRender.cpp
        Source/Frame/CSimpleRegion.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/CBackdropGenerator.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Services/TextBlock.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/CFramePoint.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/CSimpleFrame.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/CSimpleButtonScript.cpp
        Source/Frame/CSimpleCheckboxScript.cpp
        Source/Frame/CSimpleEditBoxScript.cpp
        Source/Frame/CSimpleFrameScript.cpp
        Source/Frame/CSimpleHTMLScript.cpp
        Source/Frame/CSimpleMessageFrameScript.cpp
        Source/Frame/CSimpleMessageScrollFrameScript.cpp
        Source/Frame/CSimpleModelScript.cpp
        Source/Frame/CSimpleRenderScript.cpp
        Source/Frame/CSimpleScrollFrameScript.cpp
        Source/Frame/CSimpleSliderScript.cpp
        Source/Frame/CSimpleStatusBarScript.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Gr /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Frame/SimpleFrameRegistry.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Frame/CSimpleButton.cpp
        Source/Frame/CSimpleCheckbox.cpp
        Source/Frame/CSimpleEditBox.cpp
        Source/Frame/CSimpleMessageFrame.cpp
        Source/Frame/CSimpleModel.cpp
        Source/Frame/CSimpleScrollFrame.cpp
        Source/Frame/CSimpleMessageScrollFrame.cpp
        Source/Frame/CSimpleSlider.cpp
        Source/Frame/CSimpleHTML.cpp
        Source/Frame/CSimpleStatusBar.cpp
        Source/Frame/CSimpleHyperlinkedFrame.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Images/tga.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1")
    set_source_files_properties(
        Source/Images/blit.cpp
        Source/Images/dxt.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Images/funcs.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1")
    set_source_files_properties(
        Source/BLPFile/blp.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Services/AsyncFileRead.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob2")
    set_source_files_properties(
        Source/Services/ParticleEmitters.cpp
        PROPERTIES COMPILE_FLAGS "/GX-")
    set_source_files_properties(
        Source/Model/ModelCreate.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Model/MdlSgModel.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Model/ModelMisc.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Services/Texture.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1")
    set_source_files_properties(
        Source/Scrn/SCRN.CPP
        Source/Scrn/LAYER.CPP
        Source/Scrn/STOCK.CPP
        Source/Scrn/PERF.CPP
        PROPERTIES COMPILE_FLAGS "/G6 /GX- /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/OsClipboard.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/Debugging.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /Oy-")
    set_source_files_properties(
        Source/Os/W32/OsJoystick.cpp
        PROPERTIES COMPILE_FLAGS "/G6")
    set_source_files_properties(
        Source/Os/W32/OsTcp.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /Ob1 /GX-")
    set_source_files_properties(
        Source/Os/W32/OsCall.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX-")

    set_source_files_properties(
        Source/Base/FileCache.cpp
        PROPERTIES COMPILE_FLAGS "/G6 /GX-")

    set_property(SOURCE
        Source/Event/CObserver.cpp
        Source/Event/CInputObserver.cpp
        Source/Base/FileCache.cpp
        Source/Services/SysMessage.cpp
        Source/Gx/CGxDeviceOpenGL/W32/CGxOglDeviceW32.cpp
        Source/Gxu/IGxuFontGlyph.cpp
        Source/Os/W32/OsCall.cpp
        Source/Os/W32/OsJoystick.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /Ob2")
endif()

target_include_directories(Engine
    PUBLIC
    Source/

    PRIVATE
    ${CMAKE_SOURCE_DIR}/Storm/Source
    "$ENV{DXSDK_DIR}/Include"
)

target_link_libraries(Engine Storm Expat FreeType lua)

if(WIN32)
    set(FMOD_IMPORT_LIBRARY
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/FMOD/api/lib/fmodvc.lib")
    find_library(D3DX9_IMPORT_LIBRARY d3dx9 PATHS "$ENV{DXSDK_DIR}/Lib/x86")
    target_link_libraries(
        Engine
        "${FMOD_IMPORT_LIBRARY}"
        "${D3DX9_IMPORT_LIBRARY}"
        glu32
        imm32
        opengl32
        winmm
        wsock32)

endif()
