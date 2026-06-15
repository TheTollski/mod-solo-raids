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

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_GURTOGG_BLOODBOIL = 22948;
constexpr uint32 SPELL_ACIDIC_WOUND = 40481;
constexpr uint32 SPELL_BEWILDERING_STRIKE = 40491;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_TEMPLE = 564;

std::set<ObjectGuid> gurtoggSoloAnnouncementSent;

void CapAcidicWoundStacks(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_BLACK_TEMPLE))
        return;

    Aura* aura = unit->GetAura(SPELL_ACIDIC_WOUND);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::GurtoggAcidicWoundMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceGurtoggSoloTweaks(Creature* gurtogg)
{
    if (!gurtogg)
        return;

    ObjectGuid const guid = gurtogg->GetGUID();
    if (gurtoggSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(gurtogg->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Black Temple solo tweaks enabled for Gurtogg Bloodboil. Acidic Wound capped at " +
        std::to_string(uint32(SoloRaids::Config::GurtoggAcidicWoundMaxStacks())) + " stacks.";
    if (SoloRaids::Config::DisableGurtoggBewilderingStrike())
        message += " Bewildering Strike disabled.";

    player->SendSystemMessage(message.c_str());
    gurtoggSoloAnnouncementSent.insert(guid);
}
}

class GurtoggBloodboilSoloRaidCreatureScript : public AllCreatureScript
{
public:
    GurtoggBloodboilSoloRaidCreatureScript() : AllCreatureScript("GurtoggBloodboilSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_GURTOGG_BLOODBOIL && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            AnnounceGurtoggSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_GURTOGG_BLOODBOIL)
            gurtoggSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class GurtoggBloodboilSoloRaidSpellScript : public AllSpellScript
{
public:
    GurtoggBloodboilSoloRaidSpellScript() : AllSpellScript("GurtoggBloodboilSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_BEWILDERING_STRIKE ||
            !SoloRaids::Config::DisableGurtoggBewilderingStrike())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_GURTOGG_BLOODBOIL &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class GurtoggBloodboilSoloRaidUnitScript : public UnitScript
{
public:
    GurtoggBloodboilSoloRaidUnitScript() : UnitScript("GurtoggBloodboilSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || aura->GetId() != SPELL_ACIDIC_WOUND)
            return;

        CapAcidicWoundStacks(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapAcidicWoundStacks(unit);
    }
};

void AddGurtoggBloodboilSoloRaidScripts()
{
    new GurtoggBloodboilSoloRaidCreatureScript();
    new GurtoggBloodboilSoloRaidSpellScript();
    new GurtoggBloodboilSoloRaidUnitScript();
}
