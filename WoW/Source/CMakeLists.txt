add_subdirectory(SoundInterface)
add_subdirectory(ObjectAlloc)
add_subdirectory(ObjectMgrClient)
add_subdirectory(Glue)
add_subdirectory(Component)
add_subdirectory(Object)
add_subdirectory(Ui)
add_subdirectory(UIUtil)
add_subdirectory(Net/NetClient)
add_subdirectory(DB/DBClient)
add_subdirectory(CompFilenameDecoration)
add_subdirectory(WowSvcs/WowSvcsClient)
add_subdirectory(WowServices)
add_subdirectory(Object/ObjectClient)
add_subdirectory(Console)
add_subdirectory(Magic)
add_subdirectory(Game)
add_subdirectory(WorldClient)
add_subdirectory(Game/GameClient)
add_subdirectory(WorldCommon)
add_subdirectory(LogClient)
add_subdirectory(Magic/MagicClient)

if(WIN32)
    add_subdirectory(UpdateClient)
    add_subdirectory(WowError)
    add_subdirectory(WowUpdateHelper)
endif()

if(WIN32)
    add_executable(Wow WIN32
        Client.cpp
    )
elseif(APPLE)
    add_executable(Wow
        Mac/Source/AboutBox.cpp
        Mac/Source/FastTiming.cpp
        Mac/Source/GetApplicationInformation.cpp
        Mac/Source/MacClient.cpp
        Mac/Source/MacClientMenus.cpp
        Mac/Source/MoreCocoa.cpp
        Mac/Source/MoreFilesExtras.cpp
        Mac/Source/TemporaryItems.cpp
        Mac/Source/Utils.cpp
    )
elseif(UNIX)
    add_executable(Wow
        LinuxClient.cpp
    )
endif()

target_sources(Wow PRIVATE
    ClientCommands.cpp
    ClientGMCommands.cpp
    LoadingScreen.cpp
)

if(WIN32)
    target_sources(Wow PRIVATE Client.rc)
endif()

target_include_directories(Wow PRIVATE
    ../Common
    ../../Engine/Source
    ../../Storm/H
    ../../Engine/Source/lua/include

    .
)

set_target_properties(Wow PROPERTIES OUTPUT_NAME Wowae)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Wow PRIVATE /Gr /Z7)
    set_property(SOURCE Client.cpp APPEND PROPERTY COMPILE_OPTIONS /GR-)
    set_property(SOURCE Client.cpp APPEND PROPERTY COMPILE_DEFINITIONS WOW_STORM_ENTRYPOINT=1)
    set_property(TARGET Wow APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:StormStaticEntryPoint")
endif()

if(MSVC_VERSION EQUAL 1200)
    target_link_libraries(Wow PRIVATE unicows msvcrt)
endif()

if(WIN32)
    unset(D3DX9_IMPORT_LIBRARY CACHE)
    find_library(
        D3DX9_IMPORT_LIBRARY
        d3dx9
        PATHS
            "$ENV{DXSDK_DIR}/Lib/x86"
            "$ENV{DXSDK_DIR}/Lib"
        NO_DEFAULT_PATH)
    target_link_libraries(Wow PRIVATE
        comctl32
        winmm
        kernel32
        user32
        shell32
        advapi32
        wsock32
        opengl32
        glu32
        "${D3DX9_IMPORT_LIBRARY}"
        gdi32
        imm32
        ole32
        "${CMAKE_SOURCE_DIR}/Engine/Source/FMOD/api/lib/fmodvc.lib"
        uuid
    )
endif()

set(WOW_PROJECT_LIBRARIES
    Anim
    Base
    Event
    Frame
    MDLFile
    Model
    $<TARGET_NAME_IF_EXISTS:Os>
    Scrn
    Services
    Tempest
    SoundInterface
    ObjectAlloc
    ObjectMgrClient
    Glue
    Component
    Object
    UI
    UIUtil
    NetClient
    DBClient
    Gx
    Images
    FreeType
    CompFilenameDecoration
    BLPFile
    WowSvcsClient
    WowServices
    Gxu
    SRP
    ObjectClient
    ConsoleClient
    Magic
    Game
    StormStAE
    WorldClient
    GameClient
    WorldCommon
    LogClient
    MagicClient
    expat
    FrameXML
    lua
    FrameScript
    regex
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Wow PRIVATE -Wl,--start-group ${WOW_PROJECT_LIBRARIES} -Wl,--end-group)
else()
    target_link_libraries(Wow PRIVATE ${WOW_PROJECT_LIBRARIES})
endif()
