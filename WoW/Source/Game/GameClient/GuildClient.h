#ifndef WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H
#define WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H

unsigned int __fastcall GuildGetTabardCost();

bool __fastcall GuildGetGuildTabard(
    unsigned int guildID,
    void(__fastcall *callback)(int, const unsigned __int64 &, void *, bool),
    int &eStyle,
    int &eColor,
    int &bStyle,
    int &bColor,
    int &background
);

#endif
