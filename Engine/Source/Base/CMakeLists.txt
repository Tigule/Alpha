add_library(Base STATIC
    Activity.cpp
    Base.cpp
    CDataAllocator.cpp
    CDataRecycler.cpp
    CDataStore.cpp
    CmdLine.cpp
    ConvertUTF.cpp
    Coordinate.cpp
    CUnreal.cpp
    FileCache.cpp
    Handle.cpp
    MsgBuffer.cpp
    Prop.cpp
    RCString.cpp
    Status.cpp
    UnrealConstants.cpp
)

target_include_directories(Base PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Base PRIVATE /Gr /Z7)
endif()
