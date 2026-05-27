#ifndef MOD_SOLO_RAIDS_SOLO_RAID_UTILS_H
#define MOD_SOLO_RAIDS_SOLO_RAID_UTILS_H

#include "Define.h"

class Map;
class Player;
class Unit;

namespace SoloRaids
{
bool IsSoloMap(Map const* map);
bool IsSoloMap(Map const* map, uint32 expectedMapId);
Player* GetSoloPlayer(Map const* map);
Player* GetSoloPlayer(Map const* map, uint32 expectedMapId);
bool IsSoloPlayer(Unit const* unit);
bool IsSoloPlayer(Unit const* unit, uint32 expectedMapId);
}

#endif
