#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_PRINCE_KELESETH = 37972;
constexpr uint32 NPC_PRINCE_TALDARAM = 37973;
constexpr uint32 NPC_PRINCE_VALANAR = 37970;
constexpr uint32 SPELL_SHADOW_LANCE = 71405;
constexpr uint32 SPELL_EMPOWERED_SHADOW_LANCE = 71815;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

std::set<uint32> bloodPrinceCouncilSoloAnnouncementInstances;

bool IsBloodPrinceCouncil(uint32 entry)
{
    return entry == NPC_PRINCE_KELESETH ||
        entry == NPC_PRINCE_TALDARAM ||
        entry == NPC_PRINCE_VALANAR;
}

bool IsShadowLance(uint32 spellId)
{
    return spellId == SPELL_SHADOW_LANCE || spellId == SPELL_EMPOWERED_SHADOW_LANCE;
}

void AnnounceBloodPrinceCouncilSoloTweaks(Creature* prince)
{
    if (!prince || !IsBloodPrinceCouncil(prince->GetEntry()))
        return;

    uint32 const instanceId = prince->GetInstanceId();
    if (bloodPrinceCouncilSoloAnnouncementInstances.count(instanceId) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(prince->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    float const meleePct = SoloRaids::Config::BloodPrinceCouncilMeleeDamagePct();
    float const shadowLancePct = SoloRaids::Config::BloodPrinceCouncilShadowLanceDamagePct();
    if (meleePct == 1.0f && shadowLancePct == 1.0f)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Blood Prince Council. Melee damage set to " +
        std::to_string(uint32(meleePct * 100.0f)) + "% and Shadow Lance damage set to " +
        std::to_string(uint32(shadowLancePct * 100.0f)) + "%.").c_str());
    bloodPrinceCouncilSoloAnnouncementInstances.insert(instanceId);
}
}

class BloodPrinceCouncilSoloRaidCreatureScript : public AllCreatureScript
{
public:
    BloodPrinceCouncilSoloRaidCreatureScript() : AllCreatureScript("BloodPrinceCouncilSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && IsBloodPrinceCouncil(creature->GetEntry()) && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceBloodPrinceCouncilSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && IsBloodPrinceCouncil(creature->GetEntry()))
            bloodPrinceCouncilSoloAnnouncementInstances.erase(creature->GetInstanceId());
    }
};

class BloodPrinceCouncilSoloRaidUnitScript : public UnitScript
{
public:
    BloodPrinceCouncilSoloRaidUnitScript() : UnitScript("BloodPrinceCouncilSoloRaidUnitScript", true, { UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!target || !attacker ||
            !IsBloodPrinceCouncil(attacker->GetEntry()) ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::BloodPrinceCouncilMeleeDamagePct());
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !attacker || !spellInfo ||
            attacker->GetEntry() != NPC_PRINCE_KELESETH ||
            !IsShadowLance(spellInfo->Id) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = int32(float(damage) * SoloRaids::Config::BloodPrinceCouncilShadowLanceDamagePct());
    }
};

void AddBloodPrinceCouncilSoloRaidScripts()
{
    new BloodPrinceCouncilSoloRaidCreatureScript();
    new BloodPrinceCouncilSoloRaidUnitScript();
}
