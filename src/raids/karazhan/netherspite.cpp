#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <set>

namespace
{
constexpr uint32 NPC_NETHERSPITE = 15689;
constexpr uint32 SPELL_PERSEVERANCE_NETHER_BUFF = 30466;
constexpr uint32 SPELL_SERENITY_NETHER_BUFF = 30467;
constexpr uint32 SPELL_DOMINANCE_NETHER_BUFF = 30468;
constexpr uint32 SOLO_RAIDS_MAP_KARAZHAN = 532;

std::set<ObjectGuid> netherspiteSoloAnnouncementSent;

void RemoveNetherspitePortalBuffs(Creature* netherspite)
{
    if (!netherspite || !SoloRaids::Config::DisableNetherspitePortalBuffs())
        return;

    netherspite->RemoveAurasDueToSpell(SPELL_PERSEVERANCE_NETHER_BUFF);
    netherspite->RemoveAurasDueToSpell(SPELL_SERENITY_NETHER_BUFF);
    netherspite->RemoveAurasDueToSpell(SPELL_DOMINANCE_NETHER_BUFF);
}

void AnnounceNetherspiteSoloTweaks(Creature* netherspite)
{
    if (!netherspite)
        return;

    ObjectGuid const guid = netherspite->GetGUID();
    if (netherspiteSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(netherspite->GetMap(), SOLO_RAIDS_MAP_KARAZHAN);
    if (!player || !SoloRaids::Config::DisableNetherspitePortalBuffs())
        return;

    player->SendSystemMessage("mod-solo-raids active: Karazhan solo tweaks enabled for Netherspite. Portal beam buffs on Netherspite disabled.");
    netherspiteSoloAnnouncementSent.insert(guid);
}
}

class NetherspiteSoloRaidCreatureScript : public AllCreatureScript
{
public:
    NetherspiteSoloRaidCreatureScript() : AllCreatureScript("NetherspiteSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_NETHERSPITE || !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_KARAZHAN))
            return;

        RemoveNetherspitePortalBuffs(creature);

        if (creature->IsInCombat())
            AnnounceNetherspiteSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_NETHERSPITE)
            return;

        netherspiteSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

void AddNetherspiteSoloRaidScripts()
{
    new NetherspiteSoloRaidCreatureScript();
}
