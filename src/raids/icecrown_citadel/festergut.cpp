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
constexpr uint32 NPC_FESTERGUT = 36626;
constexpr uint32 SPELL_GASTRIC_BLOAT_10N = 72219;
constexpr uint32 SPELL_GASTRIC_BLOAT_25N = 72551;
constexpr uint32 SPELL_GASTRIC_BLOAT_10H = 72552;
constexpr uint32 SPELL_GASTRIC_BLOAT_25H = 72553;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

std::set<ObjectGuid> festergutSoloAnnouncementSent;

bool IsGastricBloat(uint32 spellId)
{
    return spellId == SPELL_GASTRIC_BLOAT_10N ||
        spellId == SPELL_GASTRIC_BLOAT_25N ||
        spellId == SPELL_GASTRIC_BLOAT_10H ||
        spellId == SPELL_GASTRIC_BLOAT_25H;
}

void CapAura(Unit* unit, uint32 spellId, uint8 maxStacks)
{
    Aura* aura = unit->GetAura(spellId);
    if (!aura)
        return;

    if (maxStacks == 0)
    {
        unit->RemoveAurasDueToSpell(spellId);
        return;
    }

    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceFestergutSoloTweaks(Creature* festergut)
{
    if (!festergut)
        return;

    ObjectGuid const guid = festergut->GetGUID();
    if (festergutSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(festergut->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Festergut. Gastric Bloat capped at " +
        std::to_string(uint32(SoloRaids::Config::FestergutGastricBloatMaxStacks())) + " stacks.").c_str());
    festergutSoloAnnouncementSent.insert(guid);
}

void CapGastricBloat(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        return;

    uint8 const maxStacks = SoloRaids::Config::FestergutGastricBloatMaxStacks();
    CapAura(unit, SPELL_GASTRIC_BLOAT_10N, maxStacks);
    CapAura(unit, SPELL_GASTRIC_BLOAT_25N, maxStacks);
    CapAura(unit, SPELL_GASTRIC_BLOAT_10H, maxStacks);
    CapAura(unit, SPELL_GASTRIC_BLOAT_25H, maxStacks);
}
}

class FestergutSoloRaidCreatureScript : public AllCreatureScript
{
public:
    FestergutSoloRaidCreatureScript() : AllCreatureScript("FestergutSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_FESTERGUT && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceFestergutSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_FESTERGUT)
            festergutSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class FestergutSoloRaidUnitScript : public UnitScript
{
public:
    FestergutSoloRaidUnitScript() : UnitScript("FestergutSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (aura && IsGastricBloat(aura->GetId()))
            CapGastricBloat(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapGastricBloat(unit);
    }
};

void AddFestergutSoloRaidScripts()
{
    new FestergutSoloRaidCreatureScript();
    new FestergutSoloRaidUnitScript();
}
