#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellInfo.h"

#include <set>

namespace
{
constexpr uint32 NPC_SUNBLADE_CABALIST = 25363;
constexpr uint32 SPELL_IGNITE_MANA = 46543;
constexpr uint32 SOLO_RAIDS_MAP_SUNWELL_PLATEAU = 580;

std::set<uint32> sunwellTrashSoloAnnouncementMaps;

void AnnounceSunwellTrashSoloTweaks(Creature* creature)
{
    if (!creature || !SoloRaids::Config::DisableSunbladeCabalistIgniteMana())
        return;

    Player* player = SoloRaids::GetSoloPlayer(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU);
    if (!player)
        return;

    uint32 const instanceId = creature->GetInstanceId();
    if (sunwellTrashSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Sunwell Plateau solo tweaks enabled for trash. Sunblade Cabalist's Ignite Mana disabled.");
    sunwellTrashSoloAnnouncementMaps.insert(instanceId);
}
}

class SunwellPlateauTrashSoloRaidCreatureScript : public AllCreatureScript
{
public:
    SunwellPlateauTrashSoloRaidCreatureScript() : AllCreatureScript("SunwellPlateauTrashSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_SUNBLADE_CABALIST && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            AnnounceSunwellTrashSoloTweaks(creature);
    }
};

class SunwellPlateauTrashSoloRaidSpellScript : public AllSpellScript
{
public:
    SunwellPlateauTrashSoloRaidSpellScript() : AllSpellScript("SunwellPlateauTrashSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_IGNITE_MANA ||
            !SoloRaids::Config::DisableSunbladeCabalistIgniteMana())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_SUNBLADE_CABALIST &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddSunwellPlateauTrashSoloRaidScripts()
{
    new SunwellPlateauTrashSoloRaidCreatureScript();
    new SunwellPlateauTrashSoloRaidSpellScript();
}
