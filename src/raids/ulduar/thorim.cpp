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
constexpr uint32 NPC_THORIM = 32865;
constexpr uint32 NPC_DARK_RUNE_CHAMPION = 32876;
constexpr uint32 NPC_DARK_RUNE_WARBRINGER = 32877;
constexpr uint32 NPC_DARK_RUNE_EVOKER = 32878;
constexpr uint32 NPC_DARK_RUNE_COMMONER = 32904;
constexpr uint32 NPC_DARK_RUNE_WARBRINGER_25 = 33155;
constexpr uint32 NPC_DARK_RUNE_EVOKER_25 = 33156;
constexpr uint32 NPC_DARK_RUNE_COMMONER_25 = 33157;
constexpr uint32 NPC_DARK_RUNE_CHAMPION_25 = 33158;
constexpr uint32 NPC_LIGHTNING_ORB = 33138;
constexpr uint32 SPELL_LIGHTNING_DESTRUCTION = 62393;
constexpr uint32 SPELL_LIGHTNING_CHARGE_BUFF = 62279;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<ObjectGuid> thorimSoloAnnouncementSent;

bool IsThorimArenaWaveAdd(Creature const* creature)
{
    if (!creature)
        return false;

    switch (creature->GetEntry())
    {
        case NPC_DARK_RUNE_CHAMPION:
        case NPC_DARK_RUNE_WARBRINGER:
        case NPC_DARK_RUNE_EVOKER:
        case NPC_DARK_RUNE_COMMONER:
        case NPC_DARK_RUNE_WARBRINGER_25:
        case NPC_DARK_RUNE_EVOKER_25:
        case NPC_DARK_RUNE_COMMONER_25:
        case NPC_DARK_RUNE_CHAMPION_25:
            return true;
        default:
            return false;
    }
}

void PauseArenaAddSpawn(Creature* creature)
{
    if (!creature || !creature->IsAlive() ||
        !SoloRaids::Config::PauseThorimArenaAddSpawns() ||
        !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR) ||
        !IsThorimArenaWaveAdd(creature))
        return;

    creature->DespawnOrUnsummon();
}

void PreventLightningOrbWipe(Creature* creature)
{
    if (!creature || creature->GetEntry() != NPC_LIGHTNING_ORB ||
        !SoloRaids::Config::PreventThorimLightningOrbWipe() ||
        !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
        return;

    creature->DespawnOrUnsummon();
}

void CapLightningChargeStacks(Unit* unit)
{
    if (!unit || unit->GetEntry() != NPC_THORIM ||
        !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
        return;

    Aura* aura = unit->GetAura(SPELL_LIGHTNING_CHARGE_BUFF);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::ThorimLightningChargeMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceThorimSoloTweaks(Creature* thorim)
{
    if (!thorim)
        return;

    ObjectGuid const guid = thorim->GetGUID();
    if (thorimSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(thorim->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Ulduar solo tweaks enabled for Thorim.";
    if (SoloRaids::Config::PauseThorimArenaAddSpawns())
        message += " Arena wave add spawns paused.";
    if (SoloRaids::Config::PreventThorimLightningOrbWipe())
        message += " Lightning Orb wipe prevented.";
    message += " Lightning Charge capped at " +
        std::to_string(uint32(SoloRaids::Config::ThorimLightningChargeMaxStacks())) + " stacks.";

    player->SendSystemMessage(message.c_str());
    thorimSoloAnnouncementSent.insert(guid);
}
}

class ThorimSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ThorimSoloRaidCreatureScript() : AllCreatureScript("ThorimSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_LIGHTNING_ORB)
        {
            PreventLightningOrbWipe(creature);
            return;
        }

        if (IsThorimArenaWaveAdd(creature))
        {
            PauseArenaAddSpawn(creature);
            return;
        }

        if (creature->GetEntry() == NPC_THORIM && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceThorimSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_THORIM)
            thorimSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class ThorimSoloRaidSpellScript : public AllSpellScript
{
public:
    ThorimSoloRaidSpellScript() : AllSpellScript("ThorimSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || spell->GetSpellInfo()->Id != SPELL_LIGHTNING_DESTRUCTION ||
            !SoloRaids::Config::PreventThorimLightningOrbWipe())
            return true;

        Unit* caster = spell->GetCaster();
        return !caster || caster->GetEntry() != NPC_LIGHTNING_ORB ||
            !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_LIGHTNING_DESTRUCTION ||
            !SoloRaids::Config::PreventThorimLightningOrbWipe())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_LIGHTNING_ORB &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class ThorimSoloRaidUnitScript : public UnitScript
{
public:
    ThorimSoloRaidUnitScript() : UnitScript("ThorimSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || aura->GetId() != SPELL_LIGHTNING_CHARGE_BUFF)
            return;

        CapLightningChargeStacks(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapLightningChargeStacks(unit);
    }
};

void AddThorimSoloRaidScripts()
{
    new ThorimSoloRaidCreatureScript();
    new ThorimSoloRaidSpellScript();
    new ThorimSoloRaidUnitScript();
}
