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
constexpr uint32 NPC_AURIAYA = 33515;
constexpr uint32 NPC_FERAL_DEFENDER = 34035;
constexpr uint32 NPC_SANCTUM_SENTRY = 34014;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

struct AddHealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, AddHealthScaleState> addHealthScaleStates;
std::set<ObjectGuid> auriayaSoloAnnouncementSent;

bool IsAuriayaAdd(Unit const* unit)
{
    return unit && (unit->GetEntry() == NPC_SANCTUM_SENTRY || unit->GetEntry() == NPC_FERAL_DEFENDER);
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

void ScaleAuriayaAddMaxHealth(Creature* creature)
{
    if (!IsAuriayaAdd(creature))
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto stateItr = addHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
    {
        if (stateItr != addHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(creature, stateItr->second.baselineMaxHealth);
            addHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == addHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = creature->GetMaxHealth();
        stateItr = addHealthScaleStates.emplace(
            guid, AddHealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (creature->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        creature->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = creature->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::AuriayaAddMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(creature, scaledMaxHealth);
}

void AnnounceAuriayaSoloTweaks(Creature* auriaya)
{
    if (!auriaya)
        return;

    ObjectGuid const guid = auriaya->GetGUID();
    if (auriayaSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(auriaya->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for Auriaya. Sanctum Sentry and Feral Defender max health set to " +
        std::to_string(uint32(SoloRaids::Config::AuriayaAddMaxHealthPct() * 100.0f)) + "% after other scaling.").c_str());
    auriayaSoloAnnouncementSent.insert(guid);
}
}

class AuriayaSoloRaidCreatureScript : public AllCreatureScript
{
public:
    AuriayaSoloRaidCreatureScript() : AllCreatureScript("AuriayaSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (IsAuriayaAdd(creature))
        {
            ScaleAuriayaAddMaxHealth(creature);
            return;
        }

        if (creature->GetEntry() == NPC_AURIAYA && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceAuriayaSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        addHealthScaleStates.erase(creature->GetGUID());
        if (creature->GetEntry() == NPC_AURIAYA)
            auriayaSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

void AddAuriayaSoloRaidScripts()
{
    new AuriayaSoloRaidCreatureScript();
}
