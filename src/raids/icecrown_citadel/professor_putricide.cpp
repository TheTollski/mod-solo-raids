#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_PROFESSOR_PUTRICIDE = 36678;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

struct HealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, HealthScaleState> putricideHealthScaleStates;
std::set<ObjectGuid> putricideSoloAnnouncementSent;

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

void ScaleProfessorPutricideMaxHealth(Creature* putricide)
{
    if (!putricide || putricide->GetEntry() != NPC_PROFESSOR_PUTRICIDE)
        return;

    ObjectGuid const guid = putricide->GetGUID();
    auto stateItr = putricideHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(putricide->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
    {
        if (stateItr != putricideHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(putricide, stateItr->second.baselineMaxHealth);
            putricideHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == putricideHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = putricide->GetMaxHealth();
        stateItr = putricideHealthScaleStates.emplace(
            guid, HealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (putricide->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        putricide->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = putricide->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::ProfessorPutricideMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(putricide, scaledMaxHealth);
}

void AnnounceProfessorPutricideSoloTweaks(Creature* putricide)
{
    if (!putricide)
        return;

    ObjectGuid const guid = putricide->GetGUID();
    if (putricideSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(putricide->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Professor Putricide. Max health set to " +
        std::to_string(uint32(SoloRaids::Config::ProfessorPutricideMaxHealthPct() * 100.0f)) +
        "% after other scaling. Damage set to " +
        std::to_string(uint32(SoloRaids::Config::ProfessorPutricideDamagePct() * 100.0f)) + "%.").c_str());
    putricideSoloAnnouncementSent.insert(guid);
}
}

class ProfessorPutricideSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ProfessorPutricideSoloRaidCreatureScript() : AllCreatureScript("ProfessorPutricideSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_PROFESSOR_PUTRICIDE)
            return;

        ScaleProfessorPutricideMaxHealth(creature);

        if (creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceProfessorPutricideSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_PROFESSOR_PUTRICIDE)
        {
            putricideHealthScaleStates.erase(creature->GetGUID());
            putricideSoloAnnouncementSent.erase(creature->GetGUID());
        }
    }
};

class ProfessorPutricideSoloRaidUnitScript : public UnitScript
{
public:
    ProfessorPutricideSoloRaidUnitScript() : UnitScript("ProfessorPutricideSoloRaidUnitScript", true, { UNITHOOK_MODIFY_MELEE_DAMAGE }) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!attacker || attacker->GetEntry() != NPC_PROFESSOR_PUTRICIDE ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::ProfessorPutricideDamagePct());
    }
};

void AddProfessorPutricideSoloRaidScripts()
{
    new ProfessorPutricideSoloRaidCreatureScript();
    new ProfessorPutricideSoloRaidUnitScript();
}
