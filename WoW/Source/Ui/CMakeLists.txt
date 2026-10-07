add_library(UI STATIC
    ActionBarFrame.cpp
    BankFrame.cpp
    BuffFrame.cpp
    CharacterModelBase.cpp
    ChatFrame.cpp
    ClassTrainerFrame.cpp
    ContainerFrame.cpp
    CraftInfo.cpp
    DuelInfo.cpp
    FadingScreen.cpp
    GameUI.cpp
    GuildRegistrar.cpp
    ItemTextFrame.cpp
    LootFrame.cpp
    MerchantFrame.cpp
    MinimapFrame.cpp
    NamePlateFrame.cpp
    PaperDollInfoFrame.cpp
    PartyFrame.cpp
    PetInfo.cpp
    PetitionInfo.cpp
    QuestFrame.cpp
    QuestLog.cpp
    QuestTextParser.cpp
    ReputationInfo.cpp
    SpellBookFrame.cpp
    TabardCreationFrame.cpp
    TabardModelFrame.cpp
    TaxiMapFrame.cpp
    TradeFrame.cpp
    TradeSkillFrame.cpp
    Tutorial.cpp
    UIBindings.cpp
    WorldFrame.cpp
    WorldMap.cpp
)

target_include_directories(UI PRIVATE
    ..
    ../../Common
    ../../../Engine/Source
    ../../../Storm/H

    ../../../Engine/Source/lua/include
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(UI PRIVATE /Gr /Z7)
endif()
