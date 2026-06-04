#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_GLUTH = 15932;
constexpr uint32 NPC_GLUTH_40 = 351004;
constexpr uint32 NPC_ZOMBIE_CHOW = 16360;
constexpr uint32 NPC_ZOMBIE_CHOW_40 = 351069;
constexpr uint32 SPELL_MORTAL_WOUND = 25646;
constexpr uint32 SPELL_ENRAGE = 28371;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;

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

    std::string message = "mod-solo-raids active: Naxxramas solo tweaks enabled for Gluth.";
    if (SoloRaids::Config::DisableGluthZombieHealing())
        message += " Zombie healing disabled.";

    message += " Mortal Wound capped at " + std::to_string(uint32(SoloRaids::Config::GluthMortalWoundMaxStacks())) + " stacks.";

    float const enrageDurationPct = SoloRaids::Config::GluthEnrageDurationPct();
    if (enrageDurationPct != 1.0f)
        message += " Enrage duration set to " + std::to_string(uint32(enrageDurationPct * 100.0f)) + "%.";

    player->SendSystemMessage(message.c_str());
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
        if (!unit || !aura)
            return;

        if (aura->GetId() == SPELL_MORTAL_WOUND && SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_NAXXRAMAS))
        {
            uint8 const maxStacks = SoloRaids::Config::GluthMortalWoundMaxStacks();
            if (aura->GetStackAmount() > maxStacks)
                aura->SetStackAmount(maxStacks);
        }

        if (aura->GetId() == SPELL_ENRAGE && IsGluth(unit) && SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
        {
            float const durationPct = SoloRaids::Config::GluthEnrageDurationPct();
            if (durationPct == 1.0f || aura->GetMaxDuration() <= 0)
                return;

            int32 const duration = int32(float(aura->GetMaxDuration()) * durationPct);
            aura->SetMaxDuration(duration);
            aura->SetDuration(duration);
        }
    }

    void OnUnitDeath(Unit* unit, Unit* killer) override
    {
        if (!SoloRaids::Config::DisableGluthZombieHealing() || !IsZombieChow(unit) || !IsGluth(killer) || !SoloRaids::IsSoloMap(killer->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        gluthHealthBeforeZombieHeal[killer->GetGUID()] = killer->GetHealth();
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (!SoloRaids::Config::DisableGluthZombieHealing() || !IsGluth(unit))
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
