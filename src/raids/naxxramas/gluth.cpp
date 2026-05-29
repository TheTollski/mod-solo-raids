#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <map>
#include <set>

namespace
{
constexpr uint32 NPC_GLUTH = 15932;
constexpr uint32 NPC_GLUTH_40 = 351004;
constexpr uint32 NPC_ZOMBIE_CHOW = 16360;
constexpr uint32 NPC_ZOMBIE_CHOW_40 = 351069;
constexpr uint32 SPELL_MORTAL_WOUND = 25646;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;
constexpr uint8 MAX_MORTAL_WOUND_STACKS = 3;

std::set<uint32> gluthSoloAnnouncementMaps;
std::map<ObjectGuid, uint32> gluthHealthBeforeZombieHeal;

bool IsGluth(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_GLUTH || entry == NPC_GLUTH_40;
}

bool IsZombieChow(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_ZOMBIE_CHOW || entry == NPC_ZOMBIE_CHOW_40;
}

void AnnounceGluthSoloTweaks(Creature* gluth)
{
    if (!gluth)
        return;

    Player* player = SoloRaids::GetSoloPlayer(gluth->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = gluth->GetInstanceId();
    if (gluthSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Gluth. Zombie healing disabled, Mortal Wound capped at 3 stacks.");
    gluthSoloAnnouncementMaps.insert(instanceId);
}
}

class GluthSoloRaidCreatureScript : public AllCreatureScript
{
public:
    GluthSoloRaidCreatureScript() : AllCreatureScript("GluthSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsGluth(creature))
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnounceGluthSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsGluth(creature))
            return;

        gluthSoloAnnouncementMaps.erase(creature->GetInstanceId());
        gluthHealthBeforeZombieHeal.erase(creature->GetGUID());
    }
};

class GluthSoloRaidUnitScript : public UnitScript
{
public:
    GluthSoloRaidUnitScript() : UnitScript("GluthSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_DEATH, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura || aura->GetId() != SPELL_MORTAL_WOUND || !SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        if (aura->GetStackAmount() > MAX_MORTAL_WOUND_STACKS)
            aura->SetStackAmount(MAX_MORTAL_WOUND_STACKS);
    }

    void OnUnitDeath(Unit* unit, Unit* killer) override
    {
        if (!IsZombieChow(unit) || !IsGluth(killer) || !SoloRaids::IsSoloMap(killer->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        gluthHealthBeforeZombieHeal[killer->GetGUID()] = killer->GetHealth();
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (!IsGluth(unit))
            return;

        auto itr = gluthHealthBeforeZombieHeal.find(unit->GetGUID());
        if (itr == gluthHealthBeforeZombieHeal.end())
            return;

        uint32 const healthBeforeZombieHeal = itr->second;
        gluthHealthBeforeZombieHeal.erase(itr);

        if (unit->GetHealth() > healthBeforeZombieHeal)
            unit->SetHealth(healthBeforeZombieHeal);
    }
};

void AddGluthSoloRaidScripts()
{
    new GluthSoloRaidCreatureScript();
    new GluthSoloRaidUnitScript();
}
