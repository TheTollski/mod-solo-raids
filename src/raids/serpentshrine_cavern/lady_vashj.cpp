#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <chrono>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_LADY_VASHJ = 21212;
constexpr uint32 NPC_ENCHANTED_ELEMENTAL = 21958;
constexpr uint32 NPC_TAINTED_ELEMENTAL = 22009;
constexpr uint32 SPELL_TAINTED_CORE_PARALYZE = 38132;
constexpr uint32 SPELL_POISON_BOLT = 38253;
constexpr uint32 SPELL_SUMMON_TOXIC_SPOREBAT = 38494;
constexpr uint32 SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN = 548;

std::set<ObjectGuid> ladyVashjSoloAnnouncementSent;
std::set<ObjectGuid> taintedElementalDespawnCancelled;
std::map<ObjectGuid, uint32> activeSoloEnchantedElementalInstances;
std::map<uint32, std::chrono::steady_clock::time_point> lastSoloSporebatSummonTimes;

uint32 CountActiveEnchantedElementals(uint32 instanceId)
{
    uint32 count = 0;
    for (auto const& elemental : activeSoloEnchantedElementalInstances)
        if (elemental.second == instanceId)
            ++count;

    return count;
}

void LimitSoloEnchantedElementals(Creature* elemental)
{
    if (!elemental || elemental->GetEntry() != NPC_ENCHANTED_ELEMENTAL ||
        !SoloRaids::IsSoloMap(elemental->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
        return;

    ObjectGuid const guid = elemental->GetGUID();
    if (activeSoloEnchantedElementalInstances.count(guid) != 0)
        return;

    uint32 const instanceId = elemental->GetInstanceId();
    if (CountActiveEnchantedElementals(instanceId) >= SoloRaids::Config::LadyVashjEnchantedElementalMaxActive())
    {
        elemental->DespawnOrUnsummon(Milliseconds(100));
        return;
    }

    activeSoloEnchantedElementalInstances[guid] = instanceId;
}

void PreventSoloTaintedElementalDespawn(Creature* elemental)
{
    if (!elemental || elemental->GetEntry() != NPC_TAINTED_ELEMENTAL ||
        !SoloRaids::Config::PreventLadyVashjTaintedElementalDespawn() ||
        !SoloRaids::IsSoloMap(elemental->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
        return;

    if (taintedElementalDespawnCancelled.insert(elemental->GetGUID()).second)
        elemental->m_Events.KillAllEvents(false);
}

void AnnounceLadyVashjSoloTweaks(Creature* vashj)
{
    if (!vashj)
        return;

    ObjectGuid const guid = vashj->GetGUID();
    if (ladyVashjSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(vashj->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Serpentshrine Cavern solo tweaks enabled for Lady Vashj.";
    if (SoloRaids::Config::DisableLadyVashjTaintedCoreParalyze())
        message += " Tainted Core Paralyze disabled.";
    if (SoloRaids::Config::PreventLadyVashjTaintedElementalDespawn())
        message += " Tainted Elementals do not despawn.";

    message += " Tainted Elemental Poison Bolt range set to " +
        std::to_string(uint32(SoloRaids::Config::LadyVashjTaintedElementalPoisonBoltRange())) + " yards.";
    message += " Enchanted Elementals capped at " +
        std::to_string(SoloRaids::Config::LadyVashjEnchantedElementalMaxActive()) + ".";
    message += " Toxic Sporebat minimum spawn interval set to " +
        std::to_string(SoloRaids::Config::LadyVashjSporebatMinSpawnIntervalMs() / 1000) + " seconds.";

    player->SendSystemMessage(message.c_str());
    ladyVashjSoloAnnouncementSent.insert(guid);
}
}

class LadyVashjSoloRaidCreatureScript : public AllCreatureScript
{
public:
    LadyVashjSoloRaidCreatureScript() : AllCreatureScript("LadyVashjSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_LADY_VASHJ && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            AnnounceLadyVashjSoloTweaks(creature);

        if (creature->GetEntry() == NPC_ENCHANTED_ELEMENTAL)
            LimitSoloEnchantedElementals(creature);
        else if (creature->GetEntry() == NPC_TAINTED_ELEMENTAL)
            PreventSoloTaintedElementalDespawn(creature);
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        LimitSoloEnchantedElementals(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_LADY_VASHJ)
        {
            ladyVashjSoloAnnouncementSent.erase(creature->GetGUID());
            lastSoloSporebatSummonTimes.erase(creature->GetInstanceId());
        }

        taintedElementalDespawnCancelled.erase(creature->GetGUID());
        activeSoloEnchantedElementalInstances.erase(creature->GetGUID());
    }
};

class LadyVashjSoloRaidUnitScript : public UnitScript
{
public:
    LadyVashjSoloRaidUnitScript() : UnitScript("LadyVashjSoloRaidUnitScript", true, { UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (SoloRaids::Config::DisableLadyVashjTaintedCoreParalyze() &&
            SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            unit->RemoveAurasDueToSpell(SPELL_TAINTED_CORE_PARALYZE);
    }
};

class LadyVashjSoloRaidSpellScript : public AllSpellScript
{
public:
    LadyVashjSoloRaidSpellScript() : AllSpellScript("LadyVashjSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || spell->GetSpellInfo()->Id != SPELL_SUMMON_TOXIC_SPOREBAT)
            return true;

        Unit* caster = spell->GetCaster();
        if (!caster || caster->GetEntry() != NPC_LADY_VASHJ ||
            !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return true;

        uint32 const minimumIntervalMs = SoloRaids::Config::LadyVashjSporebatMinSpawnIntervalMs();
        uint32 const instanceId = caster->GetInstanceId();
        auto const now = std::chrono::steady_clock::now();
        auto const itr = lastSoloSporebatSummonTimes.find(instanceId);
        if (itr != lastSoloSporebatSummonTimes.end() &&
            now - itr->second < std::chrono::milliseconds(minimumIntervalMs))
            return false;

        lastSoloSporebatSummonTimes[instanceId] = now;
        return true;
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_POISON_BOLT)
            return;

        Unit* caster = spell->GetCaster();
        Unit* target = spell->m_targets.GetUnitTarget();
        if (!caster || !target || caster->GetEntry() != NPC_TAINTED_ELEMENTAL ||
            !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return;

        if (caster->GetDistance(target) > SoloRaids::Config::LadyVashjTaintedElementalPoisonBoltRange())
            result = SPELL_FAILED_OUT_OF_RANGE;
    }
};

void AddLadyVashjSoloRaidScripts()
{
    new LadyVashjSoloRaidCreatureScript();
    new LadyVashjSoloRaidUnitScript();
    new LadyVashjSoloRaidSpellScript();
}
