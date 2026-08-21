#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_SINDRAGOSA = 36853;
constexpr uint32 NPC_ICE_TOMB = 36980;
constexpr uint32 SPELL_FROST_BEACON = 70126;
constexpr uint32 SPELL_ICE_TOMB_TARGET = 69712;
constexpr uint32 SPELL_ICE_TOMB_DUMMY = 69675;
constexpr uint32 SPELL_ICE_TOMB_UNTARGETABLE = 69700;
constexpr uint32 SPELL_ICE_TOMB_DAMAGE = 70157;
constexpr uint32 SPELL_MYSTIC_BUFFET_AURA = 70127;
constexpr uint32 SPELL_MYSTIC_BUFFET_CASTER_AURA = 70128;
constexpr uint32 SPELL_BERSERK = 26662;
constexpr uint32 SPELL_BERSERK2 = 47008;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

std::set<uint32> sindragosaSoloAnnouncementInstances;

bool IsIceTombAura(uint32 spellId)
{
    return spellId == SPELL_FROST_BEACON ||
        spellId == SPELL_ICE_TOMB_TARGET ||
        spellId == SPELL_ICE_TOMB_DUMMY ||
        spellId == SPELL_ICE_TOMB_UNTARGETABLE ||
        spellId == SPELL_ICE_TOMB_DAMAGE;
}

bool IsMysticBuffet(uint32 spellId)
{
    return spellId == SPELL_MYSTIC_BUFFET_AURA ||
        spellId == SPELL_MYSTIC_BUFFET_CASTER_AURA;
}

void RemoveIceTombAuras(Unit* unit)
{
    if (!unit || !SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
        !SoloRaids::Config::DisableSindragosaIceTomb())
        return;

    unit->RemoveAurasDueToSpell(SPELL_FROST_BEACON);
    unit->RemoveAurasDueToSpell(SPELL_ICE_TOMB_TARGET);
    unit->RemoveAurasDueToSpell(SPELL_ICE_TOMB_DUMMY);
    unit->RemoveAurasDueToSpell(SPELL_ICE_TOMB_UNTARGETABLE);
    unit->RemoveAurasDueToSpell(SPELL_ICE_TOMB_DAMAGE);
}

void CapMysticBuffet(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        return;

    Aura* aura = unit->GetAura(SPELL_MYSTIC_BUFFET_AURA);
    if (!aura)
        aura = unit->GetAura(SPELL_MYSTIC_BUFFET_CASTER_AURA);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::SindragosaMysticBuffetMaxStacks();
    if (maxStacks == 0)
    {
        unit->RemoveAurasDueToSpell(SPELL_MYSTIC_BUFFET_AURA);
        unit->RemoveAurasDueToSpell(SPELL_MYSTIC_BUFFET_CASTER_AURA);
        return;
    }

    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void RemoveSindragosaBerserk(Unit* unit)
{
    if (!unit || unit->GetEntry() != NPC_SINDRAGOSA ||
        !SoloRaids::Config::DisableSindragosaBerserk() ||
        !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        return;

    unit->RemoveAurasDueToSpell(SPELL_BERSERK);
    unit->RemoveAurasDueToSpell(SPELL_BERSERK2);
}

void AnnounceSindragosaSoloTweaks(Creature* sindragosa)
{
    if (!sindragosa)
        return;

    Player* player = SoloRaids::GetSoloPlayer(sindragosa->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    uint32 const instanceId = sindragosa->GetInstanceId();
    if (sindragosaSoloAnnouncementInstances.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Sindragosa. Ice Tomb " +
        std::string(SoloRaids::Config::DisableSindragosaIceTomb() ? "disabled" : "enabled") +
        ". Berserk " +
        std::string(SoloRaids::Config::DisableSindragosaBerserk() ? "disabled" : "enabled") +
        ". Mystic Buffet capped at " +
        std::to_string(uint32(SoloRaids::Config::SindragosaMysticBuffetMaxStacks())) + " stacks.").c_str());
    sindragosaSoloAnnouncementInstances.insert(instanceId);
}
}

class SindragosaSoloRaidCreatureScript : public AllCreatureScript
{
public:
    SindragosaSoloRaidCreatureScript() : AllCreatureScript("SindragosaSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_SINDRAGOSA &&
            creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        {
            RemoveSindragosaBerserk(creature);
            AnnounceSindragosaSoloTweaks(creature);
        }
        else if (creature->GetEntry() == NPC_ICE_TOMB &&
            SoloRaids::Config::DisableSindragosaIceTomb() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            creature->DespawnOrUnsummon();
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_SINDRAGOSA)
            sindragosaSoloAnnouncementInstances.erase(creature->GetInstanceId());
    }
};

class SindragosaSoloRaidUnitScript : public UnitScript
{
public:
    SindragosaSoloRaidUnitScript() : UnitScript("SindragosaSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura)
            return;

        if (IsIceTombAura(aura->GetId()))
            RemoveIceTombAuras(unit);
        else if (IsMysticBuffet(aura->GetId()))
            CapMysticBuffet(unit);
        else if (aura->GetId() == SPELL_BERSERK || aura->GetId() == SPELL_BERSERK2)
            RemoveSindragosaBerserk(unit);
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* /*attacker*/, uint32& /*damage*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo && IsMysticBuffet(spellInfo->Id))
            CapMysticBuffet(target);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        RemoveIceTombAuras(unit);
        CapMysticBuffet(unit);
        RemoveSindragosaBerserk(unit);
    }
};

void AddSindragosaSoloRaidScripts()
{
    new SindragosaSoloRaidCreatureScript();
    new SindragosaSoloRaidUnitScript();
}
