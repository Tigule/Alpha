add_library(WorldClient STATIC
    ../../Common/AaBsp.cpp
    AreaList.cpp
    DetailDoodad.cpp
    Map.cpp
    MapArea.cpp
    MapBaseObj.cpp
    MapChunk.cpp
    MapChunkRender.cpp
    MapClip.cpp
    MapDoodadDef.cpp
    MapEntity.cpp
    MapLight.cpp
    MapLoad.cpp
    MapLod.cpp
    MapMem.cpp
    MapObj.cpp
    MapObjDef.cpp
    MapObjGroup.cpp
    MapObjRead.cpp
    MapObjRender.cpp
    MapObjUpdate.cpp
    MapPrepare.cpp
    MapPurge.cpp
    MapQuery.cpp
    MapRender.cpp
    MapTexture.cpp
    MapUnload.cpp
    MapUpdate.cpp
    MapWater.cpp
    SimpleDoodad.cpp
    ../DB/DBClient/AutoCode/VideoHardwareRec.cpp
    World.cpp
    WorldParam.cpp
    WorldScene.cpp
)

target_include_directories(WorldClient PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H
    ../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(WorldClient PRIVATE /Gr /Z7 /YX)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES vc60.pch)
    set_source_files_properties(Map.cpp MapQuery.cpp PROPERTIES COMPILE_OPTIONS /QIfist)
endif()
