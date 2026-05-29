#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "Unit.h"
#include "UnitScript.h"

#include <algorithm>
#include <set>

namespace
{
constexpr uint32 NPC_FEUGEN = 15930;
constexpr uint32 NPC_FEUGEN_40 = 351002;
constexpr uint32 SPELL_STATIC_FIELD = 28135;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;

std::set<uint32> thaddiusSoloAnnouncementMaps;

bool IsFeugen(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_FEUGEN || entry == NPC_FEUGEN_40;
}

void AnnounceThaddiusSoloTweaks(Creature* feugen)
{
    if (!feugen)
        return;

    Player* player = SoloRaids::GetSoloPlayer(feugen->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = feugen->GetInstanceId();
    if (thaddiusSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Thaddius. Feugen's Static Field mana drain disabled.");
    thaddiusSoloAnnouncementMaps.insert(instanceId);
}
}

class ThaddiusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ThaddiusSoloRaidCreatureScript() : AllCreatureScript("ThaddiusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsFeugen(creature))
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnounceThaddiusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsFeugen(creature))
            return;

        thaddiusSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class ThaddiusSoloRaidUnitScript : public UnitScript
{
public:
    ThaddiusSoloRaidUnitScript() : UnitScript("ThaddiusSoloRaidUnitScript", true, { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& amount, SpellInfo const* spellInfo) override
    {
        if (!target || !source || !spellInfo || spellInfo->Id != SPELL_STATIC_FIELD || !IsFeugen(source) || !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        if (target->GetMaxPower(POWER_MANA) <= 0)
            return;

        int32 const manaToRestore = std::min<int32>(500, target->GetMaxPower(POWER_MANA) - target->GetPower(POWER_MANA));
        if (manaToRestore > 0)
            target->ModifyPower(POWER_MANA, manaToRestore);
    }
};

void AddThaddiusSoloRaidScripts()
{
    new ThaddiusSoloRaidCreatureScript();
    new ThaddiusSoloRaidUnitScript();
}
