#ifndef MOD_SOLO_RAIDS_BLACKWING_LAIR_SOLO_RAID_UTILS_H
#define MOD_SOLO_RAIDS_BLACKWING_LAIR_SOLO_RAID_UTILS_H

class Map;
class Player;
class Unit;

namespace SoloRaids::BlackwingLair
{
bool IsSoloRaidMap(Map const* map);
Player* GetSoloRaidPlayer(Map const* map);
bool IsSoloRaidPlayer(Unit const* unit);
}

#endif
