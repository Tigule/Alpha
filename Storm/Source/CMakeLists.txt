add_library(StormStAE STATIC
    CRC.CPP
    SBig.cpp
    SCmd.cpp
    SComp.cpp
    SFile2.cpp
    Sha1.cpp
    soprintf.cpp
    SOption.cpp
    SRgn.cpp
    SSignature.cpp
    SStr.cpp
    StormService.cpp
    SUnicode.cpp
    SZip.cpp

    $<TARGET_OBJECTS:Zlib>
)

if(WIN32)
    target_sources(StormStAE PRIVATE
        CryptRSA.cpp
        LogMachineState.cpp
        W32/SDir.cpp
        SErr.cpp
        SEvt.cpp
        SFile.cpp
        W32/SInterlocked.cpp
        W32/SLock.cpp
        SLog.cpp
        SMem.cpp
        SMsg.cpp
        SReg.cpp
        SRtl.cpp
        W32/SServer.cpp
        W32/SSystem.cpp
        W32/SThread.cpp
        Storm.cpp
    )
endif()

target_include_directories(StormStAE PRIVATE
    ../H

    .
)

set_target_properties(StormStAE PROPERTIES ARCHIVE_OUTPUT_DIRECTORY StormStRelease_Assertions_Enabled)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(StormStAE PRIVATE /Gr /Ob0 /Z7)
    target_compile_definitions(StormStAE PRIVATE Z_PREFIX)
endif()
