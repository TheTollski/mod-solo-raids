#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_HODIR = 32845;
constexpr uint32 NPC_FLASH_FREEZE_PLR = 32926;
constexpr uint32 NPC_FLASH_FREEZE_NPC = 32938;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

struct HealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, HealthScaleState> healthScaleStates;
std::set<ObjectGuid> hodirSoloAnnouncementSent;

bool IsFlashFreeze(Creature const* creature)
{
    return creature && (creature->GetEntry() == NPC_FLASH_FREEZE_PLR || creature->GetEntry() == NPC_FLASH_FREEZE_NPC);
}

float GetMaxHealthPct(Creature const* creature)
{
    if (!creature)
        return 1.0f;

    if (creature->GetEntry() == NPC_HODIR)
        return SoloRaids::Config::HodirMaxHealthPct();

    if (IsFlashFreeze(creature))
        return SoloRaids::Config::HodirFlashFreezeMaxHealthPct();

    return 1.0f;
}

bool IsScaledCreature(Creature const* creature)
{
    return creature && (creature->GetEntry() == NPC_HODIR || IsFlashFreeze(creature));
}

void SetMaxHealthPreservingPct(Creature* creature, uint32 maxHealth)
{
    if (!creature || creature->GetMaxHealth() == maxHealth)
        return;

    float const healthPct = creature->GetMaxHealth() > 0
        ? float(creature->GetHealth()) / float(creature->GetMaxHealth())
        : 1.0f;

    creature->SetMaxHealth(maxHealth);
    uint32 newHealth = uint32(float(maxHealth) * healthPct);
    if (creature->IsAlive())
        newHealth = std::max<uint32>(1, newHealth);
    creature->SetHealth(std::min(maxHealth, newHealth));
}

void ScaleMaxHealth(Creature* creature)
{
    if (!IsScaledCreature(creature))
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto stateItr = healthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
    {
        if (stateItr != healthScaleStates.end())
        {
            SetMaxHealthPreservingPct(creature, stateItr->second.baselineMaxHealth);
            healthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == healthScaleStates.end())
    {
        uint32 const currentMaxHealth = creature->GetMaxHealth();
        stateItr = healthScaleStates.emplace(
            guid, HealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (creature->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        creature->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = creature->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * GetMaxHealthPct(creature)));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(creature, scaledMaxHealth);
}

void AnnounceHodirSoloTweaks(Creature* hodir)
{
    if (!hodir)
        return;

    ObjectGuid const guid = hodir->GetGUID();
    if (hodirSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(hodir->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for Hodir. Hodir max health set to " +
        std::to_string(uint32(SoloRaids::Config::HodirMaxHealthPct() * 100.0f)) +
        "% after other scaling. Flash Freeze max health set to " +
        std::to_string(uint32(SoloRaids::Config::HodirFlashFreezeMaxHealthPct() * 100.0f)) +
        "% after other scaling.").c_str());
    hodirSoloAnnouncementSent.insert(guid);
}
}

class HodirSoloRaidCreatureScript : public AllCreatureScript
{
public:
    HodirSoloRaidCreatureScript() : AllCreatureScript("HodirSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (IsScaledCreature(creature))
            ScaleMaxHealth(creature);

        if (creature->GetEntry() == NPC_HODIR && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceHodirSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        healthScaleStates.erase(creature->GetGUID());
        if (creature->GetEntry() == NPC_HODIR)
            hodirSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

void AddHodirSoloRaidScripts()
{
    new HodirSoloRaidCreatureScript();
}
