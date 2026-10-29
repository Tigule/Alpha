add_library(Storm STATIC
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

    $<TARGET_OBJECTS:Pkware>
    $<TARGET_OBJECTS:Zlib>
)

if(WIN32)
    target_sources(Storm PRIVATE
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

target_include_directories(Storm PRIVATE
    ../H

    .
)

# St = static, AE = assertions enabled
set_target_properties(Storm PROPERTIES
    OUTPUT_NAME StormSt
    OUTPUT_NAME_RELEASE_ASSERTIONS_ENABLED StormStAE
    OUTPUT_NAME_RELEASE StormSt
    OUTPUT_NAME_DEBUG StormStD
    ARCHIVE_OUTPUT_DIRECTORY StormSt$<CONFIG>
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Storm PRIVATE /Gr /Ob0 /Zi /YX)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES vc60.pch)
    target_compile_definitions(Storm PRIVATE Z_PREFIX)
endif()
