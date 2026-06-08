#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <set>

namespace
{
constexpr uint32 NPC_AEONUS = 17881;
constexpr uint32 SPELL_THRASH_PROC = 8876;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_MORASS = 269;

std::set<ObjectGuid> aeonusSoloAnnouncementSent;

void AnnounceAeonusSoloTweaks(Creature* aeonus)
{
    if (!aeonus)
        return;

    ObjectGuid const guid = aeonus->GetGUID();
    if (aeonusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(aeonus->GetMap(), SOLO_RAIDS_MAP_BLACK_MORASS);
    if (!player || !SoloRaids::Config::DisableAeonusThrash())
        return;

    player->SendSystemMessage("mod-solo-raids active: Black Morass solo tweaks enabled for Aeonus. Thrash disabled.");
    aeonusSoloAnnouncementSent.insert(guid);
}
}

class AeonusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    AeonusSoloRaidCreatureScript() : AllCreatureScript("AeonusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_AEONUS)
            return;

        if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_MORASS))
            return;

        if (SoloRaids::Config::DisableAeonusThrash())
            creature->RemoveAurasDueToSpell(SPELL_THRASH_PROC);

        if (creature->IsInCombat())
            AnnounceAeonusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_AEONUS)
            return;

        aeonusSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

void AddAeonusSoloRaidScripts()
{
    new AeonusSoloRaidCreatureScript();
}
