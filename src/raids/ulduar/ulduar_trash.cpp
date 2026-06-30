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
constexpr uint32 NPC_DARK_RUNE_THUNDERER = 33754;
constexpr uint32 NPC_DARK_RUNE_THUNDERER_25 = 33757;
constexpr uint32 SPELL_LIGHTNING_BRAND = 63612;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<uint32> ulduarTrashSoloAnnouncementMaps;

bool IsDarkRuneThunderer(Unit const* unit)
{
    return unit && (unit->GetEntry() == NPC_DARK_RUNE_THUNDERER ||
        unit->GetEntry() == NPC_DARK_RUNE_THUNDERER_25);
}

void CapLightningBrandStacks(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ULDUAR))
        return;

    Aura* aura = unit->GetAura(SPELL_LIGHTNING_BRAND);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::UlduarTrashLightningBrandMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceUlduarTrashSoloTweaks(Creature* creature)
{
    if (!creature)
        return;

    Player* player = SoloRaids::GetSoloPlayer(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    uint32 const instanceId = creature->GetInstanceId();
    if (ulduarTrashSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo trash tweaks enabled. Dark Rune Thunderer Lightning Brand capped at " +
        std::to_string(uint32(SoloRaids::Config::UlduarTrashLightningBrandMaxStacks())) + " stacks.").c_str());
    ulduarTrashSoloAnnouncementMaps.insert(instanceId);
}
}

class UlduarTrashSoloRaidCreatureScript : public AllCreatureScript
{
public:
    UlduarTrashSoloRaidCreatureScript() : AllCreatureScript("UlduarTrashSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && IsDarkRuneThunderer(creature) && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceUlduarTrashSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && IsDarkRuneThunderer(creature))
            ulduarTrashSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class UlduarTrashSoloRaidUnitScript : public UnitScript
{
public:
    UlduarTrashSoloRaidUnitScript() : UnitScript("UlduarTrashSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || aura->GetId() != SPELL_LIGHTNING_BRAND)
            return;

        CapLightningBrandStacks(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapLightningBrandStacks(unit);
    }
};

void AddUlduarTrashSoloRaidScripts()
{
    new UlduarTrashSoloRaidCreatureScript();
    new UlduarTrashSoloRaidUnitScript();
}
