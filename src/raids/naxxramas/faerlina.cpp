#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_FAERLINA = 15953;
constexpr uint32 NPC_FAERLINA_40 = 351007;
constexpr uint32 NPC_NAXXRAMAS_WORSHIPPER = 16506;
constexpr uint32 NPC_NAXXRAMAS_WORSHIPPER_40 = 351081;
constexpr uint32 SPELL_WIDOWS_EMBRACE = 28732;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;
constexpr float FAERLINA_SEARCH_RANGE = 200.0f;

std::set<uint32> faerlinaSoloAnnouncementMaps;

bool IsNaxx25(Unit const* unit)
{
    Map const* map = unit ? unit->GetMap() : nullptr;
    return map && map->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
}

bool IsFaerlina(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FAERLINA || entry == NPC_FAERLINA_40;
}

bool IsConfiguredFaerlina(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FAERLINA_40 || (entry == NPC_FAERLINA && IsNaxx25(unit));
}

bool IsConfiguredWorshipper(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_NAXXRAMAS_WORSHIPPER_40 || (entry == NPC_NAXXRAMAS_WORSHIPPER && IsNaxx25(unit));
}

Creature* FindFaerlinaForWorshipper(Unit* worshipper)
{
    if (!worshipper)
        return nullptr;

    if (worshipper->GetEntry() == NPC_NAXXRAMAS_WORSHIPPER_40)
        return worshipper->FindNearestCreature(NPC_FAERLINA_40, FAERLINA_SEARCH_RANGE, true);

    return worshipper->FindNearestCreature(NPC_FAERLINA, FAERLINA_SEARCH_RANGE, true);
}

void AnnounceFaerlinaSoloTweaks(Creature* faerlina)
{
    if (!faerlina || !SoloRaids::Config::FaerlinaWorshippersCastWidowsEmbraceOnDeath())
        return;

    Player* player = SoloRaids::GetSoloPlayer(faerlina->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = faerlina->GetInstanceId();
    if (faerlinaSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Grand Widow Faerlina. Worshippers cast Widow's Embrace on Faerlina when they die.");
    faerlinaSoloAnnouncementMaps.insert(instanceId);
}
}

class FaerlinaSoloRaidCreatureScript : public AllCreatureScript
{
public:
    FaerlinaSoloRaidCreatureScript() : AllCreatureScript("FaerlinaSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsFaerlina(creature))
            return;

        if (creature->IsInCombat() && IsConfiguredFaerlina(creature) && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnounceFaerlinaSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsFaerlina(creature))
            return;

        faerlinaSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class FaerlinaSoloRaidUnitScript : public UnitScript
{
public:
    FaerlinaSoloRaidUnitScript() : UnitScript("FaerlinaSoloRaidUnitScript", true, { UNITHOOK_ON_UNIT_DEATH }) { }

    void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
    {
        if (!SoloRaids::Config::FaerlinaWorshippersCastWidowsEmbraceOnDeath() || !IsConfiguredWorshipper(unit) ||
            !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        Creature* faerlina = FindFaerlinaForWorshipper(unit);
        if (!faerlina)
            return;

        unit->CastSpell(faerlina, SPELL_WIDOWS_EMBRACE, true);
    }
};

void AddFaerlinaSoloRaidScripts()
{
    new FaerlinaSoloRaidCreatureScript();
    new FaerlinaSoloRaidUnitScript();
}
