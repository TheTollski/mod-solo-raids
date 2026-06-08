#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

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
constexpr uint32 NPC_TEMPORUS = 17880;
constexpr uint32 SPELL_HASTEN = 31458;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_MORASS = 269;

std::set<ObjectGuid> temporusSoloAnnouncementSent;

void AnnounceTemporusSoloTweaks(Creature* temporus)
{
    if (!temporus)
        return;

    ObjectGuid const guid = temporus->GetGUID();
    if (temporusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(temporus->GetMap(), SOLO_RAIDS_MAP_BLACK_MORASS);
    if (!player || !SoloRaids::Config::DisableTemporusHasten())
        return;

    player->SendSystemMessage("mod-solo-raids active: Black Morass solo tweaks enabled for Temporus. Hasten disabled.");
    temporusSoloAnnouncementSent.insert(guid);
}
}

class TemporusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    TemporusSoloRaidCreatureScript() : AllCreatureScript("TemporusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_TEMPORUS)
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_MORASS))
            AnnounceTemporusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_TEMPORUS)
            return;

        temporusSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class TemporusSoloRaidSpellScript : public AllSpellScript
{
public:
    TemporusSoloRaidSpellScript() : AllSpellScript("TemporusSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_HASTEN || !SoloRaids::Config::DisableTemporusHasten())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_TEMPORUS && SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_BLACK_MORASS))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddTemporusSoloRaidScripts()
{
    new TemporusSoloRaidCreatureScript();
    new TemporusSoloRaidSpellScript();
}
