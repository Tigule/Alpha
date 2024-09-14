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
        Source/SInterlocked.cpp
        Source/SLock.cpp
        Source/SLog.cpp
        Source/SMem.cpp
        Source/SMsg.cpp
        Source/SReg.cpp
        Source/SRtl.cpp
        Source/Storm.cpp
    )

    if(WOWHOOK)
        target_sources(Storm PRIVATE
            Source/StormHook.cpp
        )
    endif()
elseif(APPLE)
    target_sources(Storm PRIVATE
        Source/SMemMalloc.cpp
        Source/Linux/SCmd.cpp
        Source/Linux/SFile.cpp
        Source/Linux/SInterlocked.cpp
        Source/Linux/SLock.cpp
        Source/Linux/SLog.cpp
        Source/Linux/SThread.cpp
        Source/Mac/SErrMac.cpp
        Source/Mac/SRegMac.cpp
    )
elseif(UNIX)
    target_sources(Storm PRIVATE
        Source/SMemMalloc.cpp
        Source/Linux/SCmd.cpp
        Source/Linux/SErr.cpp
        Source/Linux/SFile.cpp
        Source/Linux/SInterlocked.cpp
        Source/Linux/SLock.cpp
        Source/Linux/SLog.cpp
        Source/Linux/SReg.cpp
        Source/Linux/SThread.cpp
    )
endif()

target_include_directories(Storm
    PUBLIC
    H/

    PRIVATE
    Source/
)

target_link_libraries(Storm Zlib)
