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
elseif(APPLE)
    add_subdirectory(WowError)
endif()

if(WIN32)
    add_executable(Wow WIN32
        Client.cpp
    )
elseif(APPLE)
    add_executable(Wow MACOSX_BUNDLE
        Client.cpp
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
    set_source_files_properties(
        Client.cpp
        Mac/Source/AboutBox.cpp
        Mac/Source/MacClient.cpp
        Mac/Source/MacClientMenus.cpp
        Mac/Source/MoreCocoa.cpp
        PROPERTIES LANGUAGE OBJCXX
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
    set(CMAKE_RC_OUTPUT_EXTENSION_REPLACE 1)
    target_sources(Wow PRIVATE Script1.rc)

    # not our branding to distribute
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/BlizzardIcon.ico")
        set_property(SOURCE Script1.rc APPEND PROPERTY COMPILE_DEFINITIONS HAVE_CLIENT_ICON)
    endif()

    set_property(SOURCE Script1.rc APPEND PROPERTY COMPILE_DEFINITIONS HAVE_CLIENT_CURSOR)
endif()

target_include_directories(Wow PRIVATE
    ../Common
    ../../Engine/Source
    ../../Storm/H
    ../../Engine/Source/lua/include

    .
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WOW_ARCH 64)
else()
    set(WOW_ARCH "")
endif()

set_target_properties(Wow PROPERTIES
    OUTPUT_NAME Wow${WOW_ARCH}
    OUTPUT_NAME_RELEASE_ASSERTIONS_ENABLED Wow${WOW_ARCH}ae
    OUTPUT_NAME_RELEASE Wow${WOW_ARCH}
    OUTPUT_NAME_DEBUG Wow${WOW_ARCH}d
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Wow PRIVATE /Gr /Zi)
endif()

if(MSVC)
    set_property(SOURCE Client.cpp APPEND PROPERTY COMPILE_OPTIONS /GR-)
    set_property(SOURCE Client.cpp APPEND PROPERTY COMPILE_DEFINITIONS WOW_STORM_ENTRYPOINT=1)
    set_property(TARGET Wow APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:StormStaticEntryPoint")
endif()

if(MSVC_VERSION EQUAL 1200)
    target_link_libraries(Wow PRIVATE unicows msvcrt)
elseif(MSVC)
    target_link_options(Wow PRIVATE /MANIFEST:NO)
endif()

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FMOD_IMPORT_LIBRARY "${CMAKE_SOURCE_DIR}/Engine/Source/FMOD/api/lib/fmod64vc.lib")
    else()
        set(FMOD_IMPORT_LIBRARY "${CMAKE_SOURCE_DIR}/Engine/Source/FMOD/api/lib/fmodvc.lib")
    endif()

    unset(D3DX9_IMPORT_LIBRARY CACHE)
    if(NOT HAVE_WIN8_SDK)
        find_library(
            D3DX9_IMPORT_LIBRARY
            d3dx9
            PATHS
                "$ENV{DXSDK_DIR}/Lib/x86"
                "$ENV{DXSDK_DIR}/Lib"
            NO_DEFAULT_PATH)
    endif()

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
        "${FMOD_IMPORT_LIBRARY}"
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
    Storm
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

if(APPLE)
    target_link_libraries(Wow PRIVATE
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework Carbon"
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework OpenGL"
    )

    set_target_properties(Wow PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Mac/Info.plist
        OUTPUT_NAME "World of Warcraft"
        OUTPUT_NAME_RELEASE_ASSERTIONS_ENABLED "World of Warcraft"
        OUTPUT_NAME_RELEASE "World of Warcraft"
        OUTPUT_NAME_DEBUG "World of Warcraft"
    )

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo "APPLWoW!")

    add_dependencies(Wow WowError)
    add_custom_command(TARGET Wow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_BUNDLE_DIR:WowError> $<TARGET_BUNDLE_CONTENT_DIR:Wow>/Resources/WoWError.app
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo $<TARGET_BUNDLE_CONTENT_DIR:Wow>/PkgInfo
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Wow PRIVATE -Wl,--start-group ${WOW_PROJECT_LIBRARIES} -Wl,--end-group)
else()
    target_link_libraries(Wow PRIVATE ${WOW_PROJECT_LIBRARIES})
endif()
