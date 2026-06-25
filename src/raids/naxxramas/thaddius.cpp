#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Unit.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_THADDIUS = 15928;
constexpr uint32 NPC_THADDIUS_40 = 351000;
constexpr uint32 NPC_FEUGEN = 15930;
constexpr uint32 NPC_FEUGEN_40 = 351002;
constexpr uint32 SPELL_STATIC_FIELD = 28135;
constexpr uint32 SPELL_TESLA_SHOCK = 28099;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;

std::set<uint32> thaddiusSoloAnnouncementMaps;

struct ThaddiusHealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, ThaddiusHealthScaleState> thaddiusHealthScaleStates;

bool IsThaddius(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_THADDIUS || entry == NPC_THADDIUS_40;
}

bool IsFeugen(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FEUGEN || entry == NPC_FEUGEN_40;
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

void ScaleThaddiusMaxHealth(Creature* thaddius)
{
    if (!thaddius || !IsThaddius(thaddius))
        return;

    ObjectGuid const guid = thaddius->GetGUID();
    auto stateItr = thaddiusHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(thaddius->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
    {
        if (stateItr != thaddiusHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(thaddius, stateItr->second.baselineMaxHealth);
            thaddiusHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == thaddiusHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = thaddius->GetMaxHealth();
        stateItr = thaddiusHealthScaleStates.emplace(
            guid, ThaddiusHealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (thaddius->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        thaddius->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = thaddius->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::ThaddiusMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(thaddius, scaledMaxHealth);
}

void AnnounceThaddiusSoloTweaks(Creature* creature)
{
    if (!creature)
        return;

    Player* player = SoloRaids::GetSoloPlayer(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = creature->GetInstanceId();
    if (thaddiusSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    bool const disableStaticFieldManaDrain = SoloRaids::Config::DisableThaddiusStaticFieldManaDrain();
    bool const disableTeslaShock = SoloRaids::Config::DisableThaddiusTeslaShock();
    float const maxHealthPct = SoloRaids::Config::ThaddiusMaxHealthPct();
    if (!disableStaticFieldManaDrain && !disableTeslaShock && maxHealthPct == 1.0f)
        return;

    std::string message = "mod-solo-raids active: Naxxramas solo tweaks enabled for Thaddius.";
    if (disableStaticFieldManaDrain)
        message += " Feugen's Static Field mana drain disabled.";
    if (disableTeslaShock)
        message += " Tesla Shock disabled during phase one.";
    if (maxHealthPct != 1.0f)
        message += " Thaddius max health set to " + std::to_string(uint32(maxHealthPct * 100.0f)) + "% after other scaling.";

    player->SendSystemMessage(message.c_str());
    thaddiusSoloAnnouncementMaps.insert(instanceId);
}
}

class ThaddiusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ThaddiusSoloRaidCreatureScript() : AllCreatureScript("ThaddiusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || (!IsFeugen(creature) && !IsThaddius(creature)))
            return;

        if (IsThaddius(creature))
            ScaleThaddiusMaxHealth(creature);

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnounceThaddiusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || (!IsFeugen(creature) && !IsThaddius(creature)))
            return;

        thaddiusSoloAnnouncementMaps.erase(creature->GetInstanceId());
        if (IsThaddius(creature))
            thaddiusHealthScaleStates.erase(creature->GetGUID());
    }
};

class ThaddiusSoloRaidSpellScript : public AllSpellScript
{
public:
    ThaddiusSoloRaidSpellScript() : AllSpellScript("ThaddiusSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || spell->GetSpellInfo()->Id != SPELL_TESLA_SHOCK ||
            !SoloRaids::Config::DisableThaddiusTeslaShock())
            return true;

        Unit* caster = spell->GetCaster();
        return !caster || !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    }
};

class ThaddiusSoloRaidUnitScript : public UnitScript
{
public:
    ThaddiusSoloRaidUnitScript() : UnitScript("ThaddiusSoloRaidUnitScript", true, { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& amount, SpellInfo const* spellInfo) override
    {
        if (!SoloRaids::Config::DisableThaddiusStaticFieldManaDrain() || !target || !source || !spellInfo || spellInfo->Id != SPELL_STATIC_FIELD || !IsFeugen(source) || !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        if (target->GetMaxPower(POWER_MANA) <= 0)
            return;

        int32 const manaToRestore = std::min<int32>(500, target->GetMaxPower(POWER_MANA) - target->GetPower(POWER_MANA));
        if (manaToRestore > 0)
            target->ModifyPower(POWER_MANA, manaToRestore);
    }
};

void AddThaddiusSoloRaidScripts()
{
    new ThaddiusSoloRaidCreatureScript();
    new ThaddiusSoloRaidSpellScript();
    new ThaddiusSoloRaidUnitScript();
}
