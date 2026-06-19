#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_MURU = 25741;
constexpr uint32 NPC_DARK_FIEND = 25744;
constexpr uint32 NPC_SHADOWSWORD_BERSERKER = 25798;
constexpr uint32 NPC_SHADOWSWORD_FURY_MAGE = 25799;
constexpr uint32 SPELL_SUMMON_DARK_FIEND_FIRST = 46000;
constexpr uint32 SPELL_SUMMON_DARK_FIEND_LAST = 46007;
constexpr uint32 SPELL_SUMMON_DARK_FIEND_ENTROPIUS = 46263;
constexpr uint32 SOLO_RAIDS_MAP_SUNWELL_PLATEAU = 580;

struct ShadowswordHealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, ShadowswordHealthScaleState> shadowswordHealthScaleStates;
std::set<ObjectGuid> muruSoloAnnouncementSent;

bool IsShadowswordAdd(Unit const* unit)
{
    return unit && (unit->GetEntry() == NPC_SHADOWSWORD_BERSERKER ||
        unit->GetEntry() == NPC_SHADOWSWORD_FURY_MAGE);
}

bool IsDarkFiendSummonSpell(uint32 spellId)
{
    return (spellId >= SPELL_SUMMON_DARK_FIEND_FIRST && spellId <= SPELL_SUMMON_DARK_FIEND_LAST) ||
        spellId == SPELL_SUMMON_DARK_FIEND_ENTROPIUS;
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

void ScaleShadowswordMaxHealth(Creature* creature)
{
    if (!IsShadowswordAdd(creature))
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto stateItr = shadowswordHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
    {
        if (stateItr != shadowswordHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(creature, stateItr->second.baselineMaxHealth);
            shadowswordHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == shadowswordHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = creature->GetMaxHealth();
        stateItr = shadowswordHealthScaleStates.emplace(
            guid, ShadowswordHealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (creature->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        creature->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = creature->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) *
            SoloRaids::Config::MuruShadowswordAddsMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(creature, scaledMaxHealth);
}

void AnnounceMuruSoloTweaks(Creature* muru)
{
    if (!muru)
        return;

    ObjectGuid const guid = muru->GetGUID();
    if (muruSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(muru->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Sunwell Plateau solo tweaks enabled for M'uru. Shadowsword add max health set to " +
        std::to_string(uint32(SoloRaids::Config::MuruShadowswordAddsMaxHealthPct() * 100.0f)) +
        "% after other scaling and damage set to " +
        std::to_string(uint32(SoloRaids::Config::MuruShadowswordAddsDamagePct() * 100.0f)) + "%." +
        (SoloRaids::Config::PreventMuruDarkFiendSpawns() ? " Dark Fiend spawns prevented." : "")).c_str());
    muruSoloAnnouncementSent.insert(guid);
}
}

class MuruSoloRaidCreatureScript : public AllCreatureScript
{
public:
    MuruSoloRaidCreatureScript() : AllCreatureScript("MuruSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_DARK_FIEND &&
            SoloRaids::Config::PreventMuruDarkFiendSpawns() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
        {
            creature->DespawnOrUnsummon();
            return;
        }

        if (IsShadowswordAdd(creature))
        {
            ScaleShadowswordMaxHealth(creature);
            return;
        }

        if (creature->GetEntry() == NPC_MURU && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            AnnounceMuruSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        shadowswordHealthScaleStates.erase(creature->GetGUID());
        if (creature->GetEntry() == NPC_MURU)
            muruSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class MuruSoloRaidSpellScript : public AllSpellScript
{
public:
    MuruSoloRaidSpellScript() : AllSpellScript("MuruSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || !IsDarkFiendSummonSpell(spell->GetSpellInfo()->Id) ||
            !SoloRaids::Config::PreventMuruDarkFiendSpawns())
            return true;

        Unit* caster = spell->GetCaster();
        return !caster || !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU);
    }
};

class MuruSoloRaidUnitScript : public UnitScript
{
public:
    MuruSoloRaidUnitScript() : UnitScript("MuruSoloRaidUnitScript", true, { UNITHOOK_ON_DAMAGE }) { }

    void OnDamage(Unit* attacker, Unit* /*victim*/, uint32& damage) override
    {
        if (!IsShadowswordAdd(attacker) ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::MuruShadowswordAddsDamagePct());
    }
};

void AddMuruSoloRaidScripts()
{
    new MuruSoloRaidCreatureScript();
    new MuruSoloRaidSpellScript();
    new MuruSoloRaidUnitScript();
}
