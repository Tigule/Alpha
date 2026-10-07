add_library(Frame STATIC
    CBackdropGenerator.cpp
    CFramePoint.cpp
    CLayoutFrame.cpp
    CSimpleButton.cpp
    CSimpleButtonScript.cpp
    CSimpleCheckbox.cpp
    CSimpleCheckboxScript.cpp
    CSimpleEditBox.cpp
    CSimpleEditBoxScript.cpp
    CSimpleFrame.cpp
    CSimpleFrameScript.cpp
    CSimpleHTML.cpp
    CSimpleHTMLScript.cpp
    CSimpleHyperlinkedFrame.cpp
    CSimpleMessageFrame.cpp
    CSimpleMessageFrameScript.cpp
    CSimpleMessageScrollFrame.cpp
    CSimpleMessageScrollFrameScript.cpp
    CSimpleModel.cpp
    CSimpleModelScript.cpp
    CSimpleRegion.cpp
    CSimpleRender.cpp
    CSimpleRenderScript.cpp
    CSimpleScrollFrame.cpp
    CSimpleScrollFrameScript.cpp
    CSimpleSlider.cpp
    CSimpleSliderScript.cpp
    CSimpleStatusBar.cpp
    CSimpleStatusBarScript.cpp
    CSimpleTop.cpp
    SimpleFrameRegistry.cpp
)

target_include_directories(Frame PRIVATE
    ..
    ../../../Storm/H

    ../lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Frame PRIVATE /Gr /Z7)
endif()
