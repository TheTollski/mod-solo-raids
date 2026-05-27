#include "solo_raid_utils.h"

#include "Map.h"
#include "Player.h"
#include "Unit.h"

namespace SoloRaids
{
Player* GetSoloPlayer(Map const* map)
{
    if (!map || (!map->IsDungeon() && !map->IsRaid()))
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

Player* GetSoloPlayer(Map const* map, uint32 expectedMapId)
{
    if (!map || map->GetId() != expectedMapId)
        return nullptr;

    return GetSoloPlayer(map);
}

bool IsSoloMap(Map const* map)
{
    return GetSoloPlayer(map) != nullptr;
}

bool IsSoloMap(Map const* map, uint32 expectedMapId)
{
    return GetSoloPlayer(map, expectedMapId) != nullptr;
}

bool IsSoloPlayer(Unit const* unit)
{
    Player const* player = unit ? unit->ToPlayer() : nullptr;
    if (!player || player->IsGameMaster())
        return false;

    return GetSoloPlayer(player->GetMap()) == player;
}

bool IsSoloPlayer(Unit const* unit, uint32 expectedMapId)
{
    Player const* player = unit ? unit->ToPlayer() : nullptr;
    if (!player || player->IsGameMaster())
        return false;

    return GetSoloPlayer(player->GetMap(), expectedMapId) == player;
}
}
