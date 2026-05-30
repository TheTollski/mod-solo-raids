#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <map>
#include <set>

namespace
{
constexpr uint32 NPC_KELTHUZAD = 15990;
constexpr uint32 NPC_KELTHUZAD_40 = 351019;
constexpr uint32 NPC_GUARDIAN_OF_ICECROWN = 16441;
constexpr uint32 NPC_GUARDIAN_OF_ICECROWN_40 = 351076;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;
constexpr uint32 MAX_SOLO_GUARDIANS = 2;

std::set<uint32> kelthuzadSoloAnnouncementMaps;
std::map<ObjectGuid, uint32> activeSoloGuardianInstances;

bool IsKelThuzad(Creature const* creature)
{
    if (!creature)
        return false;

    uint32 const entry = creature->GetEntry();
    return entry == NPC_KELTHUZAD || entry == NPC_KELTHUZAD_40;
}

bool IsGuardianOfIcecrown(Creature const* creature)
{
    if (!creature)
        return false;

    uint32 const entry = creature->GetEntry();
    return entry == NPC_GUARDIAN_OF_ICECROWN || entry == NPC_GUARDIAN_OF_ICECROWN_40;
}

uint32 CountActiveGuardians(uint32 instanceId)
{
    uint32 count = 0;
    for (auto const& guardian : activeSoloGuardianInstances)
        if (guardian.second == instanceId)
            ++count;

    return count;
}

void AnnounceKelThuzadSoloTweaks(Creature* kelthuzad)
{
    if (!kelthuzad)
        return;

    Player* player = SoloRaids::GetSoloPlayer(kelthuzad->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = kelthuzad->GetInstanceId();
    if (kelthuzadSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Kel'Thuzad. Guardian of Icecrown spawns capped at 2.");
    kelthuzadSoloAnnouncementMaps.insert(instanceId);
}

void LimitSoloGuardians(Creature* guardian)
{
    if (!guardian || !SoloRaids::IsSoloMap(guardian->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
        return;

    ObjectGuid const guid = guardian->GetGUID();
    if (activeSoloGuardianInstances.count(guid) != 0)
        return;

    uint32 const instanceId = guardian->GetInstanceId();
    if (CountActiveGuardians(instanceId) >= MAX_SOLO_GUARDIANS)
    {
        guardian->DespawnOrUnsummon(Milliseconds(100));
        return;
    }

    activeSoloGuardianInstances[guid] = instanceId;
}
}

class KelThuzadSoloRaidCreatureScript : public AllCreatureScript
{
public:
    KelThuzadSoloRaidCreatureScript() : AllCreatureScript("KelThuzadSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (IsKelThuzad(creature))
        {
            if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
                AnnounceKelThuzadSoloTweaks(creature);

            return;
        }

        if (IsGuardianOfIcecrown(creature))
            LimitSoloGuardians(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        if (IsKelThuzad(creature))
            kelthuzadSoloAnnouncementMaps.erase(creature->GetInstanceId());

        if (IsGuardianOfIcecrown(creature))
            activeSoloGuardianInstances.erase(creature->GetGUID());
    }
};

void AddKelThuzadSoloRaidScripts()
{
    new KelThuzadSoloRaidCreatureScript();
}
