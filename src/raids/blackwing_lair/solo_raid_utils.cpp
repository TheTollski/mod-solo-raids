#include "solo_raid_utils.h"

#include "Map.h"
#include "Player.h"
#include "Unit.h"

namespace SoloRaids::BlackwingLair
{
namespace
{
constexpr uint32 MAP_BLACKWING_LAIR = 469;
}

bool IsSoloRaidMap(Map const* map)
{
    if (!map || map->GetId() != MAP_BLACKWING_LAIR)
        return false;

    uint32 playerCount = 0;

    for (Map::PlayerList::const_iterator itr = map->GetPlayers().begin(); itr != map->GetPlayers().end(); ++itr)
    {
        Player const* player = itr->GetSource();
        if (!player || player->IsGameMaster())
            continue;

        if (++playerCount > 1)
            return false;
    }

    return playerCount == 1;
}

Player* GetSoloRaidPlayer(Map const* map)
{
    if (!map || map->GetId() != MAP_BLACKWING_LAIR)
        return nullptr;

    Player* soloPlayer = nullptr;

    for (Map::PlayerList::const_iterator itr = map->GetPlayers().begin(); itr != map->GetPlayers().end(); ++itr)
    {
        Player* player = itr->GetSource();
        if (!player || player->IsGameMaster())
            continue;

        if (soloPlayer)
            return nullptr;

        soloPlayer = player;
    }

    return soloPlayer;
}

bool IsSoloRaidPlayer(Unit const* unit)
{
    Player const* player = unit ? unit->ToPlayer() : nullptr;
    if (!player || player->IsGameMaster())
        return false;

    return GetSoloRaidPlayer(player->GetMap()) == player;
}
}
