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
constexpr uint32 NPC_RAZORSCALE = 33186;
constexpr uint32 SPELL_FUSE_ARMOR = 64771;
constexpr uint32 SPELL_FUSED_ARMOR = 64774;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<ObjectGuid> razorscaleSoloAnnouncementSent;

void CapFuseArmor(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ULDUAR))
        return;

    unit->RemoveAurasDueToSpell(SPELL_FUSED_ARMOR);

    Aura* aura = unit->GetAura(SPELL_FUSE_ARMOR);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::RazorscaleFuseArmorMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceRazorscaleSoloTweaks(Creature* razorscale)
{
    if (!razorscale)
        return;

    ObjectGuid const guid = razorscale->GetGUID();
    if (razorscaleSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(razorscale->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for Razorscale. Fuse Armor capped at " +
        std::to_string(uint32(SoloRaids::Config::RazorscaleFuseArmorMaxStacks())) + " stacks.").c_str());
    razorscaleSoloAnnouncementSent.insert(guid);
}
}

class RazorscaleSoloRaidCreatureScript : public AllCreatureScript
{
public:
    RazorscaleSoloRaidCreatureScript() : AllCreatureScript("RazorscaleSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_RAZORSCALE && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceRazorscaleSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_RAZORSCALE)
            razorscaleSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class RazorscaleSoloRaidUnitScript : public UnitScript
{
public:
    RazorscaleSoloRaidUnitScript() : UnitScript("RazorscaleSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || (aura->GetId() != SPELL_FUSE_ARMOR && aura->GetId() != SPELL_FUSED_ARMOR))
            return;

        CapFuseArmor(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapFuseArmor(unit);
    }
};

void AddRazorscaleSoloRaidScripts()
{
    new RazorscaleSoloRaidCreatureScript();
    new RazorscaleSoloRaidUnitScript();
}
