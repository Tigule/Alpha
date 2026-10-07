add_library(MDLFile STATIC
    Attachment.cpp
    BinHandlers.cpp
    Bone.cpp
    Camera.cpp
    Collision.cpp
    EventObject.cpp
    GenObject.cpp
    Geoset.cpp
    Header.cpp
    Helper.cpp
    HitTest.cpp
    lexout.cpp
    Light.cpp
    Material.cpp
    MDLFile.cpp
    MDLStatus.cpp
    Model.cpp
    Parser.cpp
    ParticleEmitter.cpp
    ParticleEmitter2.cpp
    PivotPoint.cpp
    RibbonEmitter.cpp
    Sequences.cpp
    TextHandlers.cpp
    Texture.cpp
    TextureAnim.cpp
    TokenText.cpp
    TSet.cpp
    Version.cpp
)

target_include_directories(MDLFile PRIVATE
    ..
    ../../../Storm/H
)

set_source_files_properties(lexout.cpp PROPERTIES OBJECT_NAME lex)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(MDLFile PRIVATE /Gr /Z7)
endif()
