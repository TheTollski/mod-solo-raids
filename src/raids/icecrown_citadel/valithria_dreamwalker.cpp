#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "InstanceScript.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "Unit.h"
#include "UnitScript.h"

#include <limits>
#include <set>
#include <string>

namespace
{
constexpr uint32 DATA_VALITHRIA_DREAMWALKER = 10;
constexpr uint32 NPC_VALITHRIA_DREAMWALKER = 36789;
constexpr uint32 NPC_RISEN_ARCHMAGE = 37868;
constexpr uint32 NPC_BLAZING_SKELETON = 36791;
constexpr uint32 NPC_SUPPRESSER = 37863;
constexpr uint32 NPC_BLISTERING_ZOMBIE = 37934;
constexpr uint32 NPC_GLUTTONOUS_ABOMINATION = 37886;
constexpr uint32 NPC_ROT_WORM = 37907;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;
constexpr float VALITHRIA_SEARCH_RANGE = 300.0f;

std::set<uint32> valithriaDreamwalkerSoloAnnouncementInstances;

bool IsValithriaDreamwalker(Unit const* unit)
{
    return unit && unit->GetEntry() == NPC_VALITHRIA_DREAMWALKER;
}

bool IsValithriaEncounterAdd(Unit const* unit)
{
    if (!unit)
        return false;

    switch (unit->GetEntry())
    {
        case NPC_RISEN_ARCHMAGE:
        case NPC_BLAZING_SKELETON:
        case NPC_SUPPRESSER:
        case NPC_BLISTERING_ZOMBIE:
        case NPC_GLUTTONOUS_ABOMINATION:
        case NPC_ROT_WORM:
            return true;
        default:
            return false;
    }
}

Creature* FindValithriaDreamwalker(Unit* unit)
{
    if (!unit)
        return nullptr;

    Creature* valithria = unit->FindNearestCreature(NPC_VALITHRIA_DREAMWALKER, VALITHRIA_SEARCH_RANGE, true);
    if (!valithria)
        return nullptr;

    InstanceScript* instance = valithria->GetInstanceScript();
    if (!instance || instance->GetBossState(DATA_VALITHRIA_DREAMWALKER) != IN_PROGRESS)
        return nullptr;

    return valithria;
}

uint32 CalculateAddKillHeal(Unit const* add)
{
    float const healPct = SoloRaids::Config::ValithriaDreamwalkerAddKillHealPct();
    if (!add || healPct <= 0.0f)
        return 0;

    float const heal = float(add->GetMaxHealth()) * healPct;
    if (heal >= float(std::numeric_limits<uint32>::max()))
        return std::numeric_limits<uint32>::max();

    return uint32(heal);
}

void AnnounceValithriaDreamwalkerSoloTweaks(Creature* valithria)
{
    if (!valithria ||
        (SoloRaids::Config::ValithriaDreamwalkerAddKillHealPct() == 0.0f &&
            SoloRaids::Config::ValithriaDreamwalkerHealingReceivedPct() == 1.0f))
        return;

    Player* player = SoloRaids::GetSoloPlayer(valithria->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    uint32 const instanceId = valithria->GetInstanceId();
    if (valithriaDreamwalkerSoloAnnouncementInstances.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Valithria Dreamwalker. Encounter add kills heal Valithria for " +
        std::to_string(uint32(SoloRaids::Config::ValithriaDreamwalkerAddKillHealPct() * 100.0f)) +
        "% of the killed add's max health. Healing received set to " +
        std::to_string(uint32(SoloRaids::Config::ValithriaDreamwalkerHealingReceivedPct() * 100.0f)) + "%.").c_str());
    valithriaDreamwalkerSoloAnnouncementInstances.insert(instanceId);
}
}

class ValithriaDreamwalkerSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ValithriaDreamwalkerSoloRaidCreatureScript() : AllCreatureScript("ValithriaDreamwalkerSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_VALITHRIA_DREAMWALKER ||
            !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        InstanceScript* instance = creature->GetInstanceScript();
        if (instance && instance->GetBossState(DATA_VALITHRIA_DREAMWALKER) == IN_PROGRESS)
            AnnounceValithriaDreamwalkerSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_VALITHRIA_DREAMWALKER)
            valithriaDreamwalkerSoloAnnouncementInstances.erase(creature->GetInstanceId());
    }
};

class ValithriaDreamwalkerSoloRaidUnitScript : public UnitScript
{
public:
    ValithriaDreamwalkerSoloRaidUnitScript() : UnitScript("ValithriaDreamwalkerSoloRaidUnitScript", true, { UNITHOOK_ON_UNIT_DEATH, UNITHOOK_MODIFY_HEAL_RECEIVED }) { }

    void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
    {
        if (!IsValithriaEncounterAdd(unit) ||
            !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        uint32 const heal = CalculateAddKillHeal(unit);
        if (heal == 0)
            return;

        Creature* valithria = FindValithriaDreamwalker(unit);
        Player* player = SoloRaids::GetSoloPlayer(unit->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
        if (!valithria || !player)
            return;

        Unit::DealHeal(player, valithria, heal);
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* /*spellInfo*/) override
    {
        if (heal == 0 || (!IsValithriaDreamwalker(target) && !IsValithriaDreamwalker(healer)))
            return;

        Unit* valithria = IsValithriaDreamwalker(target) ? target : healer;
        if (!SoloRaids::IsSoloMap(valithria->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        heal = uint32(float(heal) * SoloRaids::Config::ValithriaDreamwalkerHealingReceivedPct());
    }
};

void AddValithriaDreamwalkerSoloRaidScripts()
{
    new ValithriaDreamwalkerSoloRaidCreatureScript();
    new ValithriaDreamwalkerSoloRaidUnitScript();
}
