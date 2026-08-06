#ifndef WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H
#define WOW_SOURCE_GAME_GAMECLIENT_GUILDCLIENT_H

#include <Base/Base.h>

UINT GuildGetTabardCost();

bool GuildGetGuildTabard(
    UINT guildID,
    void (*callback)(int, const DWORDLONG &, LPVOID, bool),
    int &eStyle,
    int &eColor,
    int &bStyle,
    int &bColor,
    int &background
);

#endif
