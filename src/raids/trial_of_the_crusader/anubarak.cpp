#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_ANUBARAK = 34564;
constexpr uint32 SPELL_BERSERK = 26662;
constexpr uint32 SPELL_ACID_DRENCHED_MANDIBLES_10N = 65775;
constexpr uint32 SPELL_ACID_DRENCHED_MANDIBLES_25N = 67861;
constexpr uint32 SPELL_ACID_DRENCHED_MANDIBLES_10H = 67862;
constexpr uint32 SPELL_ACID_DRENCHED_MANDIBLES_25H = 67863;
constexpr uint32 SPELL_EXPOSE_WEAKNESS = 67720;
constexpr uint32 SPELL_EXPOSE_WEAKNESS_NORMAL = 67721;
constexpr uint32 SPELL_EXPOSE_WEAKNESS_HEROIC = 67847;
constexpr uint32 SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER = 649;

std::set<ObjectGuid> anubarakSoloAnnouncementSent;

bool IsExposeWeakness(uint32 spellId)
{
    return spellId == SPELL_EXPOSE_WEAKNESS ||
        spellId == SPELL_EXPOSE_WEAKNESS_NORMAL ||
        spellId == SPELL_EXPOSE_WEAKNESS_HEROIC;
}

bool IsAcidDrenchedMandibles(uint32 spellId)
{
    return spellId == SPELL_ACID_DRENCHED_MANDIBLES_10N ||
        spellId == SPELL_ACID_DRENCHED_MANDIBLES_25N ||
        spellId == SPELL_ACID_DRENCHED_MANDIBLES_10H ||
        spellId == SPELL_ACID_DRENCHED_MANDIBLES_25H;
}

void CapAura(Unit* unit, uint32 spellId, uint8 maxStacks)
{
    Aura* aura = unit->GetAura(spellId);
    if (!aura || aura->GetStackAmount() <= maxStacks)
        return;

    aura->SetStackAmount(maxStacks);
}

void CapExposeWeakness(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return;

    uint8 const maxStacks = SoloRaids::Config::AnubarakTrialExposeWeaknessMaxStacks();
    CapAura(unit, SPELL_EXPOSE_WEAKNESS, maxStacks);
    CapAura(unit, SPELL_EXPOSE_WEAKNESS_NORMAL, maxStacks);
    CapAura(unit, SPELL_EXPOSE_WEAKNESS_HEROIC, maxStacks);
}

void CapAcidDrenchedMandibles(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return;

    uint8 const maxStacks = SoloRaids::Config::AnubarakTrialAcidDrenchedMandiblesMaxStacks();
    CapAura(unit, SPELL_ACID_DRENCHED_MANDIBLES_10N, maxStacks);
    CapAura(unit, SPELL_ACID_DRENCHED_MANDIBLES_25N, maxStacks);
    CapAura(unit, SPELL_ACID_DRENCHED_MANDIBLES_10H, maxStacks);
    CapAura(unit, SPELL_ACID_DRENCHED_MANDIBLES_25H, maxStacks);
}

void RemoveBerserk(Unit* unit)
{
    if (!SoloRaids::Config::DisableAnubarakTrialEnrage() ||
        !unit ||
        unit->GetEntry() != NPC_ANUBARAK ||
        !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return;

    unit->RemoveAurasDueToSpell(SPELL_BERSERK);
}

void AnnounceAnubarakSoloTweaks(Creature* anubarak)
{
    if (!anubarak)
        return;

    ObjectGuid const guid = anubarak->GetGUID();
    if (anubarakSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(anubarak->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Trial of the Crusader solo tweaks enabled for Anub'arak. Expose Weakness capped at " +
        std::to_string(uint32(SoloRaids::Config::AnubarakTrialExposeWeaknessMaxStacks())) +
        " stacks. Acid-Drenched Mandibles capped at " +
        std::to_string(uint32(SoloRaids::Config::AnubarakTrialAcidDrenchedMandiblesMaxStacks())) + " stacks.";
    if (SoloRaids::Config::DisableAnubarakTrialEnrage())
        message += " Berserk timer disabled.";

    player->SendSystemMessage(message.c_str());
    anubarakSoloAnnouncementSent.insert(guid);
}
}

class AnubarakTrialSoloRaidCreatureScript : public AllCreatureScript
{
public:
    AnubarakTrialSoloRaidCreatureScript() : AllCreatureScript("AnubarakTrialSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_ANUBARAK && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
            AnnounceAnubarakSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_ANUBARAK)
            anubarakSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class AnubarakTrialSoloRaidUnitScript : public UnitScript
{
public:
    AnubarakTrialSoloRaidUnitScript() : UnitScript("AnubarakTrialSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura)
            return;

        if (IsExposeWeakness(aura->GetId()))
            CapExposeWeakness(unit);
        else if (IsAcidDrenchedMandibles(aura->GetId()))
            CapAcidDrenchedMandibles(unit);
        else if (aura->GetId() == SPELL_BERSERK)
            RemoveBerserk(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapExposeWeakness(unit);
        CapAcidDrenchedMandibles(unit);
        RemoveBerserk(unit);
    }
};

void AddAnubarakTrialSoloRaidScripts()
{
    new AnubarakTrialSoloRaidCreatureScript();
    new AnubarakTrialSoloRaidUnitScript();
}
