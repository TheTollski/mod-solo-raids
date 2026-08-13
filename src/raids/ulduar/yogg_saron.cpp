#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "InstanceScript.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_SARA = 33134;
constexpr uint32 NPC_YOGG_SARON = 33288;
constexpr uint32 NPC_IMMORTAL_GUARDIAN = 33988;
constexpr uint32 NPC_MARKED_IMMORTAL_GUARDIAN = 36064;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;
constexpr uint32 DATA_SARA = 760;
constexpr int32 ACTION_BRAIN_DAMAGED = -8;

struct HealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, HealthScaleState> immortalGuardianHealthScaleStates;
std::set<ObjectGuid> yoggSoloAnnouncementSent;
std::set<ObjectGuid> yoggSoloPhaseTwoSkipped;

bool IsImmortalGuardian(Creature const* creature)
{
    return creature && (creature->GetEntry() == NPC_IMMORTAL_GUARDIAN ||
        creature->GetEntry() == NPC_MARKED_IMMORTAL_GUARDIAN);
}

Creature* GetSara(Creature* yogg)
{
    if (!yogg)
        return nullptr;

    if (InstanceScript* instance = yogg->GetInstanceScript())
        if (Creature* sara = instance->GetCreature(DATA_SARA))
            return sara;

    return yogg->FindNearestCreature(NPC_SARA, 500.0f, true);
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

void ScaleImmortalGuardianMaxHealth(Creature* creature)
{
    if (!IsImmortalGuardian(creature))
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto stateItr = immortalGuardianHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
    {
        if (stateItr != immortalGuardianHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(creature, stateItr->second.baselineMaxHealth);
            immortalGuardianHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == immortalGuardianHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = creature->GetMaxHealth();
        stateItr = immortalGuardianHealthScaleStates.emplace(
            guid, HealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (creature->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        creature->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = creature->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::YoggSaronImmortalGuardianMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(creature, scaledMaxHealth);
}

void SkipPhaseTwo(Creature* yogg)
{
    if (!yogg || yogg->GetEntry() != NPC_YOGG_SARON ||
        !SoloRaids::Config::SkipYoggSaronPhaseTwo() ||
        !yogg->IsInCombat() ||
        !yogg->IsVisible() ||
        !SoloRaids::IsSoloMap(yogg->GetMap(), SOLO_RAIDS_MAP_ULDUAR) ||
        yoggSoloPhaseTwoSkipped.count(yogg->GetGUID()) != 0)
        return;

    Creature* sara = GetSara(yogg);
    if (!sara)
        return;

    yoggSoloPhaseTwoSkipped.insert(yogg->GetGUID());
    sara->AI()->DoAction(ACTION_BRAIN_DAMAGED);
}

void AnnounceYoggSaronSoloTweaks(Creature* yogg)
{
    if (!yogg || !SoloRaids::Config::SkipYoggSaronPhaseTwo())
        return;

    ObjectGuid const guid = yogg->GetGUID();
    if (yoggSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(yogg->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for Yogg-Saron. Phase 2 skipped. Immortal Guardian max health set to " +
        std::to_string(uint32(SoloRaids::Config::YoggSaronImmortalGuardianMaxHealthPct() * 100.0f)) + "% after other scaling.").c_str());
    yoggSoloAnnouncementSent.insert(guid);
}
}

class YoggSaronSoloRaidCreatureScript : public AllCreatureScript
{
public:
    YoggSaronSoloRaidCreatureScript() : AllCreatureScript("YoggSaronSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (IsImmortalGuardian(creature))
        {
            ScaleImmortalGuardianMaxHealth(creature);
            return;
        }

        if (creature->GetEntry() != NPC_YOGG_SARON)
            return;

        SkipPhaseTwo(creature);

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceYoggSaronSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        immortalGuardianHealthScaleStates.erase(creature->GetGUID());

        if (creature->GetEntry() == NPC_YOGG_SARON)
        {
            yoggSoloAnnouncementSent.erase(creature->GetGUID());
            yoggSoloPhaseTwoSkipped.erase(creature->GetGUID());
        }
    }
};

void AddYoggSaronSoloRaidScripts()
{
    new YoggSaronSoloRaidCreatureScript();
}
