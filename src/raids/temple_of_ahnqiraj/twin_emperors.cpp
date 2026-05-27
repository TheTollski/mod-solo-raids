#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SharedDefines.h"

#include <set>

namespace
{
constexpr uint32 NPC_VEKNILASH = 15275;
constexpr uint32 NPC_VEKLOR = 15276;
constexpr uint32 SPELL_HEAL_BROTHER = 7393;
constexpr uint32 SOLO_RAIDS_MAP_TEMPLE_OF_AHNQIRAJ = 531;

std::set<uint32> twinEmperorsSoloAnnouncementMaps;

bool IsTwinEmperor(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_VEKNILASH || entry == NPC_VEKLOR;
}

void AnnounceTwinEmperorsSoloTweaks(Creature* twin)
{
    if (!twin)
        return;

    Player* player = SoloRaids::GetSoloPlayer(twin->GetMap(), SOLO_RAIDS_MAP_TEMPLE_OF_AHNQIRAJ);
    if (!player)
        return;

    uint32 const instanceId = twin->GetInstanceId();
    if (twinEmperorsSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Temple of Ahn'Qiraj solo tweaks enabled for the Twin Emperors. Heal Brother disabled.");
    twinEmperorsSoloAnnouncementMaps.insert(instanceId);
}
}

class TwinEmperorsSoloRaidCreatureScript : public AllCreatureScript
{
public:
    TwinEmperorsSoloRaidCreatureScript() : AllCreatureScript("TwinEmperorsSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsTwinEmperor(creature))
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_TEMPLE_OF_AHNQIRAJ))
            AnnounceTwinEmperorsSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsTwinEmperor(creature))
            return;

        twinEmperorsSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class TwinEmperorsSoloRaidSpellScript : public AllSpellScript
{
public:
    TwinEmperorsSoloRaidSpellScript() : AllSpellScript("TwinEmperorsSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_HEAL_BROTHER)
            return;

        Unit* caster = spell->GetCaster();
        if (IsTwinEmperor(caster) && SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_TEMPLE_OF_AHNQIRAJ))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddTwinEmperorsSoloRaidScripts()
{
    new TwinEmperorsSoloRaidCreatureScript();
    new TwinEmperorsSoloRaidSpellScript();
}
