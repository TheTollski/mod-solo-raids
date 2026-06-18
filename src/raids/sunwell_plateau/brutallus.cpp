#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_BRUTALLUS = 24882;
constexpr uint32 SPELL_METEOR_SLASH = 45150;
constexpr uint32 SOLO_RAIDS_MAP_SUNWELL_PLATEAU = 580;

std::set<ObjectGuid> brutallusSoloAnnouncementSent;

struct BrutallusHealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, BrutallusHealthScaleState> brutallusHealthScaleStates;

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

void ScaleBrutallusMaxHealth(Creature* brutallus)
{
    if (!brutallus || brutallus->GetEntry() != NPC_BRUTALLUS)
        return;

    ObjectGuid const guid = brutallus->GetGUID();
    auto stateItr = brutallusHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(brutallus->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
    {
        if (stateItr != brutallusHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(brutallus, stateItr->second.baselineMaxHealth);
            brutallusHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == brutallusHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = brutallus->GetMaxHealth();
        stateItr = brutallusHealthScaleStates.emplace(
            guid, BrutallusHealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (brutallus->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        brutallus->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = brutallus->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::BrutallusMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(brutallus, scaledMaxHealth);
}

void CapMeteorSlashStacks(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
        return;

    Aura* aura = unit->GetAura(SPELL_METEOR_SLASH);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::BrutallusMeteorSlashMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceBrutallusSoloTweaks(Creature* brutallus)
{
    if (!brutallus)
        return;

    ObjectGuid const guid = brutallus->GetGUID();
    if (brutallusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(brutallus->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Sunwell Plateau solo tweaks enabled for Brutallus. Max health set to " +
        std::to_string(uint32(SoloRaids::Config::BrutallusMaxHealthPct() * 100.0f)) +
        "% after other scaling. Melee damage set to " +
        std::to_string(uint32(SoloRaids::Config::BrutallusMeleeDamagePct() * 100.0f)) + "%. Meteor Slash capped at " +
        std::to_string(uint32(SoloRaids::Config::BrutallusMeteorSlashMaxStacks())) + " stacks.").c_str());
    brutallusSoloAnnouncementSent.insert(guid);
}
}

class BrutallusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    BrutallusSoloRaidCreatureScript() : AllCreatureScript("BrutallusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_BRUTALLUS)
            return;

        ScaleBrutallusMaxHealth(creature);

        if (creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            AnnounceBrutallusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_BRUTALLUS)
        {
            brutallusSoloAnnouncementSent.erase(creature->GetGUID());
            brutallusHealthScaleStates.erase(creature->GetGUID());
        }
    }
};

class BrutallusSoloRaidUnitScript : public UnitScript
{
public:
    BrutallusSoloRaidUnitScript() : UnitScript("BrutallusSoloRaidUnitScript", true, { UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!attacker || attacker->GetEntry() != NPC_BRUTALLUS ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::BrutallusMeleeDamagePct());
    }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || aura->GetId() != SPELL_METEOR_SLASH)
            return;

        CapMeteorSlashStacks(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapMeteorSlashStacks(unit);
    }
};

void AddBrutallusSoloRaidScripts()
{
    new BrutallusSoloRaidCreatureScript();
    new BrutallusSoloRaidUnitScript();
}
