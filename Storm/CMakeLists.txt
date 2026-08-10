add_subdirectory(Zlib)

add_library(Storm STATIC
    Source/CRC.CPP
    Source/SBig.cpp
    Source/SCmd.cpp
    Source/SComp.cpp
    Source/SFile2.cpp
    Source/Sha1.cpp
    Source/soprintf.cpp
    Source/SOption.cpp
    Source/SRgn.cpp
    Source/SSignature.cpp
    Source/SStr.cpp
    Source/StormService.cpp
    Source/SZip.cpp
    Source/SUnicode.cpp
)

if(WIN32)
    target_sources(Storm PRIVATE
        Source/W32/SDir.cpp
        Source/W32/SSystem.cpp
        Source/W32/SInterlocked.cpp
        Source/W32/SServer.cpp
        Source/W32/SThread.cpp
        Source/W32/SLock.cpp

        Source/CryptRSA.cpp
        Source/LogMachineState.cpp
        Source/SErr.cpp
        Source/SEvt.cpp
        Source/SFile.cpp
        Source/SLog.cpp
        Source/SMem.cpp
        Source/SMsg.cpp
        Source/SReg.cpp
        Source/SRtl.cpp
        Source/Storm.cpp
    )

endif()

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Storm PRIVATE /Gr /GX- /Ob1 /Oy- /Z7)

    set_property(SOURCE
        Source/SBig.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /GX- /Ob0")

    set_property(SOURCE
        Source/SCmd.cpp
        Source/SComp.cpp
        Source/SErr.cpp
        Source/SEvt.cpp
        Source/SFile.cpp
        Source/SFile2.cpp
        Source/SMsg.cpp
        Source/SRgn.cpp
        Source/SZip.cpp
        Source/W32/SDir.cpp
        Source/W32/SLock.cpp
        Source/W32/SThread.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /Ob0")

    set_property(SOURCE
        Source/SZip.cpp
        APPEND_STRING PROPERTY COMPILE_FLAGS " /Zp1")
endif()

target_include_directories(Storm
    PUBLIC
    H/

    PRIVATE
    Source/
)

target_link_libraries(Storm Zlib)
