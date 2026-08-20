#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_BLOOD_QUEEN_LANA_THEL = 37955;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

struct HealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, HealthScaleState> bloodQueenLanathelHealthScaleStates;
std::set<ObjectGuid> bloodQueenLanathelSoloAnnouncementSent;

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

void ScaleBloodQueenLanathelMaxHealth(Creature* bloodQueen)
{
    if (!bloodQueen || bloodQueen->GetEntry() != NPC_BLOOD_QUEEN_LANA_THEL)
        return;

    ObjectGuid const guid = bloodQueen->GetGUID();
    auto stateItr = bloodQueenLanathelHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(bloodQueen->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
    {
        if (stateItr != bloodQueenLanathelHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(bloodQueen, stateItr->second.baselineMaxHealth);
            bloodQueenLanathelHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == bloodQueenLanathelHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = bloodQueen->GetMaxHealth();
        stateItr = bloodQueenLanathelHealthScaleStates.emplace(
            guid, HealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (bloodQueen->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        bloodQueen->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = bloodQueen->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * SoloRaids::Config::BloodQueenLanathelMaxHealthPct()));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(bloodQueen, scaledMaxHealth);
}

void AnnounceBloodQueenLanathelSoloTweaks(Creature* bloodQueen)
{
    if (!bloodQueen)
        return;

    ObjectGuid const guid = bloodQueen->GetGUID();
    if (bloodQueenLanathelSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(bloodQueen->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Blood-Queen Lana'thel. Max health set to " +
        std::to_string(uint32(SoloRaids::Config::BloodQueenLanathelMaxHealthPct() * 100.0f)) +
        "% after other scaling. Damage set to " +
        std::to_string(uint32(SoloRaids::Config::BloodQueenLanathelDamagePct() * 100.0f)) + "%.").c_str());
    bloodQueenLanathelSoloAnnouncementSent.insert(guid);
}
}

class BloodQueenLanathelSoloRaidCreatureScript : public AllCreatureScript
{
public:
    BloodQueenLanathelSoloRaidCreatureScript() : AllCreatureScript("BloodQueenLanathelSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_BLOOD_QUEEN_LANA_THEL)
            return;

        ScaleBloodQueenLanathelMaxHealth(creature);

        if (creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceBloodQueenLanathelSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_BLOOD_QUEEN_LANA_THEL)
        {
            bloodQueenLanathelHealthScaleStates.erase(creature->GetGUID());
            bloodQueenLanathelSoloAnnouncementSent.erase(creature->GetGUID());
        }
    }
};

class BloodQueenLanathelSoloRaidUnitScript : public UnitScript
{
public:
    BloodQueenLanathelSoloRaidUnitScript() : UnitScript("BloodQueenLanathelSoloRaidUnitScript", true, { UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!attacker || attacker->GetEntry() != NPC_BLOOD_QUEEN_LANA_THEL ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::BloodQueenLanathelDamagePct());
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !attacker || !spellInfo ||
            attacker->GetEntry() != NPC_BLOOD_QUEEN_LANA_THEL ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = int32(float(damage) * SoloRaids::Config::BloodQueenLanathelDamagePct());
    }
};

void AddBloodQueenLanathelSoloRaidScripts()
{
    new BloodQueenLanathelSoloRaidCreatureScript();
    new BloodQueenLanathelSoloRaidUnitScript();
}
