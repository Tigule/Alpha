#ifndef WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H
#define WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H

unsigned int GuildGetTabardCost();

bool GuildGetGuildTabard(
    unsigned int guildID,
    void(*callback)(int, const unsigned __int64 &, void *, bool),
    int &eStyle,
    int &eColor,
    int &bStyle,
    int &bColor,
    int &background
);

#endif
