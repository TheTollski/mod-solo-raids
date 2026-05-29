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

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Patchwerk. Hateful Strike disabled.");
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

class PatchwerkSoloRaidSpellScript : public AllSpellScript
{
public:
    PatchwerkSoloRaidSpellScript() : AllSpellScript("PatchwerkSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_HATEFUL_STRIKE)
            return;

        Unit* caster = spell->GetCaster();
        if (IsPatchwerk(caster) && SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddPatchwerkSoloRaidScripts()
{
    new PatchwerkSoloRaidCreatureScript();
    new PatchwerkSoloRaidSpellScript();
}
