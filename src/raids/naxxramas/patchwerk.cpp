#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SharedDefines.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_PATCHWERK = 16028;
constexpr uint32 NPC_PATCHWERK_40 = 351028;
constexpr uint32 SPELL_HATEFUL_STRIKE = 41926;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;

std::set<uint32> patchwerkSoloAnnouncementMaps;

bool IsPatchwerk(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_PATCHWERK || entry == NPC_PATCHWERK_40;
}

void AnnouncePatchwerkSoloTweaks(Creature* patchwerk)
{
    if (!patchwerk)
        return;

    Player* player = SoloRaids::GetSoloPlayer(patchwerk->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = patchwerk->GetInstanceId();
    if (patchwerkSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    float const damagePct = SoloRaids::Config::PatchwerkHatefulStrikeDamagePct();
    if (damagePct == 1.0f)
        return;

    std::string message = "mod-solo-raids active: Naxxramas solo tweaks enabled for Patchwerk. Hateful Strike damage set to " + std::to_string(uint32(damagePct * 100.0f)) + "%.";
    player->SendSystemMessage(message.c_str());
    patchwerkSoloAnnouncementMaps.insert(instanceId);
}
}

class PatchwerkSoloRaidCreatureScript : public AllCreatureScript
{
public:
    PatchwerkSoloRaidCreatureScript() : AllCreatureScript("PatchwerkSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsPatchwerk(creature))
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnouncePatchwerkSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsPatchwerk(creature))
            return;

        patchwerkSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class PatchwerkSoloRaidUnitScript : public UnitScript
{
public:
    PatchwerkSoloRaidUnitScript() : UnitScript("PatchwerkSoloRaidUnitScript", true, { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& amount, SpellInfo const* spellInfo) override
    {
        if (!target || !source || !spellInfo || spellInfo->Id != SPELL_HATEFUL_STRIKE || !IsPatchwerk(source) || !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        amount = int32(float(amount) * SoloRaids::Config::PatchwerkHatefulStrikeDamagePct());
    }
};

void AddPatchwerkSoloRaidScripts()
{
    new PatchwerkSoloRaidCreatureScript();
    new PatchwerkSoloRaidUnitScript();
}
